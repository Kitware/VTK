//--------------------------------------------------------------------------------------
// Array Access Functions
//--------------------------------------------------------------------------------------

fn getTuple4F32(tuple_id: u32, offset: u32, values: ptr<storage,array<f32>>) -> vec4<f32>
{
  return vec4<f32>(
    values[4u * tuple_id + offset],
    values[4u * tuple_id + 1u + offset],
    values[4u * tuple_id + 2u + offset],
    values[4u * tuple_id + 3u +offset]
  );
}

fn getTuple3F32(tuple_id: u32, offset: u32, values: ptr<storage,array<f32>>) -> vec3<f32>
{
  return vec3<f32>(
    values[3u * tuple_id + offset],
    values[3u * tuple_id + 1u + offset],
    values[3u * tuple_id + 2u + offset]
  );
}

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
