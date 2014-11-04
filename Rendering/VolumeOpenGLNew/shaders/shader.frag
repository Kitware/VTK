#version 330 core

layout(location = 0) out vec4 vFragColor;	//fragment shader output

void main()
{
  //return the constant white colour as shader output
  vFragColor = vec4(1,1,1,1);
}
