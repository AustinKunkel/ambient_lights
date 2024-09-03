import asyncio
import cv2
import numpy as np
from sklearn.cluster import KMeans

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
      "bl": 0
    }


async def main(strip):
  await capture_screen(strip) if int(sc_settings["avg-color"]) == 0 else await capture_avg_screen_color(strip)


"""
captures screen, first maps the leds to their respective
indices in the frame, and then sends it off to the main 
loop so it can update the leds rapidly
"""
async def capture_screen(strip):
  try:
    cap = cv2.VideoCapture(0) # initialize video capture
    if not cap.isOpened():
      print("could not open capture card")
      return
    
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
    
   
"""
Sets up screen capture led positions

returns a dictionary with 
key: led index
value: tuple with (y, x) 
"""
async def setup(cap):
  ret, frame = cap.read()

  print("x: ",frame.shape[1], "y:", frame.shape[0])

  if not ret:
    print("failed to capture initial frame")
    cap.release()
    return
  
  led_dict = {}
    
  h, w = frame.shape[:2] # frame defaults to 640 x 480

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

  # if(is_fwd <= 1): # if it is reverse
  #   fwd_multiplier = -1
  #   next_index = l_count + r_count + t_count + b_count  
  
  if(is_bl <= 1): # if it is bottom right, clockwise while counting backwards 
    next_index += r_count * fwd_multiplier
    await setup_right_side(r_count, led_dict, w, h, h_offset, next_index, -1)
    next_index += t_count * fwd_multiplier
    await setup_top_side(t_count, led_dict, w, v_offset, next_index, -1)
    next_index += l_count * fwd_multiplier
    await setup_left_side(l_count, led_dict, h, h_offset, next_index, -1)
    next_index+= b_count * fwd_multiplier
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


"""
set up left side of screen, and add the led data to the dictionary
"""
async def setup_left_side(count, led_dict, h, h_offset, next_index, fwd_multiplier):
  spacing = h // count # spacing in between the leds

  #next_index += led_offset * fwd_multiplier # next led index
  # for the index on the screen where it starts. has to start at end because of how screen array is
  start = count
  stop = 0
  x_index = h_offset # starts at this
  for i in range(start, stop, fwd_multiplier): # start to stop - 1
    y_index = i * spacing
    led_dict[next_index] = (y_index, x_index)
    next_index += 1 * fwd_multiplier

"""
set up right side of screen, and add the led data to the dictionary
"""
async def setup_right_side(count, led_dict, w, h, h_offset, next_index, fwd_multiplier):
  spacing = h // count

  #next_index += led_offset * fwd_multiplier
  start = 0
  stop = count
  x_index = (w - 1) - h_offset
  for i in range(start, stop, fwd_multiplier):
    y_index = i * spacing
    led_dict[next_index] = (y_index, x_index)
    next_index += 1 * fwd_multiplier

"""
set up top of screen, and add the led data to the dictionary
"""
async def setup_top_side(count, led_dict, w, v_offset, next_index, fwd_multiplier):
  spacing = w // count

  #next_index+= led_offset * fwd_multiplier
  start = 0
  stop = count
  y_index = v_offset # starts here
  for i in range(start, stop, fwd_multiplier):
    x_index = i * spacing
    led_dict[next_index] = (y_index, x_index)
    next_index += 1 * fwd_multiplier

"""
set up bottom of screen, and add the led data to the dictionary
"""
async def setup_bottom_side(count, led_dict, w, h, v_offset, next_index, fwd_multiplier):
  spacing = w // count
  #next_index += 1 * fwd_multiplier
  start = count
  stop = 0
  y_index = (h - 1) - v_offset
  for i in range(start, stop, fwd_multiplier):
    x_index = i * spacing
    led_dict[next_index] = (y_index, x_index)
    next_index += 1 * fwd_multiplier

"""
cap: video input
strip: led strip object
led: dictionary of led positions

the main loop for updating and showing the 
colors on the led strip
"""
async def main_capture_loop(cap, strip, led_dict):
  while True:
    ret, frame = cap.read()

    if not ret:
      print("failed to capture frame")
      return
    
    for index, (y, x) in led_dict.items():
      color = frame[y, x]
      strip.setPixelColor(index, Color(int(color[2]), int(color[1]), int(color[0])))

    strip.show()
    await asyncio.sleep(.01) # so other actions can interrupt it
    


async def capture_frame(cap, w, h):
  ret, frame = cap.read()

  if not ret:
    print("failed to capture frame")
    return
  
  resized_frame = cv2.resize(frame, (w,h))

  return resized_frame

"""
Captures Screen's average color data
"""
async def capture_avg_screen_color(strip):
  try:
    cap = cv2.VideoCapture(0) # initialize video capture
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    w = 320
    h = 240
    if not cap.isOpened():
      print("could not open capture card")
      return


    while True:
      frame = await capture_frame(cap=cap, w = w,  h = h)

      color = await find_dominant_color(frame)
        
      await strip.show_color(Color(color[0], color[1], color[2]))

      await asyncio.sleep(.001)

  except asyncio.CancelledError:
    print("capture_screen was cancelled")
    cap.release()
    cv2.destroyAllWindows()
  finally:
    cap.release()
    cv2.destroyAllWindows()
    return
  
async def find_dominant_color(frame, k = 1):
  frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

  pixels = frame_rgb.reshape(-1,3)

  kmeans = KMeans(n_clusters=k)
  kmeans.fit(pixels)

  dominant_color = np.round(kmeans.cluster_centers_[0]).astype(int)
  return dominant_color # array of rgb values