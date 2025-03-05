from rpi_ws281x import Color
import screen_capture as sc
import time
import led_functions as lf
import numpy as np
from scipy.signal import butter, filtfilt
import traceback

NUM_CHANNELS = 1 # constant for number of audio channels (sound_capture.py reads)

l_count = int(sc.sc_settings["left-count"])
r_count = int(sc.sc_settings["right-count"])
t_count = int(sc.sc_settings["top-count"])
b_count = int(sc.sc_settings["bottom-count"])

is_static_color = False
change_brightness = True
strip_color = None
capturing_with_avg = False

past_low_rms = None
past_mid_rms = None
past_high_rms = None

is_capt = False

decay_rate = .8 # ratio that the size of the bars will be at after they go

def setup(strip):
  global l_count, r_count, t_count, b_count
  global is_static_color, strip_color, change_brightness, capturing_with_avg, is_capt
  l_count = int(sc.sc_settings["left-count"])
  r_count = int(sc.sc_settings["right-count"])
  t_count = int(sc.sc_settings["top-count"])
  b_count = int(sc.sc_settings["bottom-count"])

  is_capt = int(lf.led_values['capt']) == 1
  is_fx = int(lf.led_values['fx']) == 1
  is_avg_color = int(sc.sc_settings['avg-color']) == 1

  print(f"capt: {is_capt}, fx: {is_fx}, avg: {is_avg_color}")

  # color doesnt change if there is no effects, 
  if not is_capt and not is_fx:
    is_static_color = True
    change_brightness = True

  # but should if its average color
  if is_capt and is_avg_color:
    capturing_with_avg = True
    is_static_color = True
    change_brightness = True
  print(f"is static color: {is_static_color}, change brightness: {change_brightness}")

  strip_color = lf.hex_to_color(lf.led_values['col'])
  if not strip_color:
    pixel_color = strip.getPixelColor(0)
    # Extract red, green, and blue components from the 24-bit color
    r = (pixel_color >> 16) & 0xFF
    g = (pixel_color >> 8) & 0xFF
    b = pixel_color & 0xFF
    strip_color = Color(r, g, b)

