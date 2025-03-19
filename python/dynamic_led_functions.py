import asyncio
import time
import cv2
import numpy as np
import threading
import queue
from python.config import LED_COUNT

from rpi_ws281x import Color

sound_scalars = [(1,1,1)] * LED_COUNT # list with LED index, value: Tuple(r, g, b) scalar (0, 1)
led_colors = [(0,0,0)] * LED_COUNT #List with index as LED index, value: Tuple(r, g, b)

stop_event = threading.Event()

synchronized_thread_running = False

