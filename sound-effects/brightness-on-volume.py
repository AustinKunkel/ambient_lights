import numpy as np
import time

smoothed_rms = None
max_volume = 7000 # the max volume we want to create the ratio from
min_volume = 300 # the minimum volume we want to read from

def raised_cosine_filter(signal, alpha=0.5, num_taps=21):
    # Generate Raised Cosine filter coefficients
    t = np.linspace(-num_taps // 2, num_taps // 2, num_taps)
    rc_filter = np.sinc(t) * np.cos(np.pi * alpha * t) / (1 - (2 * alpha * t)**2)
    rc_filter /= np.sum(rc_filter)  # Normalize filter

    # Convolve the signal with the filter
    return np.convolve(signal, rc_filter, mode='same')

def run(indata, strip):
    global smoothed_rms, max_volume, min_volume
    alpha = .2

    if indata is None or len(indata) == 0:
        return

    try:
        indata = indata.astype(np.float32)

        # Apply Raised Cosine filter to smooth the signal
        filtered_signal = raised_cosine_filter(indata.flatten())

        # Calculate RMS from the filtered signal
        rms = np.sqrt(np.mean(filtered_signal**2))

        # Apply EMA smoothing on top
        if smoothed_rms is None:
            smoothed_rms = rms
        else:
            smoothed_rms = (1 - alpha) * smoothed_rms + alpha * rms

        # Normalize and calculate brightness
        normalized_volume = (smoothed_rms - min_volume) / (max_volume - min_volume)
        normalized_volume = max(0, min(normalized_volume, 1))
        brightness = int(10 + (normalized_volume * 200))

        # Update LEDs
        strip.setBrightness(brightness)
        strip.show()
        time.sleep(.01)

    except Exception as e:
        print(f"Error in brightness on volume run method: {e}")
