import numpy as np

smoothed_rms = None

def raised_cosine_filter(signal, alpha=0.5, num_taps=21):
    # Generate Raised Cosine filter coefficients
    t = np.linspace(-num_taps // 2, num_taps // 2, num_taps)
    rc_filter = np.sinc(t) * np.cos(np.pi * alpha * t) / (1 - (2 * alpha * t)**2)
    rc_filter /= np.sum(rc_filter)  # Normalize filter

    # Convolve the signal with the filter
    return np.convolve(signal, rc_filter, mode='same')

def run(indata, strip):
    global smoothed_rms
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
        normalized_volume = (smoothed_rms - 300) / 5700
        normalized_volume = max(0, min(normalized_volume, 1))
        brightness = int(10 + (normalized_volume * 120))

        # Update LEDs
        strip.setBrightness(brightness)
        strip.show()

    except Exception as e:
        print(f"Error in brightness on volume run method: {e}")
