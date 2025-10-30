# Ambient Light Project ReadMe

## Overview
Welcome to my DIY TV ambient light project! This project is designed to elevate your visual experience whether you're watching movies, playing games, listening to music, or more.

By using a Raspberry Pi Zero 2 W, an affordable capture card, an individually addressable LED strip, and a web-based interface, this project provides an immersive experience at a fraction of the cost. What would typically cost $150+ from commercial companies can be achieved for around $40.

## Features
- **Real-time Screen Capture**: Utilizes OpenCV and maps LED pixels directly to pixels on the screen.
- **Web-based Control**: Adjust settings via and interactive UI, accessible from any browser.
- **Custom LED and resolution mapping**: Configure LED positions and resolutions to fit any screen size.
- **Sound Reactive Mode**: Sync LEDs to audio with multiple different effects (can be paired with screen capture).
- **Low-Latency Updates**: Ensure minimal delay when updating LEDs, maintaining an immersive experience.

## Demonstration
Check out the project in action!

### 1. **The clip below shows the screen capture feature**

Settings: 
- Blend mode set to 3
- Capture Resolution set to 66w x 36h

*Show: Avatar, the Last Airbender*

https://github.com/user-attachments/assets/f56c9533-7821-40c1-be50-72bd195449db


### 2. **The next clip shows a sound effect, "Vertical Split Pulse Visualizer"**

Settings: 
- Capture Screen with average color
- sound react on, set to "Vertical Split Pulse Visualizer"

*Song: Posthumous Forgiveness - Tame Impala*

https://github.com/user-attachments/assets/005893cb-0b51-4522-89c6-22903486c89a


### 3. **The next clip shows another sound effect, "Seperated Visualizer"**

Settings:
- Capture Screen with average color
- sound react on, set to "Seperated Visualizer"

*Song: West Coast - Lana Del Rey*

https://github.com/user-attachments/assets/9dc6d823-4615-40ac-94d6-a4984a3fb36c

## How Screen Capture Works
### 1. **Capturing the Screen**
- Frames are captured from an HDMI capture card and read using OpenCV *(`cv2.VideoCapture`)*.
- Frames are continuously read to keep the LEDs updated in real time.
- The captured frames are then downscaled to reduce processing time and maintain color accuracy.
### 2. **Mapping LEDs to the Screen**
- During the setup phase, LED indices are mapped to regions of the screen based on the resolution and the number of LEDs along each edge.
### 3. **The Main Loop**
- In the main loop, the array of LED indices is traversed, with each LED index accessing the corresponding pixel in the frame.
- Each LED index directly corresponds to its position in the LED strip, and the strip’s color is updated based on the frame’s pixel color.
- The main loop runs in a seperate thread to avoid too much computation on one thread with asyncio, and to maintain fast updates
- If Blend Mode is enabled, each LED's color is calculated as the average of the colors within a range of (index ± blend depth) in the frame.
### 4. **Sending Data to LEDs**
- The **`rpi_ws281x`** library is used to update the LED strip.

### 5. **Optimizing for Low Latency**
- Asynchronous processing minimizes lag, and keeps the server responsive.
- Using arrays with direct indexing speeds up overall calculations.
- Other capture settings that the user can change can affect the latency (resolution, blend mode, LED count).

## How Sound Capture Works

## Future Plans & Roadmap