def raised_cosine_filter(signal, alpha=0.5, num_taps=21):
  """
  Applies a Raised Cosine filter to smooth the input signal.
    
  Parameters:
  - signal (array): The input signal to be filtered.
  - alpha (float): The filter's alpha parameter controlling the sharpness of the cosine.
  - num_taps (int): The number of taps (filter length) used for smoothing.
    
  Returns:
  - array: The filtered (smoothed) signal.
  """

  # Generate time vector for the filter (centered around zero)
  t = np.linspace(-num_taps // 2, num_taps // 2, num_taps)

  # Generate Raised Cosine filter coefficients
  rc_filter = np.sinc(t) * np.cos(np.pi * alpha * t) / (1 - (2 * alpha * t)**2)
  rc_filter /= np.sum(rc_filter)  # Normalize filter

  # Convolve the signal with the filter
  return np.convolve(signal, rc_filter, mode='same')

def butter_bandpass(lowcut, highcut, fs, order=5):
  """
  Designs a Butterworth bandpass filter to isolate frequencies within a given range.
    
  Parameters:
  - lowcut (float): The low cutoff frequency of the filter.
  - highcut (float): The high cutoff frequency of the filter.
  - fs (float): The sampling frequency of the signal.
  - order (int): The order of the filter, which controls the steepness of the filter's response.

  Returns:
    - tuple: The filter coefficients (b, a).
  """
  nyquist = .5 * fs # Nyquist frequency (half of the sampling rate)
  low = lowcut / nyquist
  high = highcut / nyquist
  b, a = butter(order, [low, high], btype='band')
  # print("a:", a)
  # print("b:", b)
  return b, a

def bandpass_filter(data, lowcut, highcut, fs, order=3):
  """
  Applies a bandpass filter to the input signal to isolate a specific frequency range.
    
  Parameters:
  - data (array): The input signal to be filtered.
  - lowcut (float): The low cutoff frequency for the filter.
  - highcut (float): The high cutoff frequency for the filter.
  - fs (float): The sampling frequency of the signal.
    
  Returns:
  - array: The filtered signal containing only frequencies in the desired range.
  """
  # Get filter coefficients for the bandpass filter
  b, a = butter_bandpass(lowcut, highcut, fs, order)
  data = np.array(data)
  flattened = data.ravel()


  # Apply the filter to the signal
  return filtfilt(b=b, a=a,x=flattened, axis=0)

def update_led_brightness(strip, start, end, brightness_factor=1, passed_color=None):
  global is_static_color, capturing_with_avg, is_capt
  global strip_color
  for i in range(start, end):
    # if is_capt and not is_static_color:
    #   if not sc.in_update_loop:
    #     sound_scalars[i] = (brightness_factor, brightness_factor, brightness_factor)
    #     continue
    if is_static_color and not is_capt:
        color = strip_color
    elif capturing_with_avg:
      if passed_color is None:
        color = strip_color
      else:
        color = passed_color
        strip_color = color

    else: # we are capturing screen and in the capturing loop
      #  if sc.in_update_loop:
      #   r, g, b = current_led_colors[i]
      #   color = Color(r, g, b)
      #  else:
        color = strip.getPixelColor(i)

    if color is None:
      color = strip.getPixelColor(i)

    # Extract red, green, and blue components from the 24-bit color
    r = (color >> 16) & 0xFF
    g = (color >> 8) & 0xFF
    b = color & 0xFF

    # Apply brightness factor and ensure the values stay within range
    r = min(255, max(0, int(r * brightness_factor)))
    g = min(255, max(0, int(g * brightness_factor)))
    b = min(255, max(0, int(b * brightness_factor)))

    # sound_scalars[i] = (r, g, b)
    strip.setPixelColor(i, Color(r, g, b))
    
# Frequency bands for lows, mids, and highs
lowcut_lows = 50.0
highcut_lows = 250.0
lowcut_mids = 500.0
highcut_mids = 3000.0
lowcut_highs = 6000.0
highcut_highs = 17500.0

def run(indata, strip):
  """
  Processes the incoming audio data, applies frequency band filtering and smoothing, 
  then adjusts the brightness of LEDs based on the smoothed frequency content.
    
  Parameters:
  - indata (array): The input audio data to be processed.
  - strip (object): The LED strip object to update the brightness.
  """
  global l_count, r_count, t_count, b_count
  global past_high_rms, past_low_rms, past_mid_rms, decay_rate
  global capturing_with_avg
  fs = 48000 # sample rate

  # Check if the input data is valid (not empty)
  if indata is None or len(indata) == 0:
    return
  
  try:
    # if is_capt and not is_static_color:
    #   if not sc.in_update_loop:
    #     current_led_colors = list(sc.led_colors) # copy led_colors to reference them 
        # since it is not updating leds, led_colors = frame colors (keeps us updated)

    # Apply bandpass filters for each frequency band
    low_band = bandpass_filter(indata, lowcut_lows, highcut_lows, fs)
    #print("low band:", low_band)
    mid_band = bandpass_filter(indata, lowcut_mids, highcut_mids, fs)
    # print("mid band:", mid_band)
    high_band = bandpass_filter(indata, lowcut_highs, highcut_highs, fs)

    # Apply the Raised Cosine filter to smooth the low and high frequency signals
    low_freq_signal_smoothed = raised_cosine_filter(low_band)
    mid_freq_signal_smoothed = raised_cosine_filter(mid_band)
    high_freq_signal_smoothed = raised_cosine_filter(high_band)

    # Calculate RMS (Root Mean Square) for each frequency band after smoothing
    low_rms = np.sqrt(np.mean(low_freq_signal_smoothed**2))
    mid_rms = np.sqrt(np.mean(mid_freq_signal_smoothed**2))
    high_rms = np.sqrt(np.mean(high_freq_signal_smoothed**2))

    if past_low_rms is None:
      past_low_rms = low_rms
    if past_mid_rms is None:
      past_mid_rms = mid_rms
    if past_high_rms is None:
      past_high_rms = high_rms

    # Apply decay to RMS values
    low_rms = decay_rate * past_low_rms + (1 - decay_rate) * low_rms
    mid_rms = decay_rate * past_mid_rms + (1 - decay_rate) * mid_rms
    high_rms = decay_rate * past_high_rms + (1 - decay_rate) * high_rms
    # Store current RMS values for next pass
    past_low_rms = low_rms
    past_mid_rms = mid_rms
    past_high_rms = high_rms

    # Normalize RMS energy to a 0-1 range for ratio adjustment
    # Ratios represent the percentage of lights in each segment that light up
    def scale_rms(rms, low=50, high=2000):
      return max(0.0, min(1.0, (rms - low) / (high - low)))
    low_ratio = scale_rms(low_rms, 50, 5000)
    mid_ratio = scale_rms(mid_rms, 50, 3000)
    high_ratio = scale_rms(high_rms, 50, 800)

    # Adjust the LED ranges based on the ratios
    l_high = int(mid_ratio * l_count)
    r_high = int(mid_ratio * r_count)
    t_high = int(high_ratio * t_count)
    b_high = int(low_ratio * b_count)

    if is_static_color or capturing_with_avg:
      low_brightness_ratio = low_ratio
      mid_brightness_ratio = mid_ratio
      high_brightness_ratio = high_ratio
    else:
      low_brightness_ratio = 1
      mid_brightness_ratio =  1
      high_brightness_ratio =  1

    color = None
    if capturing_with_avg:
      color = sc.avg_color

    time_1 = time.time()
    # right
    end = r_count
    update_led_brightness(strip=strip, start=0, end=r_high, brightness_factor=mid_brightness_ratio, passed_color=color)
    update_led_brightness(strip=strip, start = r_high, end=end, brightness_factor=0)

    # top
    start = r_count
    end += t_count # r_count + t_count
    start_of_color =  start + int((t_count - t_high)/2)
    # top right black
    update_led_brightness(strip=strip, start=start, end=start_of_color, brightness_factor=0)
    update_led_brightness(strip=strip, start=start_of_color, end=start_of_color + t_high, brightness_factor=high_brightness_ratio, passed_color=color)
    update_led_brightness(strip=strip, start=start_of_color + t_high, end=end, brightness_factor=0)

    # left
    start += t_count # r_count + t_count
    end += l_count# r + t + l
    update_led_brightness(strip=strip, start=start, end=end-l_high, brightness_factor=0)
    update_led_brightness(strip=strip, start=end-l_high, end=end, brightness_factor=mid_brightness_ratio, passed_color=color)

    # bottom
    start += l_count # r + t + l
    end += b_count # r + t + l + b
    start_of_color = start + int((b_count - b_high)/2)

    # update_led_brightness(strip=strip, start=start, end=start + b_high//2, brightness_factor=low_brightness_ratio, passed_color=color)
    # update_led_brightness(strip=strip, start = start + b_high//2, end=end - b_high//2, brightness_factor=0)
    # update_led_brightness(strip=strip, start=end - b_high//2, end=end, brightness_factor=low_brightness_ratio, passed_color=color)

    update_led_brightness(strip=strip, start=start, end=start_of_color, brightness_factor=0)
    update_led_brightness(strip=strip, start=start_of_color, end=start_of_color + b_high, brightness_factor=low_brightness_ratio, passed_color=color)
    update_led_brightness(strip=strip, start=start_of_color + b_high, end=end, brightness_factor=0)

    time_2 = time.time()

    # print(f"time for seperated: {time_2 - time_1}")
    strip.show()
    # if not is_capt or capturing_with_avg:
    #   strip.show()
    # elif is_capt and not is_static_color and not sc.in_update_loop:
    #   strip.show()
    #   print("showing while not in loop")
    time.sleep(.003)

  except Exception as e:
    print(f"Error in seperated_equalizer: {e}")
    traceback.print_exc()  # Print the full traceback
