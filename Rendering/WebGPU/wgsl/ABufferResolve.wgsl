// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause


// #include "VTK/wgsl/SceneTransform.wgsl"

struct FragmentNode
{
  color: u32,
  depth: f32,
  cellId: u32,
  propId: u32,
  compositeId: u32,
  processId: u32,
  next: u32,
};

const ABUFFER_END: u32 = 0xFFFFFFFFu;
const MAX_LAYERS: u32 = 16u;

@group(0) @binding(0) var<uniform> scene_transform: SceneTransform;
@group(0) @binding(2) var<storage, read_write> abufferHeads: array<atomic<u32>>;
@group(0) @binding(3) var<storage, read_write> abufferNodes: array<FragmentNode>;
@group(0) @binding(4) var<storage, read_write> abufferCounter: atomic<u32>;

struct ResolveOutput
{
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>,
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
  output.ids = vec4<u32>(0u, 0u, 0u, 0u);

  let coord = vec2<u32>(position.xy);
  let pixel = coord.x + coord.y * u32(scene_transform.viewport.z);
  if (pixel >= arrayLength(&abufferHeads)) {
    discard;
    return output;
  }

  var depths: array<f32, MAX_LAYERS>;
  var colors: array<u32, MAX_LAYERS>;
  var nearestDepth: f32 = 3.402823e+38;
  var nearestCell: u32 = 0u;
  var nearestProp: u32 = 0u;
  var nearestComposite: u32 = 0u;
  var nearestProcess: u32 = 0u;
  var count: u32 = 0u;
  var node: u32 = atomicLoad(&abufferHeads[pixel]);
  loop {
    if (node == ABUFFER_END) {
      break;
    }
    let nodeDepth = abufferNodes[node].depth;
    let nodeColor = abufferNodes[node].color;
    let nodeCell = abufferNodes[node].cellId;
    let nodeProp = abufferNodes[node].propId;
    let nodeComposite = abufferNodes[node].compositeId;
    let nodeProcess = abufferNodes[node].processId;
    node = abufferNodes[node].next;

    if (nodeDepth < nearestDepth) {
      nearestDepth = nodeDepth;
      nearestCell = nodeCell;
      nearestProp = nodeProp;
      nearestComposite = nodeComposite;
      nearestProcess = nodeProcess;
    }

    if (count < MAX_LAYERS) {
      var j: i32 = i32(count) - 1;
      loop {
        if (j < 0 || depths[u32(j)] >= nodeDepth) {
          break;
        }
        depths[u32(j) + 1u] = depths[u32(j)];
        colors[u32(j) + 1u] = colors[u32(j)];
        j = j - 1;
      }
      depths[u32(j + 1)] = nodeDepth;
      colors[u32(j + 1)] = nodeColor;
      count = count + 1u;
    } else if (nodeDepth < depths[0]) {
      var j: u32 = 0u;
      loop {
        if (j + 1u >= MAX_LAYERS || depths[j + 1u] <= nodeDepth) {
          break;
        }
        depths[j] = depths[j + 1u];
        colors[j] = colors[j + 1u];
        j = j + 1u;
      }
      depths[j] = nodeDepth;
      colors[j] = nodeColor;
    }
  }

  if (count == 0u) {
    discard;
    return output;
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
  output.ids = vec4<u32>(nearestCell, nearestProp, nearestComposite, nearestProcess);
  return output;
}
