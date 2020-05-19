import os
import cv2

rgby_tracking_image = os.path.join(os.path.dirname(__file__), "rgby_tracking_image.png")
rgby_tracking_image_np = cv2.imread(rgby_tracking_image)
