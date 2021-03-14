import time

import numpy as np
from pykalman import KalmanFilter


class EulerKalman(object):
    """
    A Kalman filter that keeps track of euler position and rotation.

    example usage:
        >>> k = EulerKalman() # init filter

        for i in range(10):
            print (t.apply([5+i, 6+i, 7+i])) # apply and update filter
    """

    def __init__(self, init_state=tuple(0 for _ in range(18)), n_iter=5, train_size=15):
        """
        Create the Kalman filter.

        :param init_state: Initial state of the Kalman filter. Should be equal to first element in first_train_batch.
        :param n_iter: Number of times to repeat the parameter estimation function (estimates noise)
        """
        init_state = np.array(init_state)
        if init_state.size != 18:
            raise ValueError(
                "EulerKalman expects init: [x,y,z,vx,vy,vz,ax,ay,az,r,p,w,vr,vp,vw,ar,ap,aw]"
            )
        self.t_prev = time.time()

        transition_matrix = self._get_transition_matrix()
        observation_matrix = np.eye(18)

        self._expected_shape = init_state.shape

        self._filter = KalmanFilter(
            transition_matrices=transition_matrix,
            observation_matrices=observation_matrix,
            initial_state_mean=init_state,
        )

        self._calibration_countdown = 0
        self._calibration_observations = []
        self._em_iter = n_iter

        self.calibrate(train_size, n_iter)

        self._x_now = np.zeros(3)
        self._p_now = np.zeros(3)

    def _get_transition_matrix(self):
        """
        State transition matrix for euler position+rotation with velocity and acceleration.

        See: http://campar.in.tum.de/Chair/KalmanFilter#Linear_Kalman_Filter_for_the_pos

        Assuming dt is 0.1, this gives:
        1.0     0.0     0.0     0.1     0.0     0.0     0.005   0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.005   0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.005   0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.005   0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.005   0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0     0.005
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0     0.1
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0     0.0
        0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     0.0     1.0
        :return:
        """
        a = np.eye(9)
        t_now = time.time()
        dt = t_now - self.t_prev
        a[tuple(i - 3 for i in range(3, 9)), tuple(i for i in range(3, 9))] = dt
        a[tuple(i - 6 for i in range(6, 9)), tuple(i for i in range(6, 9))] = (
            0.5 * dt ** 2
        )
        m = np.zeros((18, 18))
        m[0:9, 0:9] = a
        m[9:18, 9:18] = a
        self.t_prev = t_now
        return m

    def _check_calibration(self, obz):
        if self._calibration_countdown:
            self._calibration_observations.append(obz)
            self._calibration_countdown -= 1
            if self._calibration_countdown == 0:
                self._run_calibration()
        return self._x_now

    def apply_imu(self, imu):
        self._filter.transition_matrices = self._get_transition_matrix()

        # run with no observation to get current data for info the imu doesn't have.
        temp_x_now, _ = self._filter.filter_update(
            filtered_state_mean=self._x_now, filtered_state_covariance=self._p_now
        )

        acc = imu.get_acc(q=temp_x_now[9:12])
        orient = imu.get_instant_orientation(accel=acc)
        gyro = imu.get_gyro()
        partial_obz = temp_x_now
        partial_obz[6:9] = acc
        partial_obz[9:12] = gyro
        partial_obz[15:18] = orient

        self._x_now, new_p_now = self._filter.filter_update(
            filtered_state_mean=self._x_now,
            filtered_state_covariance=self._p_now,
            observation=partial_obz,
        )

        # only update measured variances of things we actually measured
        self._p_now[6:9] = new_p_now[6:9]
        self._p_now[9:12] = new_p_now[9:12]
        self._p_now[15:18] = new_p_now[15:18]

        self._check_calibration(partial_obz)

        return self._x_now

    def apply_blob(self, xyz):
        """Apply the Kalman filter against the observation obz."""
        # Todo: automatically start a calibration routine if everything is at rest for long enough,
        #  or if the user leaves, or if there is enough error. Also, add some UI calibrate button, save, and load.
        assert len(xyz) == 3

        self._filter.transition_matrices = self._get_transition_matrix()

        # run with no observation to get current data for info the imu doesn't have.
        temp_x_now, _ = self._filter.filter_update(
            filtered_state_mean=self._x_now, filtered_state_covariance=self._p_now
        )

        partial_obz = temp_x_now
        partial_obz[0:3] = xyz

        self._x_now, new_p_now = self._filter.filter_update(
            filtered_state_mean=self._x_now,
            filtered_state_covariance=self._p_now,
            observation=partial_obz,
        )

        # only update measured variances of things we actually measured
        self._p_now[0:3] = new_p_now[0:3]

        self._check_calibration(partial_obz)

        return self._x_now

    def _run_calibration(self):
        """Update the Kalman filter so that noise matrices are more accurate."""
        t_start = time.time()
        self._filter = self._filter.em(
            self._calibration_observations, n_iter=self._em_iter
        )
        print(f" kalman filter calibrated, took {time.time() - t_start}s")
        f_means, f_covars = self._filter.filter(self._calibration_observations)
        self._x_now = f_means[-1, :]
        self._p_now = f_covars[-1, :]
        self._calibration_observations = []

    def calibrate(self, observations=100000, em_iter=10):
        """
        Start the calibration routine.

        :param observations: Number of observations before calibration runs.
        :param em_iter: number of times calibration runs over all observations.
        """
        self._calibration_countdown = observations
        self._em_iter = em_iter
