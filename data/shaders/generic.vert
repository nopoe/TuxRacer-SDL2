attribute vec4 vert_pos;
attribute vec2 source_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

varying vec2 dest_tex_coord;

void main()
{
    mat4 MVP_mat=projection * view * model;
    dest_tex_coord=source_tex_coord;
	gl_Position=MVP_mat*vert_pos;
}