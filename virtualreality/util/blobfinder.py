from virtualreality.calibration.manual_color_mask_calibration import CalibrationData, ColorRange
from displayarray import display
import cv2
import numpy as np
from copy import copy


def mask_for_hsv_range(arr, colormask:ColorRange):
    hsv = cv2.cvtColor(arr, cv2.COLOR_BGR2HSV)

    color_low = [
        int(colormask.hue_center - colormask.hue_range),
        int(colormask.sat_center - colormask.sat_range),
        int(colormask.val_center - colormask.val_range),
    ]

    color_high = [
        int(colormask.hue_center + colormask.hue_range),
        int(colormask.sat_center + colormask.sat_range),
        int(colormask.val_center + colormask.val_range),
    ]

    color_low_neg = copy(color_low)
    color_high_neg = copy(color_high)
    for c in range(3):
        if c == 0:
            c_max = 180
        else:
            c_max = 255
        if color_low_neg[c] < 0:
            color_low_neg[c] = c_max + color_low_neg[c]
            color_high_neg[c] = c_max
            color_low[c] = 0
        elif color_high_neg[c] > c_max:
            color_low_neg[c] = 0
            color_high_neg[c] = color_high_neg[c] - c_max
            color_high[c] = c_max
    mask1 = cv2.inRange(hsv, tuple(color_low), tuple(color_high))
    mask2 = cv2.inRange(hsv, tuple(color_low_neg), tuple(color_high_neg))
    mask = cv2.bitwise_or(mask1, mask2)
    return mask

crange = ColorRange(2, 126, 32, 192, 67, 209, 114)

import time

t0 = t1 = time.time()

def find_blob(arr):
    t0 = time.time()
    shape = np.asarray(arr.shape[:2][::-1])
    scales = int(np.min(np.log2(shape)))
    # smallest to largest, image pyramid
    planes = [cv2.resize(arr, dsize=tuple(shape//(2**s)), interpolation=cv2.INTER_NEAREST)
              for s in reversed(range(scales+1))]

    crop_section = None
    final_center = None
    img_poss = []
    img_pos = None
    for i, p in enumerate(planes):
        p:np.ndarray
        if crop_section is not None:
            cropp = p[(*crop_section, slice(None))]
        else:
            cropp = p
        m = mask_for_hsv_range(cropp, crange)
        blob_clearance=1
        if np.max(m)>0:
            blob_locs = np.argwhere(m)
            blob_size = np.ceil(2.0*np.sqrt(len(blob_locs)/np.pi))
            #print(blob_size)
            img_pos = np.average(blob_locs, axis=0)
            multiplier_to_get_full_img_size = 2**(len(planes)-i-1)
            if crop_section is None:
                img_poss.append(img_pos*multiplier_to_get_full_img_size)
                final_center = img_pos
            if crop_section is not None:
                img_center = np.asarray(m.shape[:2])//2
                img_diff = img_pos - img_center
                img_poss.append(img_diff*multiplier_to_get_full_img_size)
                final_center = np.sum(np.asarray(img_poss), axis=0)
                w_min = int(max(0,(final_center[0]//multiplier_to_get_full_img_size - blob_size - blob_clearance) * 2))
                w_max = int(max(w_min+1,(final_center[0]//multiplier_to_get_full_img_size + blob_size + blob_clearance) * 2))
                h_min = int(max(0,(final_center[1]//multiplier_to_get_full_img_size - blob_size - blob_clearance) * 2))
                h_max = int(max(h_min+1,(final_center[1]//multiplier_to_get_full_img_size + blob_size + blob_clearance) * 2))
                crop_section = [
                    slice(w_min,w_max),
                    slice(h_min,h_max)
                ]
            else:
                w_min = int(max(0,(img_pos[0]-blob_size-blob_clearance)*2))
                w_max = int(max(w_min+1,(img_pos[0]+blob_size+blob_clearance)*2))
                h_min = int(max(0,(img_pos[1] - blob_size - blob_clearance) * 2))
                h_max = int(max(h_min+1,(img_pos[1] + blob_size + blob_clearance) * 2))
                crop_section = [
                    slice(w_min, w_max),
                    slice(h_min, h_max)
                ]
    if final_center is not None:
        image_percent = final_center / np.asarray(arr.shape[:2])
        print("{:.4f}, {:.4f}".format(float(image_percent[0]), float(image_percent[1])), end='\r')
    else:
        print("Blob not found. :(", end='\r')
    t1 = time.time()
    print(f'latency: {(t1-t0)*1000.0}ms')
    return m


for up in display(1, size=(640, 480), callbacks=[find_blob]):
    if up:
        '''t1 = time.time()
        if t1 == t0:
            print('inf')
        else:
            print(1.0 / (t1 - t0))
        t0 = t1'''
    else:
        pass