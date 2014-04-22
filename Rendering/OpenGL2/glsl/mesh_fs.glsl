varying vec3 fnormal;

void main()
{
  vec3 N = normalize(fnormal);
  vec3 L = normalize(vec3(0, 1, 1));
  vec3 E = vec3(0, 0, 1);
  vec3 H = normalize(L + E);
  float df = max(0.0, dot(N, L));
  float sf = max(0.0, dot(N, H));
  vec4 ambient = 0.4 * gl_Color;
  vec4 diffuse = 0.55 * gl_Color;
  vec4 specular = 0.5 * (vec4(1, 1, 1, 1) - gl_Color);
  gl_FragColor = ambient + df * diffuse + pow(sf, 20.0) * specular;
  gl_FragColor.a = gl_Color.a;
}
