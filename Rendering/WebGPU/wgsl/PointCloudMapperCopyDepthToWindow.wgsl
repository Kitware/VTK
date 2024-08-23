// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct VertexOutput {
  @builtin(position) position: vec4<f32>,
}

@vertex
fn vertexMain(@builtin(vertex_index) vertex_id: u32) -> VertexOutput {
  var output: VertexOutput;
  var coords: array<vec2<f32>, 4> = array<vec2<f32>, 4>(
    vec2<f32>(-1, -1), // bottom-left
    vec2<f32>(-1,  1), // top-left
    vec2<f32>( 1, -1), // bottom-right
    vec2<f32>( 1,  1)  // top-right
  );

  output.position = vec4<f32>(coords[vertex_id].xy, 1.0, 1.0);

  return output;
}

struct FragmentInput {
  @builtin(position) position: vec4<f32>,
}

struct FragmentOutput {
  @builtin(frag_depth) fragmentDepth: f32
}

@group(0) @binding(0) var<storage, read> pointDepthBuffer: array<u32>;
@group(0) @binding(1) var<uniform> framebufferWidth: u32;

@fragment
fn fragmentMain(fragment: FragmentInput) -> FragmentOutput {
  var fragOut: FragmentOutput;

  let pixelIndex = u32(fragment.position.x) + u32(fragment.position.y) * framebufferWidth;
  fragOut.fragmentDepth = f32(pointDepthBuffer[pixelIndex]) / (pow(2.0, 32.0) - 1);

  return fragOut;
}
