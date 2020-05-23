import time
import traceback

import numpy as np
from mayavi import mlab

from virtualreality.util.IMU import get_i2c_imu, get_coms_in_range, unit_vector
from squaternion import Quaternion

def demo_get_i2c_imu_orient_algo():
    uvw = np.asarray([[.1, 0, 0], [1, 0, 0], [0, .1, 0], [0, 1, 0], [0, 0, .1], [0, 0, 1]],
                     dtype=np.float64).transpose()

    obj = mlab.plot3d(uvw[0], uvw[1], uvw[2])

    @mlab.animate(delay=10)
    def run_get_i2c_imu_orient_algo():
        try:
            nonlocal uvw
            coms = get_coms_in_range()
            con, pro = get_i2c_imu(coms[0])
            pro.start()
            acc = (float(0),) * 3
            while True:
                grav = np.asarray(unit_vector(pro.protocol.imu.get_grav()))
                mag = np.asarray(unit_vector(pro.protocol.imu.get_mag()))
                east = unit_vector(np.cross(mag, grav))
                south = unit_vector(np.cross(east, grav))
                down = unit_vector(np.cross(south, east))
                xyz = np.asarray([east * .1, east, south * .1, south, down * .1, down],
                                 dtype=np.float64).transpose()
                if np.isnan(acc).any():
                    acc = (float(0),) * 3
                if not np.isnan(xyz).any():
                    obj.mlab_source.set(x=xyz[0], y=xyz[1], z=xyz[2])
                time.sleep(0)
                yield
        except:
            e = traceback.format_exc()
            print(e)
            print("ded")

    run_get_i2c_imu_orient_algo()
    mlab.show()


def test_get_i2c_imu():
    uvw = np.asarray([[.1, 0, 0], [1, 0, 0], [0, .1, 0], [0, 1, 0], [0, 0, .1], [0, 0, 1]],
                     dtype=np.float64).transpose()

    obj = mlab.plot3d(uvw[0], uvw[1], uvw[2])

    @mlab.animate(delay=10)
    def run_get_i2c_imu():
        try:
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
                    quvw = Quaternion(np.asarray([0]*6), *uvw)  # Behold! The multi-quaternion!
                    qxyz = orient1 * quvw * orient1.conjugate
                    xyz = qxyz.vector
                    obj.mlab_source.set(x=xyz[0], y=xyz[1], z=xyz[2])
                time.sleep(0)
                yield
        except:
            e = traceback.format_exc()
            print(e)
            print("ded")

    run_get_i2c_imu()
    mlab.show()


def test_get_i2c_imu_north_up():
    uvw = np.asarray([[.1, 0, 0], [1, 0, 0], [0, .1, 0], [0, 1, 0]],
                     dtype=np.float64).transpose()

    obj = mlab.plot3d(uvw[0], uvw[1], uvw[2])

    @mlab.animate(delay=10)
    def run_get_i2c_imu_north_up():
        try:
            nonlocal uvw
            coms = get_coms_in_range()
            con, pro = get_i2c_imu(coms[0])
            pro.start()
            acc = (float(0),) * 3
            while True:
                n, u = pro.protocol.imu.get_north_up()
                if np.isnan(acc).any():
                    acc = (float(0),) * 3
                if not np.isnan(n).any() and not np.isnan(u).any():
                    xyz = np.asarray([n * .1, n, u * .1, u],
                                     dtype=np.float64).transpose()
                    obj.mlab_source.set(x=xyz[0], y=xyz[1], z=xyz[2])
                time.sleep(0)
                yield
        except:
            e = traceback.format_exc()
            print(e)
            print("ded")

    run_get_i2c_imu_north_up()
    mlab.show()


if __name__ == '__main__':
    test_get_i2c_imu()
