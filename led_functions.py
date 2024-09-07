from rpi_ws281x import PixelStrip, Color
import asyncio

import screen_capture as capture_functions

# LED strip configuration
LED_COUNT = 299       # Number of LED pixels
LED_PIN = 18         # GPIO pin connected to the pixels (18 uses PWM)
LED_FREQ_HZ = 800000 # LED signal frequency in hertz (usually 800kHz)
LED_DMA = 10         # DMA channel to use for generating signal (try 10)
LED_BRIGHTNESS = 255 # Set to 0 for darkest and 255 for brightest
LED_INVERT = False   # True to invert the signal (when using NPN transistor level shift)
LED_CHANNEL = 0      # Set to 1 for GPIOs 13, 19, 41, 45 or 53

# Create NeoPixel object with appropriate configuration
strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
# Initialize the library (must be called once before other functions)
strip.begin()

# Dictionary to store LED values
led_values = {"bri": 50, "col": "#FED403", "capt": 0, "srea": 0, "fx": 0, "cnt": 299, "id": 0}

# Store the current task globally
current_task = None

def hex_to_color(hex_color):
    """Convert hex color to Color object."""
    hex_color = hex_color.lstrip('#')
    return Color(int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16))

async def update_leds():
    """Update LED strip with current brightness and color values."""
    print("updated leds")
    global current_task
    global LED_COUNT
    global strip

    # Cancel any existing task
    await stop_curr_task()

    new_count = int(led_values["cnt"])
    if(LED_COUNT != new_count):
        strip.setBrightness(0)
        strip.show()
        await asyncio.sleep(.05)
        create_strip(new_count)
        LED_COUNT = new_count
        
    strip.setBrightness(LED_BRIGHTNESS)

    # Create and await the new task based on led_values
    if int(led_values["capt"]) == 1:
        print("creating task: screen capture")
        current_task = asyncio.create_task(screen_capture())
    elif int(led_values["srea"]) == 1:
        current_task = asyncio.create_task(sound_react())
    elif int(led_values["fx"]) > 0:
        current_task = asyncio.create_task(show_fx())
    else:
        color = hex_to_color(led_values['col'])
        current_task = asyncio.create_task(show_color(color))

async def update_led_vars():

    global LED_BRIGHTNESS
    LED_BRIGHTNESS = int(led_values['bri'])

    global strip
    strip.setBrightness(LED_BRIGHTNESS)
    strip.show()  

async def show_color(color):
    """Show a solid color on all LEDs."""
    try:
        for i in range(LED_COUNT):
            strip.setPixelColor(i,color)
        strip.show()
        return
    except asyncio.CancelledError:
        print("show_color() was cancelled")

async def show_fx():
    """Show a color effect on the LEDs."""
    try:
        for i in range(256):
            for j in range(LED_COUNT):
                strip.setPixelColor(j, Color(i, 0, 0))
            strip.show()
            await asyncio.sleep(0.02)  # Allow for other tasks to run
    except asyncio.CancelledError:
        print("show_fx was cancelled")

async def screen_capture():
    """Update LEDs to a specific color for screen capture."""
    try:
        led_values["col"] = "00ff11"
        await capture_functions.main(strip)
    except asyncio.CancelledError:
        print("Screen capture was cancelled")

async def sound_react():
    """Update LEDs to a specific color for sound reaction."""
    try:
        led_values["col"] = "f700ff"
        await show_color()
    except asyncio.CancelledError:
        print("Sound react was cancelled")

async def stop_curr_task():
    global current_task
    # If there is an existing task, cancel it
    if current_task:
        current_task.cancel()
        try:
            await current_task
        except asyncio.CancelledError:
            print("Previous LED update task was cancelled")
        finally:
            current_task = None

def create_strip(led_count):
    global strip
    """Create and initialize the PixelStrip object."""
    strip = PixelStrip(led_count, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, 0, LED_CHANNEL)
    strip.begin()
    strip.show()
    return strip



