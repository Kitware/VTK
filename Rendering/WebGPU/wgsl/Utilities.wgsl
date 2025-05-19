//--------------------------------------------------------------------------------------
// Standard Geometric Functions
//--------------------------------------------------------------------------------------

// Compute the triangle face normal from 3 points
fn computeFaceNormal(pA: vec3<f32>, pB: vec3<f32>, pC: vec3<f32>) -> vec3<f32>
{
  return normalize(cross(normalize(pB - pA), normalize(pC - pA)));
}

//--------------------------------------------------------------------------------------
// Standard Transform Functions
//--------------------------------------------------------------------------------------

// From window pixel pos to projection frame at the specified z (view frame).
fn projToWindow(pos: vec4<f32>, viewport: vec4<f32>) -> vec2<f32>
{
  return viewport.zw * 0.5 * pos.xy / pos.w;
}
