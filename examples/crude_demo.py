import moderngl
import numpy as np
from pyrr import Vector3, Matrix44
import pyrr
import cv2

import virtualreality as vr
from virtualreality.util import driver


M_VERTEX_SHADER = '''
#version 330

in vec3 in_vert;

uniform mat4 transformMat;
uniform mat4 projection;


void main() {
    gl_Position = projection*transformMat * vec4(in_vert, 1.0);
}
'''

M_FRAG_SHADER = '''
#version 330

uniform vec3 color;

out vec4 f_color;

void main() {
    f_color = vec4(color, 1);
}
'''

ctx = moderngl.create_standalone_context()
ctx.enable(moderngl.DEPTH_TEST)

prog = ctx.program(
    vertex_shader=M_VERTEX_SHADER,
    fragment_shader=M_FRAG_SHADER,
)


prog2 = ctx.program(
    vertex_shader=M_VERTEX_SHADER,
    fragment_shader=M_FRAG_SHADER,
)

prog3 = ctx.program(
    vertex_shader=M_VERTEX_SHADER,
    fragment_shader=M_FRAG_SHADER,
)

prog4 = ctx.program(
    vertex_shader=M_VERTEX_SHADER,
    fragment_shader=M_FRAG_SHADER,
)

x = np.array([ 1, -1, 0,   0,  0, 0,   1,  0, -1])/5
y = np.array([ 0,  0, 0,  -1,  1, 0,   0, -1,  1])/5 *(-1)
z = np.array([-1, -1, 1,  -1, -1, 1,  -1, -1, -1])/5
# r = np.ones(50)
# g = np.zeros(50)
# b = np.zeros(50)
size = (1280, 720)


vertices = np.dstack([x, y, z]) # pointy thing

x = np.array([ 1, -1, 0,  0,])/5 * 3
y = np.array([ 0,  0, 0,  0,])/5 *(-1)
z = np.array([ 0,  0, 1, -1,])/5

fv = np.dstack([x, y, z]) * 10

vbo = ctx.buffer(vertices.astype('f4').tobytes())
vbo2 = ctx.buffer(vertices.astype('f4').tobytes())
vbo3 = ctx.buffer(vertices.astype('f4').tobytes())
vbo4 = ctx.buffer(fv.astype('f4').tobytes())

# vertex array with a "shader" for the 1 pointy thing
vao = ctx.vertex_array(prog, [
                            (vbo, '3f', 'in_vert'),
                            ])

prog['color'].write(np.array([1,0,1]).astype('f4').tobytes()) # setting the color

# vertex array with a "shader" for the 2 pointy thing
vao2 = ctx.vertex_array(prog2, [
                            (vbo2, '3f', 'in_vert'),
                            ])
prog2['color'].write(np.array([0,1,0]).astype('f4').tobytes()) # setting the color

# vertex array with a "shader" for the 3 pointy thing
vao3 = ctx.vertex_array(prog3, [
                            (vbo3, '3f', 'in_vert'),
                            ])
prog3['color'].write(np.array([1,0.3,0]).astype('f4').tobytes()) # setting the color


floor = ctx.vertex_array(prog4, [
                            (vbo4, '3f', 'in_vert'),
                            ])
prog4['color'].write(np.array([1,1,1]).astype('f4').tobytes()) # setting the color

# boring render buffers setup, needed to get depth or something
cbo = ctx.renderbuffer(size)
dbo = ctx.depth_texture(size, alignment=1)
fbo = ctx.framebuffer(color_attachments=(cbo,), depth_attachment=dbo)

fbo.use()


# set perspective for all "shaders"
prog['projection'].write(Matrix44.perspective_projection(95, 16/9, 1, 200, dtype='f4'))
prog2['projection'].write(Matrix44.perspective_projection(95, 16/9, 1, 200, dtype='f4'))
prog3['projection'].write(Matrix44.perspective_projection(95, 16/9, 1, 200, dtype='f4'))
prog4['projection'].write(Matrix44.perspective_projection(95, 16/9, 1, 200, dtype='f4'))

mydriver = driver.DummyDriverReceiver(57)

modelRotOffz = Matrix44.from_y_rotation(np.pi, dtype='f4')

viewPosOffz = Matrix44.from_translation([0, 0, -3], dtype='f4')
# viewPosOffz *= Matrix44.from_y_rotation(np.pi/4, dtype='f4')
prog4['transformMat'].write(Matrix44.from_translation([0, 1.2, 0], dtype='f4')*viewPosOffz)

with mydriver:
    mydriver.send('hello')
    while 1:
        fbo.clear(0.0, 0.0, 0.0, 0.0)

        data = mydriver.get_pose()
        hmd = data[:13]
        cntl1 = data[13:13+22]
        cntl2 = data[13+22:13+22+22]


        x, y, z, w, rx, ry, rz = hmd[:7]
        y = -y
        ry = -ry

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')*modelRotOffz
        mat1 = Matrix44.from_translation([x, y, z], dtype='f4')*viewPosOffz

        prog['transformMat'].write(mat1*rotz)

        x, y, z, w, rx, ry, rz = cntl1[:7]
        y = -y
        ry = -ry

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')*modelRotOffz
        mat1 = Matrix44.from_translation([x, y, z], dtype='f4')*viewPosOffz

        prog2['transformMat'].write(mat1*rotz)

        x, y, z, w, rx, ry, rz = cntl2[:7]
        y = -y
        ry = -ry

        rotz = Matrix44.from_quaternion([rx, ry, rz, w], dtype='f4')*modelRotOffz
        mat1 = Matrix44.from_translation([x, y, z], dtype='f4')*viewPosOffz

        prog3['transformMat'].write(mat1*rotz)

        vao.render(moderngl.TRIANGLES) # shader render
        vao2.render(moderngl.TRIANGLES) # shader render
        vao3.render(moderngl.TRIANGLES) # shader render
        floor.render(moderngl.LINE_LOOP)

        # get the rendered frame
        frame = np.frombuffer(fbo.read(components=4, dtype='f4'), dtype='f4')
        frame = frame.reshape((*size[::-1], 4))

        # get the depth mask
        depth = np.frombuffer(dbo.read(alignment=1), dtype='f4')
        depth = depth.reshape(size[::-1])
        depth = np.repeat(depth[:, :, np.newaxis], 4, axis=2)

        cv2.imshow('nut', cv2.cvtColor(frame*depth, cv2.COLOR_RGB2BGR))
        # cv2.imshow('nu2', depth)

        kk = cv2.waitKey(10) & 0xFF

        if kk == 27:
            break

fbo.release()

cv2.destroyAllWindows()