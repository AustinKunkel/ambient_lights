from rpi_ws281x import PixelStrip, Color
import asyncio

async def run(strip):
  try:

    for i in range(205):
      strip.setPixelColor(i,Color(255,0,0))
    strip.show()

    while True:
      print("sound capture")
      for i in range(256):
        strip.setBrightness(i)
        strip.show()
        await asyncio.sleep(.006)
  except asyncio.CancelledError:
    print("brightness on volume was cancelled")