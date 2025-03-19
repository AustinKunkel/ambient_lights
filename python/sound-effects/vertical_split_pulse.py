from rpi_ws281x import Color
import numpy as np
import python.screen_capture as sc
import time
import python.led_functions as lf
from scipy.signal import butter, filtfilt, convolve
import traceback

NUM_CHANNELS = 2 # constant for number of audio channels (sound_capture.py reads)

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

INITIAL_PULSE_SPEED = 4 # initial pulse speed
LOWEST_PULSE_SPEED = .5 # lowest possible pulse speed
LEFT_PULSE_SPEED = 0
RIGHT_PULSE_SPEED = 0

left_iteration = 0
right_iteration = 0

pulse_list = []

left_pulse_list = []
right_pulse_list = []

def setup(strip):
  global l_count, r_count, t_count, b_count, LED_COUNT
  global is_static_color, strip_color, capturing_with_avg, is_capt
  global left_iteration, right_iteration

  l_count = int(sc.sc_settings["left-count"])
  r_count = int(sc.sc_settings["right-count"])
  t_count = int(sc.sc_settings["top-count"])
  b_count = int(sc.sc_settings["bottom-count"]) 

  LED_COUNT = l_count + r_count + t_count + b_count
  left_iteration = 0
  right_iteration = 0

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

  # Apply the filter separately to each channel
  return np.apply_along_axis(lambda x: np.convolve(x, rc_filter, mode='same'), axis=0, arr=signal)

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

  # Apply the filter to the signal
  return filtfilt(b, a, data, axis=0)

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
  color = None
  wraps_around = False
  has_wrapped = False

  def __init__(self, start_position, end_index, width, color, direction=1):
    """
    Parameters:
    - start_position: starting index in the led strip
    - end_index: index that the pulse goes to
    - width: how long the pulse is
    - color: the color of the pulse
    - direction: the multiplier (1 if forward, -1 if backward)
    """
    global LED_COUNT
    self.start_position = start_position
    self.end_index = end_index
    self.width = width
    self.direction = direction
    self.has_wrapped = False
    self.color = color

    self.end_position = max(0, min(LED_COUNT, self.start_position - (self.width * self.direction)))

    self.wraps_around = start_position > end_index and direction > 0

  def increment_position(self, speed):
    """
    Increments the positions inside the pulse
    
    Returns:
      boolean: if start_position is still valid
    """
    global LED_COUNT
    self.start_position += speed * self.direction

    self.start_position %= LED_COUNT

    self.end_position += speed * self.direction
    self.end_position %= LED_COUNT

    if(self.wraps_around and self.start_position <= self.end_index):
      self.has_wrapped = True

    if (self.wraps_around and self.has_wrapped) or not self.wraps_around:
      if(self.end_index - self.start_position) * self.direction <= 0:
        self.at_end = True # the head reached the end index
        self.start_position = self.end_index # if it hits its end_index, keep the start position there

    # If the end catches up to the start, return False
    if self.direction > 0:
      if(self.wraps_around):
        if(self.at_end): return self.end_position <= self.end_index
        return True
      return self.end_position <= self.start_position
    else:
      return self.end_position >= self.start_position

    is_still_valid = self.end_position <= self.start_position if (self.direction > 0) else self.end_position >= self.end_index
    is_still_valid = is_still_valid and not self.at_end

    return is_still_valid


  def draw(self, strip):
    """
    Draws it's current position on the strip.
    
    Parameters:
    - strip: The LED strip object
    - color (tuple): r, g, b values 
    """
    """
    if wrap_around, and start_position < end_position then we want to start at end_position, and end at LED_COUNT + start_position,
    but modulous on LED_COUNT in the loop
    """
    global LED_COUNT
    end =  int(self.start_position + (1 * self.direction))

    if(self.wraps_around and self.start_position < self.end_position):
      end = int(LED_COUNT + self.start_position)

    for i in range(int(self.end_position), end, int(self.direction)):
      i %= LED_COUNT
      current_r, current_g, current_b = getRGBFromColor(strip.getPixelColor(i))

      # simulate constructive interference
      r = int(min(255, current_r + self.color[0]))
      g = int(min(255, current_g + self.color[1]))
      b = int(min(255, current_b + self.color[2]))

      strip.setPixelColor(i, Color(r, g, b))

  def __str__(self):
    return f"head position: {self.start_position}, tail position: {self.end_position}, at_end: {self.at_end}, end_index: {self.end_index}, width: {self.width}, direction: {self.direction}, wraps around: {self.wraps_around}"

lowcut_lows = 50.0
highcut_lows = 250.0
lowcut_mids = 500.0
highcut_mids = 3000.0
lowcut_highs = 6000.0
highcut_highs = 17500.0

past_high_left_rms = None
past_high_right_rms = None

past_mid_left_rms = None
past_mid_right_rms = None

