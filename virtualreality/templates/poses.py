"""pose objects"""


class PoseEuler(object):
    """Struct containing all variables needed for instantaneous pose."""

    __slots__ = [
        "x",
        "y",
        "z",
        "vel_x",
        "vel_y",
        "vel_z",
        "acc_x",
        "acc_y",
        "acc_z",
        "ang_x",
        "ang_y",
        "ang_z",
        "vel_ang_x",
        "vel_ang_y",
        "vel_ang_z",
        "acc_ang_x",
        "acc_ang_y",
        "acc_ang_z",
    ]

    def __init__(
        self,
        x: float = 0,
        y: float = 0,
        z: float = 0,
        vel_x: float = 0,
        vel_y: float = 0,
        vel_z: float = 0,
        acc_x: float = 0,
        acc_y: float = 0,
        acc_z: float = 0,
        ang_x: float = 0,
        ang_y: float = 0,
        ang_z: float = 0,
        vel_ang_x: float = 0,
        vel_ang_y: float = 0,
        vel_ang_z: float = 0,
        acc_ang_x: float = 0,
        acc_ang_y: float = 0,
        acc_ang_z: float = 0,
    ):
        """
        Instantaneous pose and velocity of an object.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        """
        self.x = x
        self.y = y
        self.z = z
        self.vel_x = vel_x
        self.vel_y = vel_y
        self.vel_z = vel_z
        self.acc_x = acc_x
        self.acc_y = acc_y
        self.acc_z = acc_z
        self.ang_x = ang_x
        self.ang_y = ang_y
        self.ang_z = ang_z
        self.vel_ang_x = vel_ang_x
        self.vel_ang_y = vel_ang_y
        self.vel_ang_z = vel_ang_z
        self.acc_ang_x = acc_ang_x
        self.acc_ang_y = acc_ang_y
        self.acc_ang_z = acc_ang_z

    def __getitem__(self, key):
        if key in self.__slots__:
            return getattr(self, key)

        raise KeyError(key)

    def __setitem__(self, key, val):
        assert isinstance(val, numbers.Number), "value should be numeric"

        if key in self.__slots__:
            setattr(self, key, val)
            return

        raise KeyError(key)

    def __repr__(self):
        return "<{}.{} [{}\n] object at {}>".format(
            self.__class__.__module__,
            self.__class__.__name__,
            "\n\t".join([f"{key}={self[key]}" for key in self.__slots__]),
            hex(id(self)),
        )


class Pose(object):
    """Struct containing all variables needed for instantaneous pose."""

    __slots__ = [
        "x",
        "y",
        "z",
        "r_w",
        "r_x",
        "r_y",
        "r_z",
        "vel_x",
        "vel_y",
        "vel_z",
        "ang_vel_x",
        "ang_vel_y",
        "ang_vel_z",
    ]

    def __init__(self, pose=(0, 0, 0, 1, 0, 0, 0), velocity=(0,) * 6):
        """
        Instantaneous pose and velocity of an object.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        """
        self.x: float = pose[0]  # +(x) is right in meters
        self.y: float = pose[1]  # +(y) is up in meters
        self.z: float = pose[2]  # -(z) is forward in meters
        self.r_w: float = pose[3]  # from 0 to 1
        self.r_x: float = pose[4]  # from -1 to 1
        self.r_y: float = pose[5]  # from -1 to 1
        self.r_z: float = pose[6]  # from -1 to 1

        self.vel_x: float = velocity[0]  # +(x) is right in meters/second
        self.vel_y: float = velocity[1]  # +(y) is up in meters/second
        self.vel_z: float = velocity[2]  # -(z) is forward in meters/second
        self.ang_vel_x: float = velocity[3]
        self.ang_vel_y: float = velocity[4]
        self.ang_vel_z: float = velocity[5]

    def get_vals(self):  # its faster this way
        return [self.x,
                self.y,
                self.z,

                self.r_w,
                self.r_x,
                self.r_y,
                self.r_z,

                self.vel_x,
                self.vel_y,
                self.vel_z,
                self.ang_vel_x,
                self.ang_vel_y,
                self.ang_vel_z,
                ]

    def __getitem__(self, key):
        if key in self.__slots__:
            return getattr(self, key)

        raise KeyError(key)

    def __setitem__(self, key, val):
        assert isinstance(val, numbers.Number), "value should be numeric"

        if key in self.__slots__:
            setattr(self, key, val)
            return

        raise KeyError(key)

    def __repr__(self):
        return "<{}.{} [{}] object at {}>".format(
            self.__class__.__module__,
            self.__class__.__name__,
            ", ".join([f"{key}={self[key]}" for key in self.__slots__]),
            hex(id(self)),
        )

    def __len__(self):
        return 13


class ControllerState(Pose):
    """Struct containing all variables needed for instantaneous controller state."""

    __slots__ = [
        "x",
        "y",
        "z",
        "r_w",
        "r_x",
        "r_y",
        "r_z",
        "vel_x",
        "vel_y",
        "vel_z",
        "ang_vel_x",
        "ang_vel_y",
        "ang_vel_z",
        "grip",
        "system",
        "menu",
        "trackpad_click",
        "trigger_value",
        "trackpad_x",
        "trackpad_y",
        "trackpad_touch",
        "trigger_click",
    ]

    def __init__(
        self,
        pose=(0, 0, 0, 1, 0, 0, 0),
        velocity=(0,) * 6,
        grip=0,
        system=0,
        menu=0,
        trackpad_click=0,
        trigger_value=0,
        trackpad_x=0,
        trackpad_y=0,
        trackpad_touch=0,
        trigger_click=0,
    ):
        """
        Instantaneous controller state.

        :param pose: location in meters and orientation in quaternion
        :param velocity: velocity in meters/second
                         Angular velocity of the pose in axis-angle representation. The direction is the angle of
                         rotation and the magnitude is the angle around that axis in radians/second.
        :param grip: is grip down
        :param system: is system button down
        :param menu: is menu button down
        :param trackpad_click: is trackpad clicked
        :param trigger_value: trigger pressure value
        :param trackpad_x: trackpad touch x position
        :param trackpad_y: trackpad touch y position
        :param trackpad_touch: is trackpad being touched
        :param trigger_click: is trigger clicked
        """
        super().__init__(pose, velocity)
        self.grip: int = grip  # 0 or 1
        self.system: int = system  # 0 or 1
        self.menu: int = menu  # 0 or 1
        self.trackpad_click: int = trackpad_click  # 0 or 1
        self.trigger_value: int = trigger_value  # from 0 to 1
        self.trackpad_x: float = trackpad_x  # from -1 to 1
        self.trackpad_y: float = trackpad_y  # from -1 to 1
        self.trackpad_touch: int = trackpad_touch  # 0 or 1
        self.trigger_click: int = trigger_click  # 0 or 1


    def get_vals(self):  # its faster this way
        return [self.x,
                self.y,
                self.z,

                self.r_w,
                self.r_x,
                self.r_y,
                self.r_z,

                self.vel_x,
                self.vel_y,
                self.vel_z,
                self.ang_vel_x,
                self.ang_vel_y,
                self.ang_vel_z,

                self.grip,
                self.system,
                self.menu,
                self.trackpad_click,
                self.trigger_value,
                self.trackpad_x,
                self.trackpad_y,
                self.trackpad_touch,
                self.trigger_click,
                ]

    def __len__(self):
        return 22


def get_slot_values(slotted_instance):
    """Get all slot values in a class with slots."""
    # thanks: https://stackoverflow.com/a/6720815/782170
    return [getattr(slotted_instance, slot) for slot in slotted_instance.__slots__]
