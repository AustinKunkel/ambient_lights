import asyncio
import time
import cv2
import numpy as np
from sklearn.cluster import KMeans
import threading
import queue
import config

from rpi_ws281x import Color

# screen capture settings, gets updated when the app runs
sc_settings =     {
      "v-offset": 0, 
      "h-offset": 0,
      "avg-color": 0, 
      "left-count": 0, 
      "top-count": 0,
      "right-count": 0,
      "bottom-count": 0,
      "fwd": 0,
      "bl": 0,
      "res-x": 640,
      "res-y": 480,
      "blend-mode": 0,
      "blend-depth": 10
    }

stop_capture_event = threading.Event()
sound_capture = False
avg_color = None
blend_mode_active = False

async def main(strip):
  global sound_capture, stop_capture_event
  global blend_mode_active
  print(f"Sound capture: {sound_capture}")
  stop_capture_event.clear()
  blend_mode_active = int(sc_settings['blend-mode']) == 1
  await capture_screen(strip) if int(sc_settings["avg-color"]) == 0 else await capture_avg_screen_color(strip)


async def capture_screen(strip):
  """
  captures screen, first maps the leds to their respective
  indices in the frame, and then sends it off to the main 
  loop so it can update the leds rapidly
  """
  try:
    cap = cv2.VideoCapture(0) # initialize video capture
    if not cap.isOpened():
      print("could not open capture card")
      return

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, int(sc_settings["res-x"]))
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, int(sc_settings["res-y"]))

    led_list = await setup(cap)

    await main_capture_loop(cap, strip, led_list)

  except asyncio.CancelledError:
      print("capture_screen was cancelled")
      cap.release()
      cv2.destroyAllWindows()
  except Exception as e:
    print(e)
    cap.release()
    cv2.destroyAllWindows()
    return
  finally:
    cap.release()
    cv2.destroyAllWindows()
    return
    
