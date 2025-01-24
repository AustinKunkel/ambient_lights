import asyncio
import importlib


async def main(strip, effect_name):
  print("Starting sound effects")
  try:
    effect_module = importlib.import_module(f"sound-effects.brightness-on-volume")

    await effect_module.run(strip)
  except ModuleNotFoundError:
    print(f"Effect '{effect_name} was not found.")
    return None
  except asyncio.CancelledError:
      print("sound react was cancelled")
         