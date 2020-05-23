import time

import numpy as np
import serial
import serial.threaded
from pyrr import Quaternion
from serial import SerialException

from virtualreality.util import utilz as u


def unit_vector(vector):
    """ Returns the unit vector of the vector.  """
    # https://stackoverflow.com/a/13849249/782170
    return vector / np.linalg.norm(vector)


def angle_between(v1, v2):
    """ Returns the angle in radians between vectors 'v1' and 'v2'::

            >>> angle_between((1, 0, 0), (0, 1, 0))
            1.5707963267948966
            >>> angle_between((1, 0, 0), (1, 0, 0))
            0.0
            >>> angle_between((1, 0, 0), (-1, 0, 0))
            3.141592653589793
    """
    # https://stackoverflow.com/a/13849249/782170
    v1_u = unit_vector(v1)
    v2_u = unit_vector(v2)
    return np.arccos(np.clip(np.dot(v1_u, v2_u), -1.0, 1.0))


def perpendicular_vector(v):
    r""" Finds an arbitrary perpendicular vector to *v*."""
    # https://codereview.stackexchange.com/a/43937
    # for two vectors (x, y, z) and (a, b, c) to be perpendicular,
    # the following equation has to be fulfilled
    #     0 = ax + by + cz

    # x = y = z = 0 is not an acceptable solution
    if v.x == v.y == v.z == 0:
        raise ValueError("zero-vector")

    # If one dimension is zero, this can be solved by setting that to
    # non-zero and the others to zero. Example: (4, 2, 0) lies in the
    # x-y-Plane, so (0, 0, 1) is orthogonal to the plane.
    if v.x == 0:
        return np.asarray([1, 0, 0])
    if v.y == 0:
        return np.asarray([0, 1, 0])
    if v.z == 0:
        return np.asarray([0, 0, 1])

    # arbitrarily set a = b = 1
    # then the equation simplifies to
    #     c = -(x + y)/z
    return np.asarray([1, 1, -1.0 * (v.x + v.y) / v.z])


def from_to_quaternion(v1, v2, is_unit=True, epsilon=1e-5):
    # https://stackoverflow.com/a/11741520/782170
    if not is_unit:
        v1 = unit_vector(v1)
        v2 = unit_vector(v2)

    k_cos_theta = np.dot(v1, v2)
    k = np.sqrt(np.dot(v1, v1) * np.dot(v2, v2))

    if abs(k_cos_theta / k + 1) < epsilon:
        # 180 degree rotation around any orthogonal vector
        return Quaternion(0, unit_vector(perpendicular_vector(v1)))

    q = Quaternion(k_cos_theta + k, *np.cross(v1, v2))
    return q.normalize


class IMU(object):
    epsilon = 1e-5

    def __init__(self):
        self._acc_x = 0.0
        self._acc_y = 0.0
        self._acc_z = 0.0
        self._gyro_x = 0.0
        self._gyro_y = 0.0
        self._gyro_z = 0.0
        self._mag_x = 0.0
        self._mag_y = 0.0
        self._mag_z = 0.0
        self.grav_magnitude = 14  # I have no idea why gravity is 14 on my imu, but it is
        self._mag_iron_offset_x = 23.5
        self._mag_iron_offset_y = 23.5
        self._mag_iron_offset_z = -45

    def get_north_west_up(self, expected_orientation=None, grav_magnitude=None, stationary=False, accel=None):
        grav = np.asarray(unit_vector(self.get_grav(expected_orientation, grav_magnitude, stationary, accel)))
        mag = unit_vector(np.asarray(self.get_mag()))
        east = unit_vector(np.cross(mag, grav))
        north = -unit_vector(np.cross(east, grav))
        up = -unit_vector(np.cross(north, east))

        return north, -east, up

    def get_orientation(self, expected_orientation=None, grav_magnitude=None, stationary=False, accel=None):
        """Gets orientation quaternion"""
        north, west, up = self.get_north_west_up()
        rot = np.asarray([north, west, up]).transpose()
        q = Quaternion.from_matrix(rot)
        return q

    def get_grav(self, q=None, magnitude=None, stationary=False, accel=None):
        if stationary or ((accel is not None) and (not np.any(accel))) or q is None:
            g = [self._acc_x, self._acc_y, self._acc_z]
            self.grav_magnitude = sum(abs(np.asarray(g)))
            return g
        elif q is not None:
            if not isinstance(q, Quaternion):
                q = Quaternion.from_euler(*q)
            if magnitude is None:
                magnitude = self.grav_magnitude
            # http://www.varesano.net/blog/fabio/simple-gravity-compensation-9-dom-imus
            g = [0.0, 0.0, 0.0]

            # get expected direction of gravity
            g[0] = q.x * magnitude
            g[1] = q.y * magnitude
            g[2] = q.z * magnitude

            return g
        elif accel is not None:
            g = np.asarray([self._acc_x, self._acc_y, self._acc_z])
            g -= accel
            self.grav_magnitude = sum(abs(np.asarray(g)))
            return g

    def get_acc(self, q=None):
        g = self.get_grav(q)
        # compensate accelerometer readings with the expected gravity
        return [self._acc_x - g[0], self._acc_y - g[1], self._acc_z - g[2]]

    def get_gyro(self):
        return [self._gyro_x, self._gyro_y, self._gyro_z]

    def set_mag_iron_offset(self, offset):
        assert len(offset) == 3
        self._mag_iron_offset_x = offset[0]
        self._mag_iron_offset_x = offset[1]
        self._mag_iron_offset_x = offset[2]

    def get_mag(self):
        mag = [
            self._mag_x - self._mag_iron_offset_x,
            self._mag_y - self._mag_iron_offset_y,
            self._mag_z - self._mag_iron_offset_z,
        ]
        return mag


class PureIMUProtocol(serial.threaded.Packetizer):
    TERMINATOR = b"\n"

    def __init__(self):
        super().__init__()
        self._last_data = None
        self.time_of_last_data = 0
        self.imu = IMU()

    def handle_packet(self, packet):
        pkt_data = u.get_numbers_from_text(packet)
        if len(pkt_data) >= 9:
            self._last_data = u.get_numbers_from_text(packet)[0:9]
            self.imu._acc_x = self._last_data[0]
            self.imu._acc_y = self._last_data[1]
            self.imu._acc_z = self._last_data[2]
            self.imu._gyro_x = self._last_data[3]
            self.imu._gyro_y = self._last_data[4]
            self.imu._gyro_z = self._last_data[5]
            self.imu._mag_x = self._last_data[6]
            self.imu._mag_y = self._last_data[7]
            self.imu._mag_z = self._last_data[8]
            self.imu.time_of_last_data = time.time()


def get_coms_in_range(start=0, stop=20):
    coms = []
    for i in range(start, stop):
        try:
            arduino = serial.Serial(f"COM{i}", 115200, timeout=0.1)
            arduino.close()
            coms.append(f"COM{i}")
            break
        except SerialException as se:
            if "FileNotFoundError" in str(se):
                continue
            else:
                print(f"COM{i} blocked. Did you try closing Cura?")
                continue
    return coms


def get_i2c_imu(i2c_port="COM0"):
    try:
        connection = serial.Serial(i2c_port, 115200, timeout=1 / 4)
        p = serial.threaded.ReaderThread(connection, PureIMUProtocol)
        return connection, p
    except Exception as e:
        raise RuntimeError(f"Could not open I2C: {e}")
