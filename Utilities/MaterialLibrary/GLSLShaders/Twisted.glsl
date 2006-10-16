uniform float Rate;

void main()
{
  float angle = gl_Vertex[2];
  float rate = Rate;
  vec4 newPos;
  newPos[0] = cos(angle* rate) * gl_Vertex[0] + sin(angle* rate) *gl_Vertex[1];
  newPos[1] = -sin(angle* rate) * gl_Vertex[0] + cos(angle* rate) *gl_Vertex[1];
  newPos[2] = gl_Vertex[2];
  newPos[3] = gl_Vertex[3];

  vec3 newNormal;
  newNormal[0] = cos(angle* rate) * gl_Normal[0] + sin(angle* rate) *gl_Normal[1];
  newNormal[1] = -sin(angle* rate) * gl_Normal[0] + cos(angle* rate) *gl_Normal[1];
  newNormal[2] = gl_Normal[2];

  vec4 ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
  vec3 normal = normalize(gl_NormalMatrix * newNormal);
  vec3 lightDir = normalize(vec3(gl_LightSource[0].position));
  float NdotL = max(dot(normal, lightDir), 0.0);

  vec4 col = vec4(0,0.5,0.5,1);
  vec4 diffuse = col * gl_LightSource[0].diffuse * NdotL;
  vec4 specular;
  
  if (NdotL > 0.0) 
    {
    float NdotHV = max(dot(normal, gl_LightSource[0].halfVector.xyz), 0.0);
    specular = gl_FrontMaterial.specular * gl_LightSource[0].specular * 
      pow(NdotHV, gl_FrontMaterial.shininess);
    }
  gl_FrontColor = diffuse + ambient + specular;

  gl_Position = gl_ModelViewProjectionMatrix * newPos;



}
