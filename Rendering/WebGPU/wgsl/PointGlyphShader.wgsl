// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"

/**
 * (-1, 1) |-------------------------------|(1, 1)
 *         |-                              |
 *         |    -                          |
 *         |        -                      |
 * (-1, 0) |              -                |
 *         |                   -           |
 *         |                        -      |
 *         |                              -|
 * (-1,-1) |-------------------------------|(1, -1)
 */
// these two triangle describes a quad spanning a bi-unit domain.
const TRIANGLE_VERTS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(1, 1),
  vec2f(-1, 1),
  vec2f(1, -1),
);

struct ActorBlock {
  transform: ActorTransform,
  render_options: ActorRenderOptions,
  color_options: ActorColorOptions,
}

//-------------------------------------------------------------------
struct F32s {
  values: array<f32>
}

//-------------------------------------------------------------------
struct U32s {
  values: array<u32>
}

//-------------------------------------------------------------------
struct AttributeArrayDescriptor {
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct MeshDescriptor {
  position: AttributeArrayDescriptor,
  point_normal: AttributeArrayDescriptor,
  point_tangent: AttributeArrayDescriptor,
  point_uv: AttributeArrayDescriptor,
  cell_normal: AttributeArrayDescriptor,
  instance_colors: AttributeArrayDescriptor,
  instance_transforms: AttributeArrayDescriptor,
  instance_normal_transforms: AttributeArrayDescriptor,
  composite_id: u32,
  process_id: u32,
  pickable: u32,
}

//-------------------------------------------------------------------
struct Topology {
  // the vtk cell ID for this index. used to index into a cell attribute.
  cell_id: u32,
  // the vtk point ID for this index. used to index into a point attribute.
  point_id: u32,
}

///-----------------------------------------------------------------///
// Renderer controlled descriptor set
///-----------------------------------------------------------------///
@group(0) @binding(0) var<uniform> scene_transform: SceneTransform;
@group(0) @binding(1) var<storage, read> scene_lights: SceneLights;

///-------------------------------------------------------------------
// Everything shader needs from the vtkActor and it's vtkProperty
///-------------------------------------------------------------------
@group(1) @binding(0) var<storage, read> actor: ActorBlock;

///-----------------------------------------------------------------///
// Mesh attributes.
///-----------------------------------------------------------------///
@group(2) @binding(0) var<storage, read> mesh: MeshDescriptor;
@group(2) @binding(1) var<storage, read> point_data: F32s;
@group(2) @binding(2) var<storage, read> cell_data: F32s;
@group(2) @binding(3) var<storage, read> instance_data: F32s;

///-----------------------------------------------------------------///
// Topology
///-----------------------------------------------------------------///
@group(3) @binding(0) var<storage, read> topology: array<Topology>;


//-------------------------------------------------------------------
struct VertexInput {
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32
}

//-------------------------------------------------------------------
struct VertexOutput {
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>,
  @location(2) normal_vc: vec3<f32>,
  @location(3) tangent_vc: vec3<f32>,
  @location(4) local_position: vec2<f32>,
  @location(5) @interpolate(flat) cell_id: u32,
  @location(6) @interpolate(flat) prop_id: u32,
  @location(7) @interpolate(flat) composite_id: u32,
  @location(8) @interpolate(flat) process_id: u32,
}

//-------------------------------------------------------------------
@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  let glyph_transform = getMat4F32(vertex.instance_id, mesh.instance_transforms.start, &instance_data.values);
  let glyph_normal_transform = getMat3F32(vertex.instance_id, mesh.instance_normal_transforms.start, &instance_data.values);

  let local_vertex_id: u32 = vertex.vertex_id % 6;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.vertex_id / 6;
  // get CellID from vertex ID -> VTK cell map.
  let cell_id = topology[pull_vertex_id].cell_id;
  // pull the point id
  let point_id = topology[pull_vertex_id].point_id;
  // pull the position for this vertex.
  let p_mc = vec4<f32>(getTuple3F32(point_id, mesh.position.start, &point_data.values), 1.0);

  // Write indices
  output.cell_id = cell_id;
  output.prop_id = actor.color_options.id;
  output.composite_id = mesh.composite_id;
  output.process_id = 0u;

  ///------------------------///
  // NDC transforms
  ///------------------------///
  // transform to view and then to clip space.
  let p_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform * p_mc;

  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let p_screen = resolution * (0.5 * p_dc.xy / p_dc.w + 0.5);

  var point_size = actor.render_options.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0 {
    point_size = 1.0;
  }
  output.local_position = TRIANGLE_VERTS[local_vertex_id];
  let pscreen_offsetted = p_screen + 0.5 * point_size * output.local_position;
  output.position = vec4(p_dc.w * ((2.0 * pscreen_offsetted) / resolution - 1.0), p_dc.z, p_dc.w);
  output.position_vc = scene_transform.inverted_projection * output.position;

