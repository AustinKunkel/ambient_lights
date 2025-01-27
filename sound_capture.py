import asyncio
import importlib
import sounddevice as sd
import queue
import threading
import time

async def main(strip, effect_name):
  print("Starting sound effects")
  try:
    effect_module = importlib.import_module(f"sound-effects.brightness-on-volume")
    device = 0
    sample_rate = 48000
    chunk_size = 1024
    audio_queue = queue.Queue(maxsize=10)
    stop_event = threading.Event()

    def callback(indata, frames, time, status):
      # if status:
      #   print(f"Audio Stream Status: {status}")
      if not audio_queue.full():
        audio_queue.put(indata.copy())

    def start_audio_stream():
      """Thread to capture audio"""
      with sd.InputStream(
       device = device,
       channels=1,
       samplerate=sample_rate,
       callback=callback,
       blocksize=chunk_size,
       dtype='int16',
       latency= .01
      ):
        print("Audio cature started")
        while not stop_event.is_set():
          stop_event.wait(.01)

    def process_audio():
       while not stop_event.is_set():
          try: 
            indata = audio_queue.get(timeout=.01)
            effect_module.run(indata, strip)
            time.sleep(.01)
          except Exception as e:
            print(f"Error with effects: {e}")
          
    # Thread that reads the audio stream and adds the data to the queue
    audio_thread = threading.Thread(target=start_audio_stream, daemon=True)
    audio_thread.start()

    # The processing thread that handles the effects
    processing_thread = threading.Thread(target=process_audio, daemon=True)
    processing_thread.start()

    while True:
      pass

  except ModuleNotFoundError:
    print(f"Effect '{effect_name} was not found.")
    return None
  except asyncio.CancelledError:
    print("sound react was cancelled")
    raise
  except Exception as e:
    print(f"Error in sound capture: {e}")
  finally:
    stop_event.set()
    if audio_thread.is_alive():
      audio_thread.join()
    if processing_thread.is_alive():
      processing_thread.join()
    print("Audio capture has been stopped")
         