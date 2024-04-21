

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out float Height;
out vec3 Position;
out vec2 TexCoord;

void main()
{
    Height = aPos.y;
    gl_Position = vec4(aPos, 1.0);
	TexCoord = aTex;
}