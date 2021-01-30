import numpy as np
from mayavi import mlab


def test_anim_arrows():
    i = 0
    x, y, z = np.mgrid[-2:3, -2:3, -2:3]
    r = np.sqrt(x ** 2 + y ** 2 + z ** 4)
    u = y * np.sin(r + i) / (r + i + 0.001)
    v = -x * np.sin(r + i) / (r + i + 0.001)
    w = np.zeros_like(z)
    obj = mlab.quiver3d(x, y, z, u, v, w, line_width=3, scale_factor=1)

    @mlab.animate(delay=10)
    def run_quiver3d():
        nonlocal i
        while True:
            i += 0.1
            if i > 10:
                i = -10
            u = y * np.sin(r + i) / (r + i + 0.001)
            v = -x * np.sin(r + i) / (r + i + 0.001)
            w = np.zeros_like(z)
            obj.mlab_source.set(u=u, v=v, w=w)
            yield

    run_quiver3d()
    mlab.show()
