// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

override haveTCoords: f32 = 0.0;
override haveLines: f32 = 0.0;

struct CameraUniforms {
    WCDCMatrix: mat4x4f,
    MCWCMatrix: mat4x4f,
};

@group(0) @binding(0) var<uniform> camera: CameraUniforms;
@group(0) @binding(1) var texture2D: texture_2d<f32>;
@group(0) @binding(2) var textureSampler: sampler;

struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec4f,
    @location(2) uv: vec2f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) vertexColor: vec4f,
    @location(1) tcoord: vec2f,
    @location(2) ldistance: f32,
};

@vertex
fn ContextDevice2DVertex(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;

    output.vertexColor = input.color;

    if haveTCoords > 0.0 {
        output.tcoord = input.uv;
    }

    if haveLines > 0.0 {
        output.ldistance = input.uv.x;
    }

    let vertex = vec4f(input.position.x, input.position.y, 0.0, 1.0);
    output.position = camera.WCDCMatrix * camera.MCWCMatrix * vertex;

    return output;
}

struct FragmentOutput {
    @location(0) color: vec4f,
    @location(1) pick_id: vec4<u32>,
};

@fragment
fn ContextDevice2DFragment(input: VertexOutput) -> FragmentOutput {

    var output: FragmentOutput;
    var finalColor = input.vertexColor;

    if haveTCoords > 0.0 {
        let sampledColor = textureSample(texture2D, textureSampler, input.tcoord);
        finalColor = sampledColor * input.vertexColor;
    }

    output.color = finalColor;
    output.pick_id = vec4<u32>(0u, 0u, 0u, 0u);

    return output;
}
