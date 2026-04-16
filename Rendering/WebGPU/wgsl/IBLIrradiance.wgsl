// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct IrradianceParameters
{
  step: f32,
  source_level: f32,
  size: u32,
  pad0: u32,
}

@group(0) @binding(0) var<uniform> parameters: IrradianceParameters;
@group(0) @binding(1) var environment_sampler: sampler;
@group(0) @binding(2) var environment: texture_cube<f32>;
@group(0) @binding(3) var output_faces: texture_storage_2d_array<rgba16float, write>;

@compute @workgroup_size(8, 8, 1)
fn irradiance(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  let uv = (vec2<f32>(id.xy) + vec2<f32>(0.5)) / f32(parameters.size);
  let n = cubeDirection(id.z, uv);
  let up = select(vec3<f32>(0.0, 1.0, 0.0), vec3<f32>(0.0, 0.0, 1.0), abs(n.y) > 0.999);
  let t = normalize(cross(n, up));
  let m = mat3x3<f32>(t, cross(n, t), n);
  let phiSteps = u32(ceil(2.0 * PI / parameters.step));
  let thetaSteps = u32(ceil(0.5 * PI / parameters.step));
  var acc = vec3<f32>(0.0);
  for (var i = 0u; i < phiSteps; i++)
  {
    let phi = f32(i) * parameters.step;
    for (var j = 0u; j < thetaSteps; j++)
    {
      let theta = f32(j) * parameters.step;
      let s = vec3<f32>(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      acc += textureSampleLevel(environment, environment_sampler, m * s,
        parameters.source_level).rgb * cos(theta) * sin(theta);
    }
  }
  let count = f32(phiSteps) * f32(thetaSteps);
  textureStore(output_faces, id.xy, id.z, vec4<f32>(acc * (PI / count), 1.0));
}
