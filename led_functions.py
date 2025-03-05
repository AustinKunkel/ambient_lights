from rpi_ws281x import PixelStrip, Color
import asyncio

import screen_capture as screen_capt
import sound_capture as sound_capt
import config

# Create NeoPixel object with appropriate configuration
strip = PixelStrip(config.LED_COUNT, config.LED_PIN, config.LED_FREQ_HZ, config.LED_DMA, config.LED_INVERT, config.LED_BRIGHTNESS, config.LED_CHANNEL)
# Initialize the library (must be called once before other functions)
strip.begin()

# Dictionary to store LED values
led_values = {"bri": 50, "col": "#FED403", "capt": 0, "srea": 0, "fx": 0, "cnt": 299, "id": 0}

# Store the current task globally
current_task = None
sound_effect_task = None

def hex_to_color(hex_color):
    """Convert hex color to Color object."""
    hex_color = hex_color.lstrip('#')
    return Color(int(hex_color[0:2], 16), int(hex_color[2:4], 16), int(hex_color[4:6], 16))

async def update_leds():
    """Update LED strip with current brightness and color values."""
    print("updated leds")
    global current_task
    global sound_effect_task
    global strip

    # Cancel any existing task
    await stop_curr_task()

    new_count = int(led_values["cnt"])
    if(config.LED_COUNT != new_count):
        strip.setBrightness(0)
        strip.show()
        await asyncio.sleep(.05)
        create_strip(new_count)
        config.LED_COUNT = new_count
        
    strip.setBrightness(config.LED_BRIGHTNESS)
    screen_capt.sound_capture = False

    # Create and await the new task based on led_values
    if int(led_values["capt"]) == 1:
        print("creating task: screen capture")
        current_task = asyncio.create_task(screen_capture())
        if int(led_values["srea"]) == 1:
            screen_capt.sound_capture = True
            print("starting sound react alongside screen capture")
            sound_effect_task = asyncio.create_task(sound_react())

    elif int(led_values["srea"]) == 1:
        print("Creating task: sound capture")
        sound_effect_task = asyncio.create_task(sound_react())
    elif int(led_values["fx"]) > 0:
        current_task = asyncio.create_task(show_fx())
    else:
        color = hex_to_color(led_values['col'])
        current_task = asyncio.create_task(show_color(color))

async def update_led_vars():
    config.LED_BRIGHTNESS = int(led_values['bri'])

    global strip
    strip.setBrightness(config.LED_BRIGHTNESS)
    strip.show()  

async def show_color(color):
    """Show a solid color on all LEDs."""
    try:
        for i in range(config.LED_COUNT):
            strip.setPixelColor(i,color)
        strip.show()
        return
    except asyncio.CancelledError:
        print("show_color() was cancelled")

async def show_fx():
    """Show a color effect on the LEDs."""
    try:
        for i in range(256):
            for j in range(config.LED_COUNT):
                strip.setPixelColor(j, Color(i, 0, 0))
            strip.show()
            await asyncio.sleep(0.02)  # Allow for other tasks to run
    except asyncio.CancelledError:
        print("show_fx was cancelled")

async def screen_capture():
    """Update LEDs to a specific color for screen capture."""
    try:
        led_values["col"] = "00ff11"
        await screen_capt.main(strip)
    except asyncio.CancelledError:
        print("Screen capture was cancelled")

async def sound_react():
    """Update LEDs to a specific color for sound reaction."""
    try:
        await sound_capt.main(strip)
    except asyncio.CancelledError:
        print("Cancelled sound react")

async def stop_curr_task():
    global current_task
    global sound_effect_task
    # If there is an existing task, cancel it
    if current_task:
        print("Cancelling current task")
        current_task.cancel()
        try:
            await current_task
        except asyncio.CancelledError:
            print("Previous LED update task was cancelled")
        current_task = None

    if sound_effect_task:
        print("Cancelling sound task")
        sound_effect_task.cancel()
        try:
            await sound_effect_task
        except asyncio.CancelledError:
            print("Sound Effect Task cancelled")
        sound_effect_task = None

def create_strip(led_count):
    global strip
    """Create and initialize the PixelStrip object."""
    strip = PixelStrip(led_count, config.LED_PIN, config.LED_FREQ_HZ, config.LED_DMA, config.LED_INVERT, 0, config.LED_CHANNEL)
    strip.begin()
    strip.show()
    return strip



