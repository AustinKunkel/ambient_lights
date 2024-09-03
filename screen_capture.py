import asyncio
import json
import cv2
import time
import numpy as np
from sklearn.cluster import KMeans

from rpi_ws281x import PixelStrip, Color

import led_functions

# screen capture settings, gets updated when the app runs
sc_settings =     {
      "v-offset": 0, 
      "h-offset": 0,
      "avg-color": 0, 
      "left-count": 0, 
      "top-count": 0,
      "right-count": 0,
      "bottom-count": 0,
      "diff-asp-ratio": 0
    }


async def main():
  await capture_screen() if int(sc_settings["avg-color"]) == 0 else await capture_avg_screen_color()


"""
start : LED index where it starts
stop : LED index where it ends
will loop through and update each LED accordingly
starts bottom left then clockwise
"""
async def capture_screen():
  try:
    cap = cv2.VideoCapture(0) # initialize video capture
    if not cap.isOpened():
      print("could not open capture card")
      return
    
    ret, frame = cap.read()

    print("x: ",frame.shape[1], "y:", frame.shape[0])

    if not ret:
      print("failed to capture initial frame")
      cap.release()
      return
    
    h, w = frame.shape[:2] # frame defaults to 640 x 480

    print("h:", h, "w:", w)
    v_offset = int(sc_settings["v-offset"]) # vertical offset (pixels from top and bottom)
    h_offset = int(sc_settings["h-offset"]) # horizontal offset
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
    print("l_count:", l_count)
    r_count = int(sc_settings["right-count"])
    print("r_count:", r_count)
    t_count = int(sc_settings["top-count"])
    print("t_count:", t_count)
    b_count = int(sc_settings["bottom-count"])
    print("b_count:", b_count)

    diff_asp_ratio = int(sc_settings["diff-asp-ratio"])
    print("diff aspect ratio:", diff_asp_ratio)
    total_h = h
    total_w = w

    # if its a different aspect ratio, it will "squeeze" the screen
    # to represent the offsets better
    if diff_asp_ratio >= 1:
      total_h = h - (v_offset * 2) # adjusts h for the offsets
      total_w = w - (h_offset * 2) #adjusts w for the offsets

    # the spacing inside of the range of the offsets
    l_spacing = total_h // l_count
    r_spacing = total_h // r_count
    t_spacing = total_w // t_count
    b_spacing = total_w // b_count
    
     # led offsets, in index, how far in the leds will start, and how far in they will end
     # the offset // spacing. 
     # l_led_offset: the left side leds: the inset from the edges
    l_led_offset = v_offset // (h // l_count)
    r_led_offset = v_offset // (h // r_count)
    t_led_offset = h_offset // (w // t_count)
    b_led_offset = h_offset // (w // b_count)
    
    while True:
      frame = await capture_frame(cap, w, h) # will change to await the screen capture  
      
      # left side
      next_index = l_led_offset# next led index
      start = l_led_offset
      stop = l_count - l_led_offset
      for i in range(stop, start, -1): # start to stop - 1, reverse because it needs to go through array reverse 
        x_index = h_offset # starts at this
        y_index = i * l_spacing
        color = frame[y_index, x_index]
        led_functions.strip.setPixelColor(next_index, Color(int(color[2]), int(color[1]), int(color[0])))
        next_index += 1
      next_index = l_count

      # top side
      start = t_led_offset
      stop = t_count - t_led_offset
      for i in range(start, stop, 1):
        x_index = i * t_spacing
        y_index = v_offset # starts here
        color = frame[y_index, x_index]
        led_functions.strip.setPixelColor(next_index, Color(int(color[2]), int(color[1]), int(color[0])))
        next_index += 1
      next_index  = l_count + t_count

      # right side
      start = r_led_offset
      stop = r_count - r_led_offset
      for i in range(start, stop, 1):
        x_index = (frame.shape[1] - 1) - h_offset
        y_index = i * r_spacing
        color = frame[y_index, x_index]
        led_functions.strip.setPixelColor(next_index, Color(int(color[2]), int(color[1]), int(color[0])))
        next_index += 1 
      next_index = r_count + l_count + t_count

      # bottom side
      start = b_led_offset
      stop = b_count - b_led_offset
      for i in range(stop, start, -1):
        x_index = i * b_spacing
        y_index = (frame.shape[0] - 1) - v_offset
        color = frame[y_index, x_index]
        led_functions.strip.setPixelColor(next_index, Color(int(color[2]), int(color[1]), int(color[0])))
        next_index += 1
      led_functions.strip.show()
      await asyncio.sleep(.001) # so other actions can interrupt it

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
async def capture_avg_screen_color():
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
        
      await led_functions.show_color(Color(color[0], color[1], color[2]))

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

