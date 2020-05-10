import cv2
import numpy as np
import time
import imutils
from imutils.video import VideoStream
import math as m

hc, hr = 98, 10
sc, sr = 200, 55
vc, vr = 250, 32

orangeHigh = [hc + hr, sc + sr, vc + vr]
orangeLow = [hc - hr, sc - sr, vc - vr]

camera_focal_length = 490
frame_width, frame_height = 640, 480

BALL_RADIUS_CM = 2

vs = cv2.VideoCapture(4)

time.sleep(2)


# vs.set(10, 10)
# vs.set(11, 30)
# vs.set(21, 0)

iX, iY = -1, -1


def get_colors(event, x, y, flags, param):
    global iX, iY

    if event == cv2.EVENT_LBUTTONDBLCLK:
        print(x, y)
        iX, iY = x, y


cv2.namedWindow("hentai3")
cv2.setMouseCallback("hentai3", get_colors)

while 1:
    ret, frame = vs.read()

    if frame is None:
        break

    # frame = imutils.resize(frame, width=600)

    blurred = cv2.GaussianBlur(frame, (3, 3), 0)

    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    mask = cv2.inRange(hsv, tuple(orangeLow), tuple(orangeHigh))
    # mask = cv2.erode(mask, None, iterations=1)
    # mask = cv2.dilate(mask, None, iterations=3)

    _, cnts, hr = cv2.findContours(
        mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )
    # cnts = imutils.grab_contours(cnts)

    # if len(cnts) > 0:
    # c = max(cnts, key=cv2.contourArea)
    # if len(c) >= 5:
    # 	elip = cv2.fitEllipse(c)

    # 	x, y = elip[0]
    # 	w, h = elip[1]

    # 	f_px = camera_focal_length
    # 	X_px = x
    # 	Y_px = y
    # 	A_px = w/2

    # 	X_px -= frame_width/2
    # 	Y_px = frame_height - Y_px

    # 	L_px = np.sqrt(X_px**2 + Y_px**2)

    # 	k = L_px / f_px

    # 	j = (L_px + A_px) / f_px

    # 	l = (j - k) / (1+j*k)

    # 	D_cm = BALL_RADIUS_CM * np.sqrt(1+l**2)/l

    # 	fl = f_px/L_px

    # 	Z_cm = D_cm * fl/np.sqrt(1+fl**2)

    # 	L_cm = Z_cm*k

    # 	X_cm = L_cm * X_px/L_px
    # 	Y_cm = L_cm * Y_px/L_px

    # 	X_cm /= 100
    # 	Y_cm /= 100
    # 	Z_cm /= 100

    # 	print ('{: <15.5f} {: <15.5f} {: <15.5f}'.format(X_cm, Y_cm, Z_cm))

    # cv2.drawContours(frame, [c], -1, (255, 255, 255), 3)

    # frame[:, :, 0] *= mask
    # frame[:, :, 1] *= mask
    # frame[:, :, 2] *= mask

    cv2.imshow("hentai", hsv)
    cv2.imshow("hentai2", mask)
    cv2.imshow("hentai3", frame)

    k = cv2.waitKey(10) & 0xFF

    print(hsv[iY, iX, :])

    # if k == ord('i'):
    # 	orangeHigh[0] += 1
    # 	# orangeLow[0] += 1

    # if k == ord('k'):
    # 	orangeHigh[0] -= 1
    # 	orangeLow[0] -= 1

    # if k == ord('j'):
    # 	orangeHigh[1] += 1
    # 	# orangeLow[1] += 1

    # if k == ord('l'):
    # 	orangeHigh[1] -= 1
    # 	# orangeLow[1] -= 1

    # if k == ord('u'):
    # 	orangeHigh[2] += 1
    # 	# orangeLow[2] += 1

    # if k == ord('o'):
    # 	orangeHigh[2] -= 1
    # 	# orangeLow[2] -= 1

    if k == ord("q"):
        break

    # print (orangeLow, orangeHigh)

vs.release()
cv2.destroyAllWindows()
