import numpy as np
from pytest import skip
from virtualreality.util.IMU import get_i2c_imu, get_coms_in_range


# @skip("Not an actual test yet")
def test_get_i2c_imu():
    coms = get_coms_in_range()
    con, pro = get_i2c_imu(coms[0])
    pro.start()
    orient1 = (float("nan"),) * 3
    while np.any(np.isnan(orient1)):
        orient1 = pro.protocol.imu.get_instant_orientation()
    while True:
        # print(f"acc: {pro.protocol.imu.get_acc(Quaternion.from_euler(3*pi/2, pi/4, 0))}")
        # pro.protocol.imu.get_grav(stationary=True)
        # print(f"grav:{pro.protocol.imu.grav_magnitude}")
        # print(f"gyro: {pro.protocol.imu.get_gyro()}")
        # print(f"mag: {pro.protocol.imu.get_mag()}")
        print(f"orient1: {orient1}")
        print(f"orient2: {pro.protocol.imu.get_instant_orientation(orient1)}")


if __name__ == "__main__":
    test_get_i2c_imu()
