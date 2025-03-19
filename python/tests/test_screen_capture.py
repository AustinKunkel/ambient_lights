import unittest
import screen_capture as sc

class testSetupFunctions(unittest.TestCase):
  def test_br_led_indexing(self):
    total = 160
    led_dict = {}
    count = total // 4
    w, h = 640, 480
    h_offset, v_offset = 0
    next_index = 0
    fwd_multiplier = 1
     # if it is bottom right, clockwise while counting backwards 
    next_index += count * fwd_multiplier
    sc.setup_right_side(count, led_dict, w, h, h_offset, next_index, -1)
    next_index += count * fwd_multiplier
    sc.setup_top_side(count, led_dict, w, v_offset, next_index, -1)
    next_index += count * fwd_multiplier
    sc.setup_left_side(count, led_dict, h, h_offset, next_index, -1)
    next_index+= count * fwd_multiplier
    sc.setup_bottom_side(count, led_dict, w, h, v_offset, next_index, -1)

    self.assertEqual(len(led_dict), count)

  def test_bl_led_indexing(self):
    total = 160
    led_dict = {}
    count = total // 4
    w, h = 640, 480
    h_offset, v_offset = 0
    next_index = 0
    fwd_multiplier = 1
    # it is bottom left, clockwise, unless its reversed
    sc.setup_left_side(count, led_dict, h, h_offset, next_index, 1)
    next_index += count * fwd_multiplier
    sc.setup_top_side(count, led_dict, w, v_offset, next_index, 1)
    next_index += count * fwd_multiplier
    sc.setup_right_side(count, led_dict, w, h, h_offset, next_index, 1)
    next_index += count * fwd_multiplier
    sc.setup_bottom_side(count, led_dict, w, h, v_offset, next_index, 1)
    
    self.assertEqual(len(led_dict), count)