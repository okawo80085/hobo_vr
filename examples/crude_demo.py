import moderngl
import numpy as np
from pyrr import Vector3, Matrix44
import cv2

import virtualreality as vr

ctx = moderngl.create_standalone_context()
ctx.enable(moderngl.DEPTH_TEST)

prog = ctx.program(
    vertex_shader='''
        #version 330

        in vec3 in_vert;

        uniform mat4 transformMat;
        uniform mat4 projection;


        void main() {
            gl_Position = projection*transformMat * vec4(in_vert, 1.0);
        }
    ''',
    fragment_shader='''
        #version 330

        out vec4 f_color;

        void main() {
            f_color = vec4(0, 0, 1, 1);
        }
    ''',
)


prog2 = ctx.program(
    vertex_shader='''
        #version 330

        in vec3 in_vert;

        uniform mat4 transformMat;
        uniform mat4 projection;


        void main() {
            gl_Position = projection*transformMat * vec4(in_vert, 1.0);
        }
    ''',
    fragment_shader='''
        #version 330

        out vec4 f_color;

        void main() {
            f_color = vec4(0, 1, 0, 1);
        }
    ''',
)

x = np.array([ 1, -1, 0,   0,  0, 0])/5
y = np.array([ 0,  0, 0,  -2,  2, 0])/5
z = np.array([-1, -1, 1,  -1, -1, 1])/5
# r = np.ones(50)
# g = np.zeros(50)
# b = np.zeros(50)
size = (1280, 720)


vertices = np.dstack([x, y, z]) # blue pointy thing

vbo = ctx.buffer(vertices.astype('f4').tobytes())
vbo2 = ctx.buffer(vertices.astype('f4').tobytes())

# vertex array with a "shader" for the blue pointy thing
vao = ctx.vertex_array(prog, [
							(vbo, '3f', 'in_vert'),
							])

# vertex array with a "shader" for the green pointy thing
vao2 = ctx.vertex_array(prog2, [
							(vbo2, '3f', 'in_vert'),
							])


# boring render buffers setup, needed to get depth or something
cbo = ctx.renderbuffer(size)
dbo = ctx.depth_texture(size, alignment=1)
fbo = ctx.framebuffer(color_attachments=(cbo,), depth_attachment=dbo)

fbo.use()


# set perspective for both "shaders"
prog['projection'].write(Matrix44.perspective_projection(80, 16/9, 1, 200, dtype='f4'))
prog2['projection'].write(Matrix44.perspective_projection(80, 16/9, 1, 200, dtype='f4'))

h = 0

while 1:
	fbo.clear(0.0, 0.0, 0.0, 0.0)

	mat = Matrix44.from_y_rotation(h, dtype='f4')
	mat2 = Matrix44.from_x_rotation(h, dtype='f4')
	mat3 = Matrix44.from_translation([np.sin(h), 0, -2], dtype='f4')
	mat4 = Matrix44.from_translation([np.cos(h), -1, -2], dtype='f4')

	prog['transformMat'].write(mat3*mat2*mat)
	prog2['transformMat'].write(mat4*mat2*mat)
	vao.render(moderngl.TRIANGLES) # render one shader
	vao2.render(moderngl.TRIANGLES) # render the other shader

	# get the rendered frame
	frame = np.frombuffer(fbo.read(components=4, dtype='f4'), dtype='f4')
	frame = frame.reshape((*size[::-1], 4))

	# get the depth mask
	depth = np.frombuffer(dbo.read(alignment=1), dtype='f4')
	depth = depth.reshape(size[::-1])
	depth = np.repeat(depth[:, :, np.newaxis], 4, axis=2)

	cv2.imshow('nut', cv2.cvtColor(frame*depth, cv2.COLOR_RGB2BGR))


	kk = cv2.waitKey(1) & 0xFF

	if kk == 27:
		break

	h += 0.05

fbo.release()

cv2.destroyAllWindows()