def inverse_square(x):
  global INITIAL_PULSE_SPEED, LOWEST_PULSE_SPEED
  return ((INITIAL_PULSE_SPEED - LOWEST_PULSE_SPEED) /((.25 * x)**2 + 1)) + LOWEST_PULSE_SPEED

def ease_in_out(x, b=10):
  """
  Ease-in-out function for brightness between the sides.
  Provide an index and it will calculate the ratio
  Desmos:
  y\ =\ \frac{\left(\frac{x}{b}\right)^{a}}{\left(\frac{x}{b}\right)^{a}+\left(1-\left(\frac{x}{b}\right)\right)^{a}}

  Parameters:
  - x: between [0, LED_COUNT]
  - b: domain that ease-in-out takes

  Returns: scalar [0, 1]
  """
  global LED_COUNT # represents the "period"
  a = 2.1
  if x >= 0 and x <= b:
    # between 0 and 10
    c = x / b
    denominator = c**a + (1 - c)**a
    return (c**a)/denominator
  if x > b and x < (LED_COUNT / 2): 
    # in the high spot
    return 1
  elif x >= (LED_COUNT / 2) and x < (LED_COUNT / 2) + b:
    # coming down
    x %= (LED_COUNT/2)
    c = x / b
    denominator = c**a + (1 - c)**a
    return 1 - ((c**a)/denominator)
  else:
    return 0

