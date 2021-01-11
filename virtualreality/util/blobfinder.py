from virtualreality.calibration.manual_color_mask_calibration import (
    CalibrationData,
    ColorRange,
)
from displayarray import display
import cv2
import numpy as np
from copy import copy


def reversed_enumerate(l):
    # thanks: https://stackoverflow.com/a/38738652
    return zip(range(len(l) - 1, 0, -1), reversed(l))


def mask_for_hsv_range(arr, colormask: ColorRange):
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


class BlobFinder(object):
    def __init__(self, color_range: ColorRange, draw_find: bool = False):
        self.color_range = color_range
        self.last_pos = None
        self.last_rad = None
        self.draw_find = draw_find

    def _get_crop_ends(self, arr, center, radius):
        w_min = int(min(max(0, (center[0] - radius)), arr.shape[0] - 2))
        w_max = int(min(max(w_min + 1, (center[0] + radius + 1)), arr.shape[0] - 1))
        h_min = int(min(max(0, (center[1] - radius)), arr.shape[1] - 2))
        h_max = int(min(max(h_min + 1, (center[1] + radius + 1)), arr.shape[1] - 1))
        return w_min, w_max, h_min, h_max

    def _center_cropped_find(
        self,
        arr,
        color_range,
        final_center,
        blob_radius,
        multiplier_to_get_full_img_size,
        blob_clearance,
        img_poss,
    ):
        w_min, w_max, h_min, h_max = self._get_crop_ends(
            arr,
            final_center,
            blob_radius * multiplier_to_get_full_img_size + blob_clearance,
        )

        avg_w = (w_min + w_max) // 2
        avg_h = (h_min + h_max) // 2
        crop_section_h = (slice(w_min, w_max), slice(avg_h, avg_h + 1), slice(None))
        crop_section_v = (slice(avg_w, avg_w + 1), slice(h_min, h_max), slice(None))

        cropph = arr[crop_section_h]
        croppv = arr[crop_section_v]
        mh = mask_for_hsv_range(cropph, color_range)
        mv = mask_for_hsv_range(croppv, color_range)
        blobh_locs_l = next((i for i, x in enumerate(mh) if x != 0), None)
        blobh_locs_r = next((i for i, x in reversed_enumerate(mh) if x != 0), None)
        blobv_locs_l = next((i for i, x in enumerate(mv[0]) if x != 0), None)
        blobv_locs_r = next((i for i, x in reversed_enumerate(mv[0]) if x != 0), None)
        if (
            any(
                [
                    blobh_locs_l is None,
                    blobh_locs_r is None,
                    blobv_locs_l is None,
                    blobv_locs_r is None,
                ]
            )
            or any(np.isnan([blobh_locs_l, blobh_locs_r, blobv_locs_l, blobv_locs_r]))
        ):
            return None
        img_pos_h = (blobh_locs_l + blobh_locs_r) / 2.0
        img_pos_v = (blobv_locs_l + blobv_locs_r) / 2.0
        img_pos = [img_pos_h, img_pos_v]
        ball_rad_1_px = np.linalg.norm(np.asarray([blobh_locs_l, img_pos_v]) - img_pos)
        ball_rad_2_px = np.linalg.norm(np.asarray([img_pos_h, blobv_locs_l]) - img_pos)
        ball_rad_px = max(ball_rad_1_px, ball_rad_2_px)

        img_center = np.asarray([avg_w - w_min, avg_h - h_min])
        img_diff = img_pos - img_center
        img_poss.append(img_diff)
        final_center = np.sum(np.asarray(img_poss), axis=0)

        # highlight ball
        if self.draw_find:
            arr[crop_section_h] //= 2
            arr[crop_section_v] //= 2
            arr[
                tuple([slice(int(x), int(x + 1)) for x in final_center] + [slice(None)])
            ] = 0
            arr[
                tuple([slice(int(x), int(x + 1)) for x in final_center] + [slice(2, 3)])
            ] = 255

            arr[tuple([int(w_min + blobh_locs_l), int(avg_h)] + [slice(None)])] = 0
            arr[tuple([int(w_min + blobh_locs_l), int(avg_h)] + [slice(1, 2)])] = 255
            arr[tuple([int(avg_w), int(h_min + blobv_locs_l)] + [slice(None)])] = 0
            arr[tuple([int(avg_w), int(h_min + blobv_locs_l)] + [slice(1, 2)])] = 255
            arr[tuple([int(w_min + blobh_locs_r), int(avg_h)] + [slice(None)])] = 0
            arr[tuple([int(w_min + blobh_locs_r), int(avg_h)] + [slice(1, 2)])] = 255
            arr[tuple([int(avg_w), int(h_min + blobv_locs_r)] + [slice(None)])] = 0
            arr[tuple([int(avg_w), int(h_min + blobv_locs_r)] + [slice(1, 2)])] = 255

        return final_center, ball_rad_px

    def find_blob(self, image):
        crop_section = None
        final_center = None
        blob_radius = None
        img_poss = []
        blob_clearance = 2

        if self.last_pos is not None and self.last_rad is not None:
            img_poss.append(self.last_pos)
            find = self._center_cropped_find(
                image,
                self.color_range,
                self.last_pos,
                self.last_rad * 2.0,
                1,
                blob_clearance,
                img_poss,
            )
            if find is not None:
                final_center, ball_rad_px = find
                self.last_pos = final_center
                self.last_rad = ball_rad_px

                return final_center, ball_rad_px
            else:
                crop_section = None
                blob_radius = None
                final_center = None
                self.last_pos = None
                self.last_rad = None
                img_poss = []

        shape = np.asarray(image.shape[:2][::-1])
        scales = int(np.min(np.log2(shape)))
        # smallest to largest, image pyramid
        planes = [
            cv2.resize(
                image, dsize=tuple(shape // (2 ** s)), interpolation=cv2.INTER_NEAREST
            )
            for s in reversed(range(scales + 1))
        ]

        for i, p in enumerate(planes):
            p: np.ndarray
            if crop_section is not None:
                cropp = p[(*crop_section, slice(None))]
            else:
                cropp = p
            m = mask_for_hsv_range(cropp, self.color_range)
            if np.max(m) > 0:
                blob_locs = np.argwhere(m)
                blob_radius = np.ceil(np.sqrt(len(blob_locs) / np.pi))
                # print(blob_size)
                img_pos = np.average(blob_locs, axis=0)
                multiplier_to_get_full_img_size = 2 ** (len(planes) - i - 1)
                if crop_section is None:
                    img_poss = [img_pos * multiplier_to_get_full_img_size]
                    final_center = img_pos
                if crop_section is not None:
                    img_center = np.asarray(m.shape[:2]) // 2
                    img_diff = img_pos - img_center
                    img_poss.append(img_diff * multiplier_to_get_full_img_size)
                    final_center = np.sum(np.asarray(img_poss), axis=0)

                    w_min, w_max, h_min, h_max = self._get_crop_ends(
                        image,
                        (final_center // multiplier_to_get_full_img_size) * 2,
                        (blob_radius + blob_clearance) * 2,
                    )
                else:
                    w_min, w_max, h_min, h_max = self._get_crop_ends(
                        image, img_pos * 2, (blob_radius + blob_clearance) * 2
                    )

                crop_section = [slice(w_min, w_max), slice(h_min, h_max)]

                # skip to last plane once we have enough knowledge about the location
                if blob_radius > 2 and crop_section is not None:

                    find = self._center_cropped_find(
                        image,
                        self.color_range,
                        final_center,
                        blob_radius,
                        multiplier_to_get_full_img_size,
                        blob_clearance,
                        img_poss,
                    )
                    if find is not None:
                        final_center, ball_rad_px = find
                    else:
                        continue

                    self.last_pos = final_center
                    self.last_rad = ball_rad_px

                    return final_center, ball_rad_px
            else:
                crop_section = None

        if final_center is not None and blob_radius is not None:
            self.last_pos = final_center
            self.last_rad = blob_radius
            return final_center, blob_radius
        else:
            self.last_pos = None
            self.last_rad = None
            return None

if __name__ == '__main__':
    b = BlobFinder(color_range=ColorRange(2, 126, 32, 192, 67, 209, 114))

    d = display(0, size=(9999, 9999))
    for up in d:
        if up:
            blob = b.find_blob(up["0"][0])
            if blob is not None:
                center, radius = blob
                print(f"center: {center}, radius: {radius}")
            else:
                print("blob not found. :(")