  ///------------------------///
  // color
  ///------------------------///
  output.color = getTuple4F32(vertex.instance_id, mesh.instance_colors.start, &instance_data.values);

  ///------------------------///
  // Set Normals/Tangents
  // Basically infers what kind of directional vectors are available to use in lighting calculations.
  ///------------------------///
  if mesh.cell_normal.num_tuples > 0u {
    // pull normal of this vertex from cell normals
    let normal_mc = getTuple3F32(cell_id, mesh.cell_normal.start, &cell_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * normal_mc;
  } else if mesh.point_tangent.num_tuples > 0u {
    // pull tangent of this vertex from point tangents
    let tangentMC = getTuple3F32(point_id, mesh.point_tangent.start, &point_data.values);
    output.tangent_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * tangentMC;
  } else if mesh.point_normal.num_tuples > 0u {
    // this if is after cell normals, so that when both are available, point normals are used.
    // pull normal of this vertex from point normals
    let normal_mc = getTuple3F32(point_id, mesh.point_normal.start, &point_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * normal_mc;
  } else {
    output.normal_vc = vec3<f32>(0.0, 0.0, 1.0);
  }
  return output;
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @builtin(frag_depth) frag_depth: f32,
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // cell_id, prop_id, composite_id, process_id
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(vertex: VertexOutput) -> FragmentOutput {
  var output: FragmentOutput;
  var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);

  var opacity: f32;

  if (mesh.pickable == 1u)
  {
    output.ids.x = vertex.cell_id + 1;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;
  }

  ///------------------------///
  // Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  ///------------------------///
  let vertex_visibility = getVertexVisibility(actor.render_options.flags);
  if (vertex_visibility) {
    // use vertex color instead of point scalar colors when drawing vertices.
    ambient_color = actor.color_options.vertex_color;
    diffuse_color = actor.color_options.vertex_color;
    opacity = actor.color_options.opacity;
  } else {
    ambient_color = vertex.color.rgb;
    diffuse_color = vertex.color.rgb;
    opacity = vertex.color.a;
  }


  let d = length(vertex.local_position); // distance of fragment from the input vertex.
  let point_2d_shape = getPoint2DShape(actor.render_options.flags);
  let render_points_as_spheres = getRenderPointsAsSpheres(actor.render_options.flags);
  if (((point_2d_shape == POINT_2D_ROUND) || render_points_as_spheres) && (d > 1)) {
    discard;
  }

  let point_size = clamp(actor.render_options.point_size, 1.0f, 100000.0f);
  var normal_vc = normalize(vertex.normal_vc);
  if (render_points_as_spheres) {
    if (d > 1) {
      discard;
    }
    normal_vc = normalize(vec3f(vertex.local_position, 1));
    normal_vc.z = sqrt(1.0f - d * d);
    // Pushes the fragment in order to fake a sphere.
    // See Rendering/OpenGL2/PixelsToZBufferConversion.txt for the math behind this. Note that,
    // that document assumes the conventions for depth buffer in OpenGL,
    // where, the z-buffer spans [-1, 1]. In WebGPU, the depth buffer spans [0, 1].
    let r = point_size / (scene_transform.viewport.z * scene_transform.projection[0][0]);
    if (getUseParallelProjection(scene_transform.flags)) {
      let s = scene_transform.projection[2][2];
      output.frag_depth = vertex.position.z + normal_vc.z * r * s;
    }
    else
    {
      let s = -scene_transform.projection[2][2];
      output.frag_depth = (s - vertex.position.z) / (normal_vc.z * r - 1.0) + s;
    }
  }
  else
  {
    output.frag_depth = vertex.position.z;
  }

  ///------------------------///
  // Lights
  ///------------------------///
  if (actor.color_options.interpolation_type == VTK_FLAT && !render_points_as_spheres) {
    // when interpolation type is flag, do not use normal vector and light direction to compute color.
    output.color = vec4<f32>(
      actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
      opacity
    );
  } else {
    if scene_lights.count == 0u {
      // allow post-processing this pixel.
      output.color = vec4<f32>(
        actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
        opacity
      );
    } else if scene_lights.count == 1u {
      let light: SceneLight = scene_lights.values[0];
      if light.positional == 1u {
        // TODO: positional
        output.color = vec4<f32>(
            actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
            opacity
        );
      } else {
        // headlight
        let df: f32 = max(0.000001f, normal_vc.z);
        let sf: f32 = pow(df, actor.color_options.specular_power);
        diffuse_color = df * diffuse_color * light.color;
        specular_color = sf * actor.color_options.specular_intensity * actor.color_options.specular_color * light.color;
        output.color = vec4<f32>(
            actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color + specular_color,
            opacity
        );
      }
    } else {
      // TODO: light kit
      output.color = vec4<f32>(
        actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
        opacity
      );
    }
  }
  // pre-multiply colors
  output.color = vec4(output.color.rgb * opacity, opacity);
  return output;
}
