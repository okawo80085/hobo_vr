import numpy as np
from displayarray import display


def convolve_1d(image, kernel):
    kw = kernel.shape[0]
    kh = kernel.shape[1]

    out_vert = np.zeros_like(image)
    pad_image = np.pad(image, 1, mode='wrap')

    for x in range(out_vert.shape[0]):
        for y in range(out_vert.shape[1]):
            out_vert[x, y] = (pad_image[x:x + kw, y:y + kh] * kernel).sum()
    return out_vert


disp = display(0, size=(1, 1))
for d in disp:
    if d:
        frame = d['0'][0]

        # could be yaw or x motion
        vertical_line = np.mean(frame, 1)
        kernel = np.asarray([[-.5, 1, -.5]] * 3)
        vertical_line = convolve_1d(vertical_line, kernel)
        vertical_line = ((vertical_line + 256.0) / 2.0).astype(np.uint8)
        disp.display_frames([vertical_line[:, np.newaxis, :]], 1, 'hi')

        # could be pitch or y motion
        horizontal_line = np.mean(frame, 0)
        horizontal_line = convolve_1d(horizontal_line, kernel)
        horizontal_line = ((horizontal_line + 256.0) / 2.0).astype(np.uint8)
        disp.display_frames([horizontal_line[np.newaxis, :, :]], 2, 'hi2')

        # z motion and roll separately
        dx = int(frame.shape[0] / 2.0)
        dy = int(frame.shape[1] / 2.0)
        radius = min(dx, dy)
        rad = 2.0 * np.pi
        circles_to_line = np.zeros((radius, 3))
        rad_to_line = np.zeros((int(rad * radius), 3))
        # todo: go 2 4 6, or 4 8 12, etc., if framerate slows down, for both r and theta
        for r in range(circles_to_line.shape[0]):
            circ_pix = np.zeros(3)
            circ = rad * r
            for t in range(1, int(circ)+1):
                theta = (t / circ) * rad

                x = r * np.cos(theta) + dx
                y = r * np.sin(theta) + dy
                circ_pix[:] += frame[int(x), int(y)]
                rad_to_line[int(((t-1) / circ)*rad * radius)] += frame[int(x), int(y)]
            if circ:
                circles_to_line[r] = circ_pix / circ
        for t in range(int(rad * radius)):
            rad_to_line[t] /= radius

        rad_to_line = convolve_1d(rad_to_line, kernel)
        rad_to_line = ((rad_to_line + 256.0) / 2.0).astype(np.uint8)

        circles_to_line = convolve_1d(circles_to_line, kernel)
        circles_to_line = ((circles_to_line + 256.0) / 2.0).astype(np.uint8)

        disp.display_frames([rad_to_line[np.newaxis, :, :]], 3, 'theta')
        disp.display_frames([circles_to_line[np.newaxis, :, :]], 4, 'radius')
