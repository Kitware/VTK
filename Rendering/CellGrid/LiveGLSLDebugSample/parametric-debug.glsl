// Debug: color by cell parametric coordinates:
vec3 tn = 0.5 * pcoordVS + 0.5 * vec3(1., 1., 1.);
gl_FragData[0] = vec4(tn, 1.0);
//VTK::Light::Impl
