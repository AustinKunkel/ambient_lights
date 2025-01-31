import asyncio
import time
import cv2
import numpy as np
from sklearn.cluster import KMeans
import threading
import queue

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
      "res-x": 640,
      "res-y": 480
    }

frame_queue = queue.Queue(maxsize=10)

stop_capture = False


async def main(strip):
  global stop_capture
  global frame_queue
  stop_capture = False
  while not frame_queue.empty():
    frame_queue.get_nowait()
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

    led_dict = await setup(cap)

    await main_capture_loop(cap, strip, led_dict)

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
      
    h, w = frame.shape[:2] # frame defaults to 640 x 480, defines the height and width, defines the height and width

    print("h:", h, "w:", w)
    v_offset = int(sc_settings["v-offset"]) # vertical offset (pixels from top and bottom)
    h_offset = int(sc_settings["h-offset"]) # horizontal offset
    # ensures v and h offset do not collide
    if(v_offset < 0) or (h_offset < 0):
      print(f"offsets less than 0, h:{h_offset}, v:{v_offset}")
      v_offset = 0 if v_offset < 0 else v_offset
      h_offset = 0 if h_offset < 0 else h_offset

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

    led_dict = {}

    # if(is_fwd <= 1): # if it is reverse
    #   fwd_multiplier = -1
    #   next_index = l_count + r_count + t_count + b_count  
    
    if(is_bl <= 1): # if it is bottom right, clockwise while counting backwards
      print("is bottom right") 
      next_index = 0
      await setup_right_side(r_count, led_dict, w, h, h_offset, next_index, -1)
      next_index += r_count
      await setup_top_side(t_count, led_dict, w, v_offset, next_index, -1)
      next_index += t_count
      await setup_left_side(l_count, led_dict, h, h_offset, next_index, -1)
      next_index+= l_count
      await setup_bottom_side(b_count, led_dict, w, h, v_offset, next_index, -1)
    else: # it is bottom left, clockwise, unless its reversed
      await setup_left_side(l_count, led_dict, h, h_offset, next_index, 1)
      next_index += r_count * fwd_multiplier
      await setup_top_side(t_count, led_dict, w, v_offset, next_index, 1)
      next_index += t_count * fwd_multiplier
      await setup_right_side(r_count, led_dict, w, h, h_offset, next_index, 1)
      next_index += r_count * fwd_multiplier
      await setup_bottom_side(b_count, led_dict, w, h, v_offset, next_index, 1)

    return led_dict
  except Exception as e:
    print(e)
    return


