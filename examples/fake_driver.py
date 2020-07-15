"""
fake driver, meant to be used as a debugger

press c to toggle first person control

press esc to exit

"""

import math
from pathlib import Path
from pyrr import Matrix44, matrix44, Vector3
import numpy as np


import moderngl
import moderngl_window
from moderngl_window import geometry

from base import CameraWindow
from virtualreality.util import driver

myDriver = driver.UduDummyDriverReceiver('h13 c22 c22 t13 t13 t13') # receiver is global, too bad!

class Device_hmd(moderngl_window.WindowConfig):
    def __init__(self, ctx, color):
        self.ctx = ctx
        self.color = color

        self.meshes = [
            geometry.cube(size=np.array((7, 5, 1))/2, center=(0, 0, 0)),
            geometry.cube(size=np.array((2, 2, 1))/2, center=(0, 0, 2))
            ]

        self.basic_light_prog = self.load_program('programs/shadow_mapping/directional_light.glsl')
        self.basic_light_prog['shadowMap'].value = 0
        self.basic_light_prog['color'].value = self.color

    def render(self, prog):
        for i in self.meshes:
            i.render(prog)

    def write_bl(self, pose, camera, m_shadow_bias, lightDir):
        x, y, z, w, rx, ry, rz = pose[:7]
        # y = -y
        ry = -ry
        rx = -rx
        rz = -rz

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')
        mat1 = Matrix44.from_translation([x*10, y*10, z*10], dtype='f4')

        self.basic_light_prog['m_model'].write(mat1*rotz)
        self.basic_light_prog['m_proj'].write(camera.projection.matrix)
        self.basic_light_prog['m_camera'].write(camera.matrix)
        self.basic_light_prog['m_shadow_bias'].write(m_shadow_bias)
        self.basic_light_prog['lightDir'].write(lightDir)

class Device_cntrlr(moderngl_window.WindowConfig):
    def __init__(self, ctx, color):
        self.ctx = ctx
        self.color = color

        self.meshes = [
            geometry.cube(size=np.array((2, 2, 5))/2, center=(0, 0, 0)),
            geometry.cube(size=np.array((1, 1, 1))/2, center=(2, 0, 0)),
            geometry.cube(size=np.array((1, 1, 1))/2, center=(0, 0, 4))
            ]

        self.basic_light_prog = self.load_program('programs/shadow_mapping/directional_light.glsl')
        self.basic_light_prog['shadowMap'].value = 0
        self.basic_light_prog['color'].value = self.color

    def render(self, prog):
        for i in self.meshes:
            i.render(prog)

    def write_bl(self, pose, camera, m_shadow_bias, lightDir):
        x, y, z, w, rx, ry, rz = pose[:7]
        # y = -y
        ry = -ry
        rx = -rx
        rz = -rz

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')
        mat1 = Matrix44.from_translation([x*10, y*10, z*10], dtype='f4')

        self.basic_light_prog['m_model'].write(mat1*rotz)
        self.basic_light_prog['m_proj'].write(camera.projection.matrix)
        self.basic_light_prog['m_camera'].write(camera.matrix)
        self.basic_light_prog['m_shadow_bias'].write(m_shadow_bias)
        self.basic_light_prog['lightDir'].write(lightDir)

class Device_trkr(moderngl_window.WindowConfig):
    def __init__(self, ctx, color):
        self.ctx = ctx
        self.color = color

        self.meshes = [
            geometry.cube(size=np.array((2, 2, 1))/2, center=(0, 0, 0)),
            geometry.cube(size=np.array((1, 1, 1))/2, center=(2, 0, 0)),
            geometry.cube(size=np.array((1, 1, 1))/2, center=(0, 1.5, 0)),
            geometry.cube(size=np.array((1, 1, 1))/2, center=(0, 0, 3)),
            ]

        self.basic_light_prog = self.load_program('programs/shadow_mapping/directional_light.glsl')
        self.basic_light_prog['shadowMap'].value = 0
        self.basic_light_prog['color'].value = self.color

    def render(self, prog):
        for i in self.meshes:
            i.render(prog)

    def write_bl(self, pose, camera, m_shadow_bias, lightDir):
        x, y, z, w, rx, ry, rz = pose[:7]
        # y = -y
        ry = -ry
        rx = -rx
        rz = -rz

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')
        mat1 = Matrix44.from_translation([x*10, y*10, z*10], dtype='f4')

        self.basic_light_prog['m_model'].write(mat1*rotz)
        self.basic_light_prog['m_proj'].write(camera.projection.matrix)
        self.basic_light_prog['m_camera'].write(camera.matrix)
        self.basic_light_prog['m_shadow_bias'].write(m_shadow_bias)
        self.basic_light_prog['lightDir'].write(lightDir)



