from rpi_ws281x import Color
import numpy as np
import screen_capture as sc
import time
import led_functions as lf
from scipy.signal import butter, filtfilt
import traceback

NUM_CHANNELS = 1 # constant for number of audio channels (sound_capture.py reads)

l_count = int(sc.sc_settings["left-count"])
r_count = int(sc.sc_settings["right-count"])
t_count = int(sc.sc_settings["top-count"])
b_count = int(sc.sc_settings["bottom-count"])

is_static_color = False
strip_color = None
capturing_with_avg = False
is_capt = False

past_low_rms = None
past_mid_rms = None
past_high_rms = None
LED_COUNT = None
strip_brightness_ratio = 0

INITIAL_PULSE_SPEED = 6 # initial pulse speed
LOWEST_PULSE_SPEED = .5 # lowest possible pulse speed
PULSE_SPEED = 0
iteration = 0

pulse_list = []

def setup(strip):
  global l_count, r_count, t_count, b_count, LED_COUNT
  global is_static_color, strip_color, capturing_with_avg, is_capt
  global iteration

  l_count = int(sc.sc_settings["left-count"])
  r_count = int(sc.sc_settings["right-count"])
  t_count = int(sc.sc_settings["top-count"])
  b_count = int(sc.sc_settings["bottom-count"]) 

  LED_COUNT = l_count + r_count + t_count + b_count
  iteration = 0

  is_capt = int(lf.led_values['capt']) == 1
  is_fx = int(lf.led_values['fx']) == 1
  is_avg_color = int(sc.sc_settings['avg-color']) == 1
  print(f"capt: {is_capt}, fx: {is_fx}, avg: {is_avg_color}")

  # color doesnt change if there is no effects, 
  is_static_color = not is_capt and not is_fx

  # but should if its average color
  if is_capt and is_avg_color:
    capturing_with_avg = True
    is_static_color = True
  print(f"is static color: {is_static_color}")

  strip_color = lf.hex_to_color(lf.led_values['col'])
  if not strip_color:
    pixel_color = strip.getPixelColor(0)
    # Extract red, green, and blue components from the 24-bit color
    r, g, b = getRGBFromColor(pixel_color)
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

def getRGBFromColor(color):
  """Returns: (Tuple) of r value, g value, b value"""
  r = (color >> 16) & 0xFF
  g = (color >> 8) & 0xFF
  b = color & 0xFF
  return r, g, b

class Pulse:
  """
  Represents a single pulse for the sound

  Functions:
  - increment_position(): Increments the internal positions
  - draw(strip): draws the pulse on the strip
  """

  width = None
  start_position = None # may be decimals
  end_position = None
  direction = None # direction multiplier (1 for forward, -1 for backward)
  end_index = None
  at_end = False
  iteration = 0
  color = None

  def __init__(self, start_position, end_index, width, color, direction=1):
    """
    Parameters:
    - start_position (int): starting index in the led strip
    - end_index (int): index that the pulse goes to
    - width (int): how long the pulse is
    - color (tuple): the color of the pulse
    - direction: the multiplier (1 if forward, -1 if backward)
    """
    global LED_COUNT
    self.start_position = start_position
    self.end_index = end_index
    self.width = width
    self.direction = direction
    self.color = color

    self.end_position = max(0, min(LED_COUNT, self.start_position - (self.width * self.direction)))
    self.iteration = 1

  def increment_position(self):
    """
    Increments the positions inside the pulse
    
    Returns:
      boolean: if start_position is still valid
    """
    global LED_COUNT, PULSE_SPEED
    self.start_position += PULSE_SPEED * self.direction

    # if it is at the required length, or its at the end, increment end_position
    if abs(self.start_position - self.end_position) >= self.width or self.at_end:
      self.end_position += PULSE_SPEED * self.direction

    if(self.end_index - self.start_position) * self.direction < 0:
      self.at_end = True # the head reached the end index
      self.start_position = self.end_index # if it hits its end_index, keep the start position there

    self.iteration += 1
    # If the end catches up to the start, return False
    return self.start_position >= self.end_position if self.direction > 0 else self.start_position <= self.end_position


  def draw(self, strip):
    """
    Draws it's current position on the strip.
    
    Parameters:
    - strip: The LED strip object
    """
    
    for i in range(int(self.end_position), int(self.start_position + (1 * self.direction)), int(self.direction)):
      current_r, current_g, current_b = getRGBFromColor(strip.getPixelColor(i))

      # simulate constructive interference
      r = int(min(255, current_r + self.color[0]))
      g = int(min(255, current_g + self.color[1]))
      b = int(min(255, current_b + self.color[2]))

      strip.setPixelColor(i, Color(r, g, b))

  def __str__(self):
    return f"head position: {self.start_position}, tail position: {self.end_position}, at_end: {self.at_end}, end_index: {self.end_index}, width: {self.width}"