async def setup(cap):
  """
  Sets up screen capture led positions

  returns a dictionary with 
  key: led index
  value: tuple with (y, x) 
  """
  try:
    ret, frame = cap.read()

    print("x: ",cap.get(cv2.CAP_PROP_FRAME_WIDTH), "y:", cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

    if not ret:
      print("failed to capture initial frame")
      cap.release()
      return
    
    # resize frame to desired size
    frame = cv2.resize(frame, (int(sc_settings['res-x']), int(sc_settings['res-y'])))
      
    h, w = frame.shape[:2] # frame defaults to 640 x 480, defines the height and width

    print("h:", h, "w:", w)
    v_offset = int(sc_settings["v-offset"]) # vertical offset (pixels from top and bottom)
    h_offset = int(sc_settings["h-offset"]) # horizontal offset

    v_offset = 0 if v_offset < 0 else v_offset
    h_offset = 0 if h_offset < 0 else h_offset

    # ensures v and h offset do not collide
    if(v_offset > (h // 2)):
      print("v collides")
      v_offset = (h // 2) - 1

    print("set v offset to:", v_offset)

    if(h_offset > (w // 2)):
      h_offset = (w // 2) - 1
      print("h collides")

    print("set h offset to:", h_offset)

    l_count = int(sc_settings["left-count"])
    r_count = int(sc_settings["right-count"])
    t_count = int(sc_settings["top-count"])
    b_count = int(sc_settings["bottom-count"])

    #is_fwd = int(sc_settings["fwd"])
    is_bl = int(sc_settings["bl"]) # is bottom left

    fwd_multiplier = 1
    next_index = 0

    led_list = [(0, 0)] * config.LED_COUNT # list containing the indexes and a Tuple (y, x) values to read from

    print("led count:", len(led_list))

    # if(is_fwd <= 1): # if it is reverse
    #   fwd_multiplier = -1
    #   next_index = l_count + r_count + t_count + b_count  
    
    if(is_bl <= 1): # if it is bottom right, clockwise while counting backwards
      print("is bottom right") 
      next_index = 0
      await setup_right_side(r_count, led_list, w, h, h_offset, next_index, -1)
      next_index += r_count
      await setup_top_side(t_count, led_list, w, v_offset, next_index, -1)
      next_index += t_count
      await setup_left_side(l_count, led_list, h, h_offset, next_index, -1)
      next_index+= l_count
      await setup_bottom_side(b_count, led_list, w, h, v_offset, next_index, -1)
    else: # it is bottom left, clockwise, unless its reversed
      await setup_left_side(l_count, led_list, h, h_offset, next_index, 1)
      next_index += r_count * fwd_multiplier
      await setup_top_side(t_count, led_list, w, v_offset, next_index, 1)
      next_index += t_count * fwd_multiplier
      await setup_right_side(r_count, led_list, w, h, h_offset, next_index, 1)
      next_index += r_count * fwd_multiplier
      await setup_bottom_side(b_count, led_list, w, h, v_offset, next_index, 1)

    return led_list
  except Exception as e:
    print(e)
    return


"""
set up left side of screen, and add the led data to the dictionary
"""
async def setup_left_side(count, led_list, h, h_offset, next_index, fwd_multiplier):
  try:
    spacing = (h - 1) / (count - 1) # spacing in between the leds

    next_index += count
    start = count
    stop = 0
    x_index = 1 + h_offset # starts at this
    print("left:",x_index)
    for i in range(start, stop, -1): # start to stop - 1
      y_index = int(i * spacing) if(i * spacing) < h else h - 1
      led_list[(next_index) + ((count - i) + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up right side of screen, and add the led data to the dictionary
"""
async def setup_right_side(count, led_list, w, h, h_offset, next_index, fwd_multiplier):
  try:
    spacing = (h - 1) / (count - 1)
    next_index += count
    start = 0
    stop = count
    x_index = (w - 1) - h_offset
    print("right:",x_index)
    for i in range(start, stop, 1):
      y_index = int(i * spacing) if(i * spacing) < h else h - 1
      led_list[next_index + (i + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up top of screen, and add the led data to the dictionary
"""
async def setup_top_side(count, led_list, w, v_offset, next_index, fwd_multiplier):
  try:
    spacing = (w - 1) / (count - 1)
    next_index += count
    start = 0
    stop = count
    y_index = 1 + v_offset # starts here
    print("top:",y_index)
    for i in range(start, stop, 1):
      x_index = int(i * spacing) if(i * spacing) < w else w - 1
      led_list[next_index + (i + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up bottom of screen, and add the led data to the dictionary
"""
async def setup_bottom_side(count, led_list, w, h, v_offset, next_index, fwd_multiplier):
  try:
    spacing = (w - 1) / (count - 1)
    next_index += count
    start = count
    stop = 0
    y_index = (h - 1) - v_offset
    print("bottom:",y_index)
    for i in range(start, stop, -1):
      x_index = int(i * spacing) if(i * spacing) < w else w - 1
      led_list[next_index + ((count - i) + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return


async def main_capture_loop(cap, strip, led_list):
  """
  the main loop for updating and showing the 
  colors on the led strip

  :param: cap: video input
  :param: strip: led strip object
  :param: led_list: dictionary of led positions
  """
  global stop_capture_event
  try:
    cap.set(cv2.CAP_PROP_FPS, 60)

    update_thread = threading.Thread(target=update_led_colors, args=(strip, cap, led_list), daemon=True)
    update_thread.start()

    while not stop_capture_event.is_set():
      await asyncio.sleep(.1)
    cap.release()
    cv2.destroyAllWindows()

  except asyncio.CancelledError:
    print("capture_screen was cancelled")
    cap.release()
    cv2.destroyAllWindows()
    raise
  except Exception as e:
    print(e)
    cap.release()
    cv2.destroyAllWindows()
    return
  finally:
    stop_capture_event.set()
    if update_thread.is_alive():
      update_thread.join(timeout=.5)
    cap.release()
    cv2.destroyAllWindows()

def update_led_colors(strip, cap, led_list):
  """
   Gets the next frame from the queue and updates the led
   strip according to how the strip was set up
   """
  global stop_capture_event, blend_mode_active, sound_capture
  try:
    while not stop_capture_event.is_set():
        # print("update_colors started")
        # time_1 = time.time()
        ret, frame = cap.read()
        if not ret: continue
        frame = cv2.resize(frame, (int(sc_settings['res-x']), int(sc_settings['res-y'])))
        if led_list:
          precomputed_colors = {} # dictionary with index as keys (only used with blend mode)
          if blend_mode_active:
            precomputed_colors = precompute_blended_colors(led_list, frame, depth=3) # precompute the colors for fast lookup
    
          for index, (y, x) in enumerate(led_list):
            color = frame[y, x]
            if blend_mode_active: # use blended colors if blend mode is on
              color = precomputed_colors[index]
            b, g, r = color
            strip.setPixelColor(index, Color(r, g, b))
          # time_2 = time.time()
          # print(f"update_colors ended: {time_2 - time_1}")
          if not sound_capture:
            strip.show()
          if not blend_mode_active:
            time.sleep(.006)
    cap.release()
    cv2.destroyAllWindows()
  except Exception as e:
    print(f"Error in update_led_colors: {e}")

def precompute_blended_colors(led_list, frame, depth=3):
  """
  Precompute blended colors for all LEDs before updating them.

  :param led_list: Dictionary mapping LED indices to (y, x) pixel locations
  :param frame: The captured screen frame (NumPy array)
  :param depth: Number of previous LEDs to blend
  :return: Dictionary mapping LED indices to (b, g, r) blended colors
  """
  blended_colors = {}
  depth = sc_settings["blend-depth"]

  depth = int(depth) if depth else 3

  for index in range(len(led_list)):
     blended_colors[index] = blend_colors(led_list, frame, index, depth)

  return blended_colors

def blend_colors(led_list, frame, index, depth=3):
  """Blends an LED's color by averaging previous and next LED colors up to a given depth.

  :param led_list: Dictionary mapping LED indices to (y, x) pixel coordinates.
  :param frame: The captured frame containing pixel color data (BGR format).
  :param index: The index of the current LED in led_list.
  :param depth: The number of previous and next LEDs to blend with (default: 3).
  :returns: A tuple of (b, g, r) representing the blended color.
  """
  r_total, g_total, b_total = 0, 0, 0
  count = 0

  for i in range(-depth, depth + 1):
    check_index = (index + i) % len(led_list) # the index we will be taking the color from
    check_pixel_location = led_list[check_index] # Tuple (y, x)

    if check_pixel_location:
      color = frame[int(check_pixel_location[0]), int(check_pixel_location[1])]
      r_total += color[2]
      g_total += color[1]
      b_total += color[0]
      count += 1

  # Prevent division by zero
  if count == 0:
    return (0, 0, 0)

  # Blend the colors
  r = r_total // count
  g = g_total // count
  b = b_total // count

  return (b, g, r)

async def capture_avg_screen_color(strip):
  """
  Captures Screen's average color data
  """
  global stop_capture_event
  try:
    avg_color_thread = threading.Thread(target=run_avg_screen_color, args=(strip,), daemon=True)
    avg_color_thread.start()
    print("Starting average color thread")
    await asyncio.sleep(.3)
    while not stop_capture_event.is_set():
      await asyncio.sleep(.1)

  except asyncio.CancelledError:
    pass
  except Exception as e:
    print(e)
  finally:
    stop_capture_event.set()
    if(avg_color_thread.is_alive()):
      avg_color_thread.join(timeout=1)
    return


def run_avg_screen_color(strip):
  """The thread to run the avg screen color function"""
  global stop_capture_event
  try:
    print("Running average screen capture")
    cap = cv2.VideoCapture(0)  # initialize video capture
    if not cap.isOpened():
      print("Could not open capture card")
      return
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, int(sc_settings["res-x"]))
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, int(sc_settings["res-y"]))

    # Initialize previous color to black
    prev_color = np.array([0, 0, 0])

    while not stop_capture_event.is_set():
      ret, frame = cap.read()
      if ret and frame is None:
        print("Could not capture frame!")
        continue
      new_color = find_dominant_color(frame)
      smooth_transition(prev_color, new_color, strip, steps=60, delay=.003)
      prev_color = new_color
    cap.release()
    cv2.destroyAllWindows()
  except Exception as e:
    print("Error occurred in average screen color thread:", e)
    cap.release()
    cv2.destroyAllWindows()
    return
  finally:
    stop_capture_event.set()
    cap.release()
    cv2.destroyAllWindows()
    return

def find_dominant_color(frame, k=1):
  frame = cv2.resize(frame, (50, 50))  # Reduce processing time
  frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

  # Reshape frame into a 2D array of pixels
  pixels = frame_rgb.reshape(-1, 3)

  # Apply KMeans to find k clusters
  kmeans = KMeans(n_clusters=k)
  kmeans.fit(pixels)

  # Get the labels (which cluster each pixel belongs to)
  labels = kmeans.labels_

  # Find the most frequent cluster label (most dominant color)
  unique, counts = np.unique(labels, return_counts=True)
  dominant_cluster = unique[np.argmax(counts)]

  # Get the dominant color
  dominant_color = np.round(kmeans.cluster_centers_[dominant_cluster]).astype(int)
  return dominant_color  # Return the most dominant color as RGB


def smooth_transition(prev_color, new_color, strip, steps=30, delay=0.01):
  """
  Smoothly transition from prev_color to new_color over a number of steps.
  """
  global avg_color
  prev_color = np.array(prev_color)
  new_color = np.array(new_color)

  # Calculate the step increment for each color channel
  step_values = (new_color - prev_color) / steps

  for i in range(steps):
    # Calculate the intermediate color for this step
    intermediate_color = prev_color + step_values * i
    intermediate_color = np.clip(intermediate_color, 0, 255).astype(int)

    # Set the color on the strip
    color = Color(intermediate_color[0], intermediate_color[1], intermediate_color[2])
    avg_color = color
    if not sound_capture:
      show_color(color, strip)

    # Wait before applying the next color step
    time.sleep(delay)

def show_color(color, strip):
  """Show a solid color on all LEDs."""
  left = int(sc_settings["left-count"])
  right = int(sc_settings["right-count"] )
  top = int(sc_settings["top-count"])
  bottom = int(sc_settings["bottom-count"])
  count = left + right + top + bottom 
  for i in range(count):
    strip.setPixelColor(i,color)
  strip.show()
