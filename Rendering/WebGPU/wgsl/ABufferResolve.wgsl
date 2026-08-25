// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause


struct FragmentNode
{
  color: u32,
  depth: f32,
  objectId: u32,
  next: u32,
};

const ABUFFER_END: u32 = 0xFFFFFFFFu;
const MAX_LAYERS: u32 = 16u;

@group(0) @binding(0) var<storage, read_write> abufferHeads: array<atomic<u32>>;
@group(0) @binding(1) var<storage, read_write> abufferNodes: array<FragmentNode>;
@group(0) @binding(2) var<storage, read_write> abufferCounter: atomic<u32>;

struct ResolveOutput
{
  @location(0) color: vec4<f32>,
};

@vertex
fn resolveVertexMain(@builtin(vertex_index) vertexId: u32) -> @builtin(position) vec4<f32>
{
  let x = f32((vertexId & 1u) * 2u) - 1.0;
  let y = f32((vertexId >> 1u) * 2u) - 1.0;
  return vec4<f32>(x, y, 0.0, 1.0);
}

@fragment
fn resolveFragmentMain(@builtin(position) position: vec4<f32>) -> ResolveOutput
{
  var output: ResolveOutput;
  output.color = vec4<f32>(0.0, 0.0, 0.0, 0.0);

  let coord = vec2<u32>(position.xy);
  let pixel = coord.x + coord.y * u32(scene_transform.viewport.z);
  if (pixel >= arrayLength(&abufferHeads)) {
    discard;
    return output;
  }

  var depths: array<f32, MAX_LAYERS>;
  var colors: array<u32, MAX_LAYERS>;
  var count: u32 = 0u;
  var node: u32 = atomicLoad(&abufferHeads[pixel]);
  loop {
    if (node == ABUFFER_END || count >= MAX_LAYERS) {
      break;
    }
    depths[count] = abufferNodes[node].depth;
    colors[count] = abufferNodes[node].color;
    count = count + 1u;
    node = abufferNodes[node].next;
  }

  if (count == 0u) {
    discard;
    return output;
  }

  var i: u32 = 1u;
  loop {
    if (i >= count) {
      break;
    }
    let keyDepth = depths[i];
    let keyColor = colors[i];
    var j: i32 = i32(i) - 1;
    loop {
      if (j < 0 || depths[u32(j)] >= keyDepth) {
        break;
      }
      depths[u32(j) + 1u] = depths[u32(j)];
      colors[u32(j) + 1u] = colors[u32(j)];
      j = j - 1;
    }
    depths[u32(j + 1)] = keyDepth;
    colors[u32(j + 1)] = keyColor;
    i = i + 1u;
  }

  var accum = vec4<f32>(0.0, 0.0, 0.0, 0.0);
  var k: u32 = 0u;
  loop {
    if (k >= count) {
      break;
    }
    let src = unpack4x8unorm(colors[k]);
    accum = vec4<f32>(src.rgb * src.a + accum.rgb * (1.0 - src.a), src.a + accum.a * (1.0 - src.a));
    k = k + 1u;
  }

  output.color = accum;
  return output;
}