lowcut_lows = 50.0
highcut_lows = 250.0
lowcut_mids = 500.0
highcut_mids = 3000.0
lowcut_highs = 6000.0
highcut_highs = 17500.0


past_high_rms = None
past_mid_rms = None
past_low_rms = None

def inverse_square(x):
  global INITIAL_PULSE_SPEED, LOWEST_PULSE_SPEED
  return ((INITIAL_PULSE_SPEED - LOWEST_PULSE_SPEED) /((.15 * x)**2 + 1)) + LOWEST_PULSE_SPEED

def run(indata, strip):
  """
  Processes the incoming audio data and creates a "pulse" effect 

  Parameters:
  - indata (array): The input audio data to be processed.
  - strip (object): The LED strip object to update the brightness.
  """
  global l_count, r_count, t_count, b_count, LED_COUNT, PULSE_SPEED
  global past_high_rms, past_low_rms, past_mid_rms
  global capturing_with_avg, pulse_list, strip_color
  global iteration, strip_brightness_ratio
  fs = 48000 # sample rate

  # check if the input is valid (not empty)
  if indata is None or len(indata) == 0:
    return
  
  if capturing_with_avg: 
    new_color = sc.avg_color
    if new_color is not None:
      strip_color = new_color

  try:
    def wipe_colors(r, g, b):
      new_color = Color(int(r * .5), int(g * .5), int(b * .5))
      for i in range(LED_COUNT):
        strip.setPixelColor(i, new_color)

    r, g, b = getRGBFromColor(strip_color)
    wipe_colors(r, g, b)

    # Apply the Raised Cosine filter to smooth the low and high frequency signals
    low_band = bandpass_filter(indata, lowcut_lows, highcut_lows, fs)
    mid_band = bandpass_filter(indata, lowcut_mids, highcut_mids, fs)
    high_band = bandpass_filter(indata, lowcut_highs, highcut_highs, fs)

    # Calculate RMS (Root Mean Square) for each frequency band after smoothing
    low_freq_signal_smoothed = raised_cosine_filter(low_band)
    mid_freq_signal_smoothed = raised_cosine_filter(mid_band)
    high_freq_signal_smoothed = raised_cosine_filter(high_band)

    # Calculate RMS (Root Mean Square) for each frequency band after smoothing
    low_rms = np.sqrt(np.mean(low_freq_signal_smoothed**2))
    mid_rms = np.sqrt(np.mean(mid_freq_signal_smoothed**2))
    high_rms = np.sqrt(np.mean(high_freq_signal_smoothed**2))

    # if past_low_rms is None:
    #   past_low_rms = low_rms
    if past_mid_rms is None:
      past_mid_rms = mid_rms
    if past_high_rms is None:
      past_high_rms = high_rms

    # Apply decay to RMS values
    decay_rate = .9
    # low_rms = decay_rate * past_low_rms + (1 - decay_rate) * low_rms
    mid_rms = decay_rate * past_mid_rms + (1 - decay_rate) * mid_rms
    high_rms = decay_rate * past_high_rms + (1 - decay_rate) * high_rms

    # Store current RMS values for next pass
    # past_low_rms = low_rms
    past_mid_rms = mid_rms
    past_high_rms = high_rms

    # Normalize RMS energy to a 0-1 range for ratio adjustment
    # Filters out the lower values
    def scale_rms(rms, low=50, high=2000):
      return max(0.0, min(1.0, (rms - low) / (high - low)))
    low_ratio = scale_rms(low_rms, 3750, 5000)
    mid_ratio = scale_rms(mid_rms, 50, 3000)
    high_ratio = scale_rms(high_rms, 50, 800)

    def calculate_width(ratio, min_ratio, max_width, min_width):
      n = (ratio - min_ratio)/(1 - min_ratio)
      return int(min_width + n * (max_width - min_width))

    """
    if mid_ratio > 0 or high_ratio > 0:
      iteration += 1
    """

    iteration += 1

    if low_ratio > 0:
      calculated_iteration = calculate_width(low_ratio, 0.95, 0, 15)
      iteration = calculated_iteration if iteration > calculated_iteration else iteration
      width = calculate_width(low_ratio, .95, 16, 8)
      color = (int(r*low_ratio), int(g*.5*low_ratio), int(b*low_ratio*low_ratio))
      pulse_1 = Pulse((r_count + t_count - 1), end_index=0, width=width, color=color, direction=-1)
      pulse_2 = Pulse((r_count + t_count), LED_COUNT - 1,width, color, 1)
      pulse_list.append(pulse_1)
      pulse_list.append(pulse_2)

    """
    if mid_ratio > 0:
      width = calculate_width(mid_ratio, .95, 12, 4)
      mid_ratio = min(1, .3 + mid_ratio) # give it a lil boost, but max it at 1
      color = Color(int(r*.6 * mid_ratio), int(g*.3 * mid_ratio), int(b*.5 * mid_ratio))
      pulse_1 = Pulse((r_count + t_count - 1), 0, width, color, -1)
      pulse_2 = Pulse((r_count + t_count), LED_COUNT - 1, width, color, 1)
      pulse_list.append(pulse_1)
      pulse_list.append(pulse_2)

    if high_ratio > 0:
      width = calculate_width(high_ratio, .95, 8, 3)
      high_ratio = min(1, .3 + high_ratio) # give it a lil boost, but max it at 1
      color =  Color(int(r*.4 * high_ratio), int(g*.5 * high_ratio), int(b*.4* high_ratio))
      pulse_1 = Pulse((r_count + t_count - 1), 0, width, color, -1)
      pulse_2 = Pulse((r_count + t_count), LED_COUNT - 1, width, color, 1)
      pulse_list.append(pulse_1)
      pulse_list.append(pulse_2)

    """
    # ensures that any pulse that reached the end is removed from the list
    pulse_list = [pulse for pulse in pulse_list if pulse.increment_position()]

    PULSE_SPEED = inverse_square(iteration)

    # r = max(50, int(high_ratio * 205))
    # g = max(50, int(low_ratio * 205))
    # b = max(50, int(mid_ratio * 205))
    # color = (r, g, b)
    for pulse in pulse_list: 
      pulse.draw(strip)

    strip.setBrightness(min(255, max(50, int(mid_ratio * 205))))

    """
    # Lower value to smooth more
    alpha = .03
    strip_brightness_ratio = ((1 - alpha) * strip_brightness_ratio + alpha * mid_ratio)
    strip.setBrightness(max(20, int(strip_brightness_ratio * 120)))
    """
    strip.show()

    time.sleep(.005)

  except Exception as e:
    print(f"Error in pulse visualizer: {e}")
    traceback.print_exc()
    
  # Keep only objects where is_valid() returns True
# objects = [obj for obj in objects if obj.is_valid()]

