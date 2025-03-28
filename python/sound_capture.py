import asyncio
import importlib.util
import sounddevice as sd
import queue
import threading
import time
import traceback
import json
effect_name = "Brightness on Volume"

async def main(strip):
  global effect_name
  print("Starting sound effects")
  try:
    effect_filepath = 'sound-effects/' + await get_effect_filename()
    spec = importlib.util.spec_from_file_location("effect_module", effect_filepath)
    effect_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(effect_module)

    if hasattr(effect_module, 'setup'):
      effect_module.setup(strip)

    if hasattr(effect_module, 'NUM_CHANNELS'):
      num_channels = effect_module.NUM_CHANNELS
    else:
      num_channels = 1

    if not hasattr(effect_module, 'run'):
      raise ImportError(f"Module '{effect_name}' does not have a 'run' method.")
    device = 0
    sample_rate = 48000
    chunk_size = 512
    audio_queue = queue.Queue(maxsize=5)
    stop_event = threading.Event()

    def callback(indata, frames, time, status):
      if not audio_queue.full():
        audio_queue.put(indata.copy())

    def start_audio_stream():
      """Thread to capture audio"""
      with sd.InputStream(
       device = device,
       channels=num_channels,
       samplerate=sample_rate,
       callback=callback,
       blocksize=chunk_size,
       dtype='int16',
       latency=None
      ):
        print("Audio capture started")
        while not stop_event.is_set():
          stop_event.wait(.01)

    def process_audio():
       while not stop_event.is_set():
          try: 
            time.sleep(.01)
            indata = audio_queue.get(timeout=.05)
            effect_module.run(indata, strip)
          except Exception as e:
            print(f"Error with effects: {type(e).__name__} - {e}")
            traceback.print_exc()  # Print the full traceback
          
    # Thread that reads the audio stream and adds the data to the queue
    audio_thread = threading.Thread(target=start_audio_stream, daemon=True)
    audio_thread.start()

    time.sleep(.25)

    # The processing thread that handles the effects
    processing_thread = threading.Thread(target=process_audio, daemon=True)
    processing_thread.start()

    while not stop_event.is_set():
      await asyncio.sleep(.1)

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

async def get_effect_filename():
  global effect_name
  effect_config_path ='json/sound-effects.json'

  # effect_config_path = os.path.normpath(effect_config_path)

  try:
    with open(effect_config_path, 'r') as file:
      data = json.load(file)
      return data.get(effect_name)
  except FileNotFoundError:
    print(f"Could not find the file: {effect_config_path}")
  except Exception as e:
    print(f"Exception occurred when trying to get sound effect file path: {e}")
  except asyncio.CancelledError:
    pass
         