

#version 410 core

in float Height;

out vec4 FragColor;

void main()
{
    float h = (Height + 16)/64.0f;
	if (h < .02f)
		FragColor = vec4(0, 0, 0.55+h*6, 1.0);
	else if (h < .07f)
		FragColor = vec4(.3f+h*3, .3f+h*2, 0, 1.0);
	else if (h < .4f)
		FragColor = vec4(0, h+.2f, 0, 1.0);
	else if(h < .7)
		FragColor = vec4(h, h/2, 0, 1.0);
	else
		FragColor = vec4(h, h, h, 1.0);
}

