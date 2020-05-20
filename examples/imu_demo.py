import time

import numpy as np
from mayavi import mlab
from squaternion import Quaternion

from virtualreality.util.IMU import get_i2c_imu, get_coms_in_range


def test_get_i2c_imu():
    uvw = np.asarray([[.1, 0, 0], [1, 0, 0], [0, .1, 0], [0, 1, 0], [0, 0, .1], [0, 0, 1]],
                     dtype=np.float64).transpose()

    obj = mlab.plot3d(uvw[0], uvw[1], uvw[2])

    @mlab.animate(delay=10)
    def run_get_i2c_imu():
        nonlocal uvw
        coms = get_coms_in_range()
        con, pro = get_i2c_imu(coms[0])
        pro.start()
        acc = (float(0),) * 3
        while True:
            orient1 = pro.protocol.imu.get_orientation()
            if np.isnan(acc).any():
                acc = (float(0),) * 3
            if not np.isnan(orient1).any():
                orient = Quaternion.from_euler(*orient1)
                rot = np.array(orient.to_rot())
                xyz = np.matmul(rot, uvw)
                obj.mlab_source.set(x=xyz[0], y=xyz[1], z=xyz[2])
            time.sleep(0)
            yield

    run_get_i2c_imu()
    mlab.show()


if __name__ == '__main__':
    test_get_i2c_imu()
