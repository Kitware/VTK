// Debug: color by normal vector (offset to avoid black):
vec3 tn = 0.25 * vertexNormalVCVS + 0.75 * vec3(1., 1., 1.);
gl_FragData[0] = vec4(tn, 1.0);
//VTK::Light::Impl