def run(indata, strip):
  """
  Processes the incoming audio data and creates a "pulse" effect 

  Parameters:
  - indata (array): The input audio data to be processed.
  - strip (object): The LED strip object to update the brightness.
  """
  global l_count, r_count, t_count, b_count, LED_COUNT, LEFT_PULSE_SPEED, RIGHT_PULSE_SPEED
  global past_mid_left_rms, past_mid_right_rms, past_high_left_rms, past_high_right_rms
  global capturing_with_avg, left_pulse_list, right_pulse_list, strip_color
  global left_iteration, right_iteration, strip_brightness_ratio
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

    # Filter the bands into the range of hz values
    low_band = bandpass_filter(indata, lowcut_lows, highcut_lows, fs)
    mid_band = bandpass_filter(indata, lowcut_mids, highcut_mids, fs)
    # high_band = bandpass_filter(indata, lowcut_highs, highcut_highs, fs)

    # Apply the Raised Cosine filter to smooth the low and high frequency signals
    low_freq_signal_smoothed = raised_cosine_filter(low_band)
    low_left = low_freq_signal_smoothed[:, 0] # left
    low_right = low_freq_signal_smoothed[:, 1] # right

    mid_freq_signal_smoothed = raised_cosine_filter(mid_band)
    mid_left = mid_freq_signal_smoothed[:, 0]
    mid_right = mid_freq_signal_smoothed[:, 1]

    # high_freq_signal_smoothed = raised_cosine_filter(high_band)
    # high_left = high_freq_signal_smoothed[:, 0]
    # high_right = high_freq_signal_smoothed[:, 1]

    # Calculate RMS (Root Mean Square) for each frequency band after smoothing
    low_left_rms = np.sqrt(np.mean(low_left**2))
    low_right_rms = np.sqrt(np.mean(low_right**2))

    mid_left_rms = np.sqrt(np.mean(mid_left**2))
    mid_right_rms = np.sqrt(np.mean(mid_right**2))

    # high_left_rms = np.sqrt(np.mean(high_left**2))
    # high_right_rms = np.sqrt(np.mean(high_right**2))

    if past_mid_left_rms is None:
      past_mid_left_rms = mid_left_rms
    if past_mid_right_rms is None:
      past_mid_right_rms = mid_right_rms

    # if past_high_left_rms is None:
    #   past_high_left_rms = high_left_rms
    # if past_high_right_rms is None:
    #   past_high_right_rms = high_right_rms

    # Apply decay to RMS values
    decay_rate = .8
    # low_rms = decay_rate * past_low_rms + (1 - decay_rate) * low_rms
    mid_left_rms = decay_rate * past_mid_left_rms + (1 - decay_rate) * mid_left_rms
    mid_right_rms = decay_rate * past_mid_right_rms + (1 - decay_rate) * mid_right_rms

    # past_high_left_rms = decay_rate * past_high_left_rms + (1 - decay_rate) * high_left_rms
    # past_high_left_rms = decay_rate * past_high_left_rms + (1 - decay_rate) * high_left_rms

    # Store current RMS values for next pass
    # past_low_rms = low_rms

    past_mid_left_rms = mid_left_rms
    past_mid_right_rms = mid_right_rms

    # past_high_left_rms = high_left_rms
    # past_high_right_rms = high_right_rms

    # Normalize RMS energy to a 0-1 range for ratio adjustment
    # Filters out the lower values
    def scale_rms(rms, low=50, high=2000, min_ratio=0.0):
      return (max(0,min(1.0, ((rms[0] - low) / (high - low))) + min_ratio), max(0,min(1.0, ((rms[1] - low) / (high - low)) + min_ratio)))
    low_left_ratio, low_right_ratio = scale_rms((low_left_rms, low_right_rms), 3500, 5000)
    mid_left_ratio, mid_right_ratio = scale_rms((mid_left_rms, mid_right_rms), 50, 3000, min_ratio=.3)
    # high_left_ratio, high_right_ratio = scale_rms((high_left_rms, high_right_rms), 50, 800)

    def calculate_width(ratio, min_ratio, max_width, min_width):
      n = max(0, min(1,(ratio - min_ratio)/(1 - min_ratio))) # keep n between 0 and 1
      return int(min_width + n * (max_width - min_width))

    left_iteration += 1
    right_iteration += 1

    top_middle = r_count + (t_count // 2)
    bottom_middle = r_count + t_count + l_count + (b_count // 2)
    
    if low_left_ratio > 0:
      calculated_iteration = calculate_width(low_left_ratio, 0.95, 0, 15)
      left_iteration = calculated_iteration if left_iteration > calculated_iteration else left_iteration
      width = calculate_width(low_left_ratio, .95, 12, 4)
      color = (int(r*low_left_ratio), int(g*.5*low_left_ratio), int(b*low_left_ratio*low_left_ratio))
      pulse_1 = Pulse(top_middle, end_index=r_count + t_count + (l_count // 2), width=width,color=color, direction=1)
      pulse_2 = Pulse(bottom_middle,r_count + t_count + (l_count // 2), width, color, -1)
      left_pulse_list.append(pulse_1)
      left_pulse_list.append(pulse_2)
    
    if low_right_ratio > 0:
      calculated_iteration = calculate_width(low_right_ratio, .95, 0, 15)
      right_iteration = calculated_iteration if right_iteration > calculated_iteration else right_iteration
      width = calculate_width(low_right_ratio, .95, 12, 4)
      color = (int(r*low_right_ratio), int(g*.5*low_right_ratio), int(b*low_right_ratio*low_right_ratio))
      pulse_1 = Pulse(top_middle, end_index=(r_count // 2), width=width,color=color, direction=-1)
      pulse_2 = Pulse(bottom_middle,(r_count // 2),width,color, 1)
      right_pulse_list.append(pulse_1)
      right_pulse_list.append(pulse_2)

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

    LEFT_PULSE_SPEED = inverse_square(left_iteration)
    RIGHT_PULSE_SPEED = inverse_square(right_iteration)

    # ensures that any pulse that reached the end is removed from the list
    left_pulse_list = [pulse for pulse in left_pulse_list if pulse.increment_position(LEFT_PULSE_SPEED)]
    right_pulse_list = [pulse for pulse in right_pulse_list if pulse.increment_position(RIGHT_PULSE_SPEED)]

    for pulse in left_pulse_list: 
      pulse.draw(strip)

    for pulse in right_pulse_list:
      pulse.draw(strip)

    """
    brightness_ratio = (mid_left_ratio + mid_right_ratio) / 2

    strip.setBrightness(min(255, max(30, int(brightness_ratio * 225))))
    """

    ease_in_out_length = 20 # used in ease_in_out()

    # starting indices for the ease-in-out function
    left_index = LED_COUNT - top_middle + (ease_in_out_length/2) # +(b/2) for a little crossover
    right_index = LED_COUNT - bottom_middle + (ease_in_out_length/2)

    for i in range(LED_COUNT):
      left_opacity_scalar = ease_in_out(left_index, ease_in_out_length)
      right_opacity_scalar = ease_in_out(right_index, ease_in_out_length)
      
      left_index = (left_index + 1) % (LED_COUNT) # ensure wrap around wth a little cross over
      right_index = (right_index + 1) % (LED_COUNT)

      combined_left = mid_left_ratio * left_opacity_scalar
      combined_right = mid_right_ratio * right_opacity_scalar

      r, g, b = getRGBFromColor(strip.getPixelColor(i))

      # if i >= bottom_middle - ease_in_out_length/2 and i <= bottom_middle + ease_in_out_length/2:
        # print(f"i: {i}\tleft index: {left_index} left opacity: {left_opacity_scalar} combined_left: {combined_left}\tright index: {right_index} right opacity: {right_opacity_scalar} combined_right: {combined_right}")

      r = int(min(255, r * (combined_left + combined_right)))
      g = int(min(255, g * (combined_left + combined_right)))
      b = int(min(255, b * (combined_left + combined_right)))

      strip.setPixelColor(i, Color(r, g, b))

    """
    # Lower value to smooth more
    alpha = .03
    strip_brightness_ratio = ((1 - alpha) * strip_brightness_ratio + alpha * mid_ratio)
    strip.setBrightness(max(20, int(strip_brightness_ratio * 120)))
    """
    strip.show()

    # time.sleep(.001)

  except Exception as e:
    print(f"Error in pulse visualizer: {e}")
    traceback.print_exc()
    
  # Keep only objects where is_valid() returns True
# objects = [obj for obj in objects if obj.is_valid()]