class ShadowMapping(CameraWindow):
    title = "Shadow Mapping"
    resource_dir = (Path(__file__) / '../resources').resolve()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.camera.projection.update(near=1, far=200)
        self.wnd.mouse_exclusivity = True

        # Offscreen buffer
        offscreen_size = 1024, 1024
        self.offscreen_depth = self.ctx.depth_texture(offscreen_size)
        self.offscreen_depth.compare_func = ''
        self.offscreen_depth.repeat_x = False
        self.offscreen_depth.repeat_y = False
        self.offscreen_color = self.ctx.texture(offscreen_size, 4)

        self.offscreen = self.ctx.framebuffer(
            color_attachments=[self.offscreen_color],
            depth_attachment=self.offscreen_depth,
        )

        # Scene geometry
        self.floor = geometry.cube(size=np.array((100, 0.3, 100)), center=(0, -16, 0))

        # self.sun = geometry.sphere(radius=1.0)

        # Debug geometry
        self.offscreen_quad = geometry.quad_2d(size=(0.5, 0.5), pos=(0.75, 0.75))
        self.offscreen_quad2 = geometry.quad_2d(size=(0.5, 0.5), pos=(0.25, 0.75))

        # Programs
        self.raw_depth_prog = self.load_program('programs/shadow_mapping/raw_depth.glsl')

        # self.basic_light = self.load_program('programs/shadow_mapping/directional_light.glsl')
        # self.basic_light['shadowMap'].value = 0
        # self.basic_light['color'].value = 1.0, 1.0, 1.0, 1.0


        self.shadowmap_program = self.load_program('programs/shadow_mapping/shadowmap.glsl')
 
        self.basic_lightFloor = self.load_program('programs/shadow_mapping/directional_light.glsl')
        self.basic_lightFloor['shadowMap'].value = 0
        self.basic_lightFloor['color'].value = 1.0, 1.0, 1.0, 1


        self.texture_prog = self.load_program('programs/texture.glsl')
        self.texture_prog['texture0'].value = 0

        self.sun_prog = self.load_program('programs/cube_simple.glsl')
        self.sun_prog['color'].value = 1, 1, 0, 1
        self.lightpos = 0, 0, 0
        time = 2
        self.lightpos = Vector3((math.sin(time) * 20, 5, math.cos(time) * 20), dtype='f4')

        self.device_list = []
        np.random.seed(69)
        for i in myDriver.device_order:
            if i == 'h':
                self.device_list.append(Device_hmd(self.ctx, tuple(np.random.randint(100, size=(4,))/100)))

            elif i == 'c':
                self.device_list.append(Device_cntrlr(self.ctx, tuple(np.random.randint(100, size=(4,))/100)))

            elif i == 't':
                self.device_list.append(Device_trkr(self.ctx, tuple(np.random.randint(100, size=(4,))/100)))

    def render(self, time, frametime):
        self.ctx.enable_only(moderngl.DEPTH_TEST | moderngl.CULL_FACE)
        scene_pos = Vector3((0, -5, -32), dtype='f4')

        all_poses = myDriver.get_pose()
        # hmd, cntl1, cntl2 = all_poses

        # --- PASS 1: Render shadow map
        self.offscreen.clear()
        self.offscreen.use()

        depth_projection = Matrix44.orthogonal_projection(-20, 20, -20, 20, -20, 40, dtype='f4')
        depth_view = Matrix44.look_at(self.lightpos, (0, 0, 0), (0, 1, 0), dtype='f4')
        depth_mvp = depth_projection * depth_view
        self.shadowmap_program['mvp'].write(depth_mvp)

        for i in range(len(myDriver.device_order)):
            self.device_list[i].render(self.shadowmap_program)

        self.floor.render(self.shadowmap_program)

        # --- PASS 2: Render scene to screen
        self.wnd.use()
        # self.basic_light['m_proj'].write(self.camera.projection.matrix)
        # self.basic_light['m_camera'].write(self.camera.matrix)
        # self.basic_light['m_model'].write(Matrix44.from_translation(scene_pos, dtype='f4') * Matrix44.from_y_rotation(time, dtype='f4'))
        bias_matrix = Matrix44(
            [[0.5, 0.0, 0.0, 0.0],
             [0.0, 0.5, 0.0, 0.0],
             [0.0, 0.0, 0.5, 0.0],
             [0.5, 0.5, 0.5, 1.0]],
            dtype='f4',
        )
        # self.basic_light['m_shadow_bias'].write(matrix44.multiply(depth_mvp, bias_matrix))
        # self.basic_light['lightDir'].write(self.lightpos)

        self.offscreen_depth.use(location=0)

        for i in range(len(myDriver.device_order)):
            self.device_list[i].write_bl(
                all_poses[i],
                self.camera,
                matrix44.multiply(depth_mvp, bias_matrix),
                self.lightpos
                )

        for i in range(len(myDriver.device_order)):
            self.device_list[i].render(self.device_list[i].basic_light_prog)


        # floor
        self.basic_lightFloor['m_model'].write(Matrix44.from_translation((0, 0, 0), dtype='f4'))
        self.basic_lightFloor['m_proj'].write(self.camera.projection.matrix)
        self.basic_lightFloor['m_camera'].write(self.camera.matrix)
        self.basic_lightFloor['m_shadow_bias'].write(matrix44.multiply(depth_mvp, bias_matrix))
        self.basic_lightFloor['lightDir'].write(self.lightpos)

        self.floor.render(self.basic_lightFloor)

        # # Render the sun position
        # self.sun_prog['m_proj'].write(self.camera.projection.matrix)
        # self.sun_prog['m_camera'].write(self.camera.matrix)
        # self.sun_prog['m_model'].write(Matrix44.from_translation(self.lightpos + scene_pos, dtype='f4'))
        # self.sun.render(self.sun_prog)

        # --- PASS 3: Debug ---
        # self.ctx.enable_only(moderngl.NOTHING)
        self.offscreen_depth.use(location=0)
        self.offscreen_quad.render(self.raw_depth_prog)
        # self.offscreen_color.use(location=0)
        # self.offscreen_quad2.render(self.texture_prog)


if __name__ == '__main__':
    with myDriver:
        myDriver.send('hello')
        moderngl_window.run_window_config(ShadowMapping)
    print ('lol')