"""
set up left side of screen, and add the led data to the dictionary
"""
async def setup_left_side(count, led_dict, h, h_offset, next_index, fwd_multiplier):
  try:
    spacing = (h - 1) // (count - 1) # spacing in between the leds

    next_index += count
    start = count
    stop = 0
    x_index = 1 + h_offset # starts at this
    print("left:",x_index)
    for i in range(start, stop, -1): # start to stop - 1
      y_index = i * spacing if(i * spacing) < h else h - 1
      led_dict[(next_index) + ((count - i) + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up right side of screen, and add the led data to the dictionary
"""
async def setup_right_side(count, led_dict, w, h, h_offset, next_index, fwd_multiplier):
  try:
    spacing = (h - 1) // (count - 1)
    next_index += count
    start = 0
    stop = count
    x_index = (w - 1) - h_offset
    print("right:",x_index)
    for i in range(start, stop, 1):
      y_index = i * spacing if(i * spacing) < h else h - 1
      led_dict[next_index + (i + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up top of screen, and add the led data to the dictionary
"""
async def setup_top_side(count, led_dict, w, v_offset, next_index, fwd_multiplier):
  try:
    spacing = (w - 1) // (count - 1)
    spacing += 1
    next_index += count
    start = 0
    stop = count
    y_index = 1 + v_offset # starts here
    print("top:",y_index)
    for i in range(start, stop, 1):
      x_index = i * spacing if(i * spacing) < w else w - 1
      led_dict[next_index + (i + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return

"""
set up bottom of screen, and add the led data to the dictionary
"""
async def setup_bottom_side(count, led_dict, w, h, v_offset, next_index, fwd_multiplier):
  try:
    spacing = (w - 1) // (count - 2)
    next_index += count
    start = count
    stop = 0
    y_index = (h - 1) - v_offset
    print("bottom:",y_index)
    for i in range(start, stop, -1):
      x_index = i * spacing if(i * spacing) < w else w - 1
      led_dict[next_index + ((count - i) + 1)*fwd_multiplier] = (y_index, x_index)
  except Exception as e:
    print(e)
    return


async def main_capture_loop(cap, strip, led_dict):
  """
  cap: video input
  strip: led strip object
  led: dictionary of led positions

  the main loop for updating and showing the 
  colors on the led strip
  """
  global stop_capture
  try:
    cap.set(cv2.CAP_PROP_FPS, 60)

    capture_thread = threading.Thread(target=video_capture_thread, args=(cap,frame_queue, .01), daemon=True)
    capture_thread.start()

    time.sleep(.1) # Give it some time to start the thread

    update_thread = threading.Thread(target=update_leds, args=(strip, frame_queue, led_dict), daemon=True)
    update_thread.start()

    while not stop_capture:
      await asyncio.sleep(.1)

  except asyncio.CancelledError:
    print("capture_screen was cancelled")
    raise
  except Exception as e:
    print(e)
    return    
  finally:
    stop_capture = True
    if capture_thread.is_alive():
      capture_thread.join(timeout=.5)
    if update_thread.is_alive():
      update_thread.join(timeout=.5)
    while not frame_queue.empty():
      frame_queue.get_nowait()
    cap.release()
    cv2.destroyAllWindows()
  
def update_leds(strip, frame_queue, led_dict):
  """
   Gets the next frame from the queue and updates the led
   strip according to how the strip was set up
   """
  global stop_capture
  try:
    while not stop_capture:
        if not frame_queue.empty():
          frame = frame_queue.get()
          if led_dict:
            for index, (y, x) in led_dict.items():
              color = frame[y, x]
              strip.setPixelColor(index, Color(int(color[2]), int(color[1]), int(color[0])))
        strip.show()
        time.sleep(.006)
  except Exception as e:
     print(f"Error in update_leds: {e}")



def video_capture_thread(cap, frame_queue, delay):
   global stop_capture
   while not stop_capture:
      ret, frame = cap.read()
      if ret and not frame_queue.full():
        frame_queue.put(frame)
      time.sleep(delay)


async def capture_avg_screen_color(strip):
  """
  Captures Screen's average color data
  """
  global stop_capture
  try:
    cap = cv2.VideoCapture(0)  # initialize video capture
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    if not cap.isOpened():
      print("could not open capture card")
      return
        
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, int(sc_settings["res-x"]))
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, int(sc_settings["res-y"]))

    # Initialize previous color to black
    prev_color = np.array([0, 0, 0])

    capture_thread = threading.Thread(target=video_capture_thread, args=(cap,frame_queue, .5), daemon=True)
    capture_thread.start()

    while True:
      if not frame_queue.empty():
        frame = await asyncio.to_thread(frame_queue.get)
        new_color = await find_dominant_color(frame)

        # Smooth transition from previous color to new color
        await smooth_transition(prev_color, new_color, strip, steps=60, delay=0.003)

        # Update prev_color to the current color
        prev_color = new_color

        await asyncio.sleep(0.4)

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
    stop_capture = True
    if(capture_thread.is_alive()):
      capture_thread.join(timeout=1)
    while not frame_queue.empty():
      frame_queue.get_nowait()
    cap.release()
    cv2.destroyAllWindows()
    return


async def find_dominant_color(frame, k=1):
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


async def smooth_transition(prev_color, new_color, strip, steps=50, delay=0.01):
    """
    Smoothly transition from prev_color to new_color over a number of steps.
    """
    prev_color = np.array(prev_color)
    new_color = np.array(new_color)

    # Calculate the step increment for each color channel
    step_values = (new_color - prev_color) / steps

    for i in range(steps):
        # Calculate the intermediate color for this step
        intermediate_color = prev_color + step_values * i
        intermediate_color = np.clip(intermediate_color, 0, 255).astype(int)

        # Set the color on the strip
        await show_color(Color(intermediate_color[0], intermediate_color[1], intermediate_color[2]), strip)

        # Wait before applying the next color step
        await asyncio.sleep(delay)

async def show_color(color, strip):
    """Show a solid color on all LEDs."""
    left = int(sc_settings["left-count"])
    right = int(sc_settings["right-count"] )
    top = int(sc_settings["top-count"])
    bottom = int(sc_settings["bottom-count"])
    count = left + right + top + bottom 
    try:
        for i in range(count):
            strip.setPixelColor(i,color)
        strip.show()
        return
    except asyncio.CancelledError:
        print("show_color() was cancelled")
