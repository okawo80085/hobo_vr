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
    vector = np.asarray(vector)
    norm = np.linalg.norm(vector)
    if norm != 0:
        return vector / norm
    else:
        return vector


def angle_between(v1, v2):
    """Returns the angle in radians between vectors 'v1' and 'v2'::

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
    cos_theta = np.clip(np.dot(v1_u, v2_u), -1.0, 1.0)
    if cos_theta > 0:
        return np.arccos(cos_theta)
    else:
        return np.arccos(cos_theta) - np.pi / 2.0


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


def get_ki_kbias(convergance_time, overshoot_measure, sampling_rate):
    ki = sampling_rate / (1.4 * convergance_time + sampling_rate)
    kbias = (overshoot_measure ** 2 * ki) / (160 * convergance_time)
    return ki, kbias


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
        self.grav_magnitude = (
            14  # I have no idea why gravity is 14 on my imu, but it is
        )
        self._mag_iron_offset_x = 23.5
        self._mag_iron_offset_y = 23.5
        self._mag_iron_offset_z = -45
        self.prev_up = None
        self.prev_north = None
        self.prev_west = None
        self.prev_orientation_time = None
        self.prev_bias = None

    def get_north_west_up(self):
        grav = np.asarray(unit_vector(self.get_grav()))
        mag = unit_vector(np.asarray(self.get_mag()))
        east = unit_vector(np.cross(mag, grav))
        north = -unit_vector(np.cross(east, grav))
        up = -unit_vector(np.cross(north, east))

        return north, -east, up

    def get_instant_orientation(self):
        """Gets orientation quaternion"""
        north, west, up = self.get_north_west_up()
        rot = np.asarray([north, west, up]).transpose()
        q = Quaternion.from_matrix(rot)
        return q

    def get_orientation(
        self,
        convergence_acc: float,
        overshoot_acc: float,
        convergence_mag: float,
        overshoot_mag: float,
    ):
        # https://www.sciencedirect.com/science/article/pii/S2405896317321201
        if self.prev_up is not None:
            t2 = time.time()

            k_acc, k_bias_acc = get_ki_kbias(
                convergence_acc, overshoot_acc, t2 - self.prev_orientation_time
            )
            k_mag, k_bias_mag = get_ki_kbias(
                convergence_mag, overshoot_mag, t2 - self.prev_orientation_time
            )

            grav = np.asarray(unit_vector(self.get_grav()))
            mag = unit_vector(np.asarray(self.get_mag()))
            east = unit_vector(np.cross(mag, grav))
            west = -east
            north = -unit_vector(np.cross(east, grav))
            up = -unit_vector(np.cross(north, east))

            gyro = -self.get_gyro()
            if self.prev_bias is not None:
                if np.isnan(self.prev_bias).any():
                    self.prev_bias = None
                else:
                    gyro += self.prev_bias

            gy = Quaternion.from_eulers(gyro * (t2 - self.prev_orientation_time))
            pred_west = (
                ~gy
                * Quaternion(
                    np.pad(self.prev_west, (0, 1), "constant", constant_values=0)
                )
                * gy
            )
            pred_up = (
                ~gy
                * Quaternion(
                    np.pad(self.prev_up, (0, 1), "constant", constant_values=0)
                )
                * gy
            )
            pred_north = (
                ~gy
                * Quaternion(
                    np.pad(self.prev_north, (0, 1), "constant", constant_values=0)
                )
                * gy
            )

            pred_north_v = np.asarray(pred_north.xyz)
            pred_west_v = np.asarray(pred_west.xyz)
            pred_up_v = np.asarray(pred_up.xyz)

            fix_north = Quaternion.from_axis(np.cross(north, pred_north_v) * k_mag)
            fix_west = Quaternion.from_axis(np.cross(west, pred_west_v) * k_mag)
            fix_up = Quaternion.from_axis(np.cross(up, pred_west_v) * k_acc)

            x_acc = np.cross(up, pred_up_v)
            x_mag = np.cross(north, pred_north_v)
            pb0 = k_bias_acc * x_acc + k_bias_mag * x_mag
            if self.prev_bias is not None:
                self.prev_bias = (
                    self.prev_bias * (t2 - self.prev_orientation_time) + pb0
                )
            else:
                self.prev_bias = pb0

            true_north = unit_vector((pred_north * fix_north).xyz)
            true_up = unit_vector((pred_up * fix_up).xyz)
            true_west = unit_vector((pred_west * fix_west).xyz)

            rot = np.asarray([true_north, true_west, true_up]).transpose()
            q = Quaternion.from_matrix(rot)

            if not np.isnan(north).any():
                self.prev_north = north
            if not np.isnan(west).any():
                self.prev_west = west
            if not np.isnan(up).any():
                self.prev_up = up

            self.prev_orientation_time = t2

            return q
        else:
            grav = np.asarray(unit_vector(self.get_grav()))
            mag = unit_vector(np.asarray(self.get_mag()))
            east = unit_vector(np.cross(mag, grav))
            north = -unit_vector(np.cross(east, grav))
            up = -unit_vector(np.cross(north, east))

            rot = np.asarray([north, -east, up]).transpose()
            q = Quaternion.from_matrix(rot)
            if (not np.isnan(grav).any()) and (not np.isnan(mag).any()):
                self.prev_west = -east
                self.prev_north = north
                self.prev_up = up
                self.prev_orientation_time = time.time()
            return q

    def get_grav(self, q=None, magnitude=None, stationary=False, accel=None):
        if stationary or q is None or ((accel is not None) and (not np.any(accel))):
            g = np.asarray([self._acc_x, self._acc_y, self._acc_z])
            self.grav_magnitude = sum(abs(np.asarray(g)))
            return g
        elif q is not None:
            if not isinstance(q, Quaternion):
                q = Quaternion.from_euler(*q)
            if magnitude is None:
                magnitude = self.grav_magnitude
            # http://www.varesano.net/blog/fabio/simple-gravity-compensation-9-dom-imus
            g = np.asarray([0.0, 0.0, 0.0])

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
        return np.asarray([self._acc_x - g[0], self._acc_y - g[1], self._acc_z - g[2]])

    def get_gyro(self):
        return np.asarray([self._gyro_x, self._gyro_y, self._gyro_z])

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
        return np.asarray(mag)


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
            self._last_data = pkt_data[:9]
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
