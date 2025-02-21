// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"

/**
 * (0, 0.5) |-------------------------------|(1, 0.5)
 *          |-                              |
 *          |    -                          |
 *          |        -                      |
 * (0, 0)   |              -                |
 *          |                   -           |
 *          |                        -      |
 *          |                              -|
 * (0,-0.5) |-------------------------------|(1, -0.5)
 */
const TRIANGLE_VERTS = array(
  vec2(0, -0.5),
  vec2(1, -0.5),
  vec2(0, 0.5),
  vec2(1, 0.5),
  vec2(0, 0.5),
  vec2(1, -0.5),
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
  @location(4) distance_from_centerline: f32,
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

  let line_segment_id: u32 = vertex.vertex_id / 6;
  let local_vertex_id: u32 = vertex.vertex_id % 6;

  var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let local_position = TRIANGLE_VERTS[local_vertex_id];

  let p = select(2 * line_segment_id, 2 * line_segment_id + 1, local_position.x == 1);

  // pull the point id
  let point_id = topology[p].point_id;
  // get CellID from vertex ID -> VTK cell map.
  let cell_id = topology[p].cell_id;

  var is_polyline_rl: bool = false; // whether polyline is going from right -> left in the topology buffer.
  var is_polyline_lr: bool = false; // whether polyline is going from left -> right in the topology buffer.
  var is_non_intersecting_vertex: bool = true;
  if (p > 0 && local_position.x == 0) {
    is_polyline_rl = topology[p - 1].cell_id == topology[p].cell_id;
  } else if (p < arrayLength(&topology) - 1 && local_position.x == 1) {
    is_polyline_lr = topology[p + 1].cell_id == topology[p].cell_id;
  }
  if (is_polyline_lr || is_polyline_rl) {
    let p0_vertex_id = select(p + 1, p - 1, is_polyline_lr);
    let p1_vertex_id = p;
    let p2_vertex_id = select(p - 2, p + 2, is_polyline_lr);

    var pos = local_position;

    if (local_position.x == 1) {
      pos = vec2(1.0 - local_position.x, -local_position.y);
    }

    let p0_point_id = topology[p0_vertex_id].point_id;
    let p1_point_id = topology[p1_vertex_id].point_id;
    let p2_point_id = topology[p2_vertex_id].point_id;

    let p0_mc = vec4<f32>(getTuple3F32(p0_point_id, mesh.position.start, &point_data.values), 1);
    let p1_mc = vec4<f32>(getTuple3F32(p1_point_id, mesh.position.start, &point_data.values), 1);
    let p2_mc = vec4<f32>(getTuple3F32(p2_point_id, mesh.position.start, &point_data.values), 1);

    // transform to view and then to clip space.
    let p0_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform * p0_mc;
    let p1_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform * p1_mc;
    let p2_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform * p2_mc;

    // transform to 2-D screen plane.
    let resolution = scene_transform.viewport.zw;
    let p0_screen = resolution * (0.5 * p0_dc.xy / p0_dc.w + 0.5);
    let p1_screen = resolution * (0.5 * p1_dc.xy / p1_dc.w + 0.5);
    let p2_screen = resolution * (0.5 * p2_dc.xy / p2_dc.w + 0.5);

    // Find the normal vector.
    let tangent = normalize(normalize(p2_screen - p1_screen) + normalize(p1_screen - p0_screen));
    let normal = vec2f(-tangent.y, tangent.x);

    // Find the vector perpendicular to p0_screen -> p1_screen.
    let p01 = p1_screen - p0_screen;
    let p21 = p1_screen - p2_screen;
    let p01_normal = normalize(vec2f(-p01.y, p01.x));

    // Determine the bend direction.
    let sigma = sign(dot(p01 + p21, normal));
    if (sign(pos.y) == sigma) {
      // This is an intersecting vertex. Adjust the position so that there's no overlap.
      let offset: vec2<f32> =  0.5 * width * -sigma * normal / dot(normal, p01_normal);
      if (length(offset) < min(length(p01), length(p21))) // clamp excessive offsets
      {
        let p_offsetted: vec2<f32> = p1_screen + offset;
        output.position = vec4<f32>(p1_dc.w * ((2.0 * p_offsetted) / resolution - 1.0), p1_dc.z, p1_dc.w);
        is_non_intersecting_vertex = false;
      }
    }
  }
  if (is_non_intersecting_vertex) {
    let p0_vertex_id: u32 = 2 * line_segment_id;
    let p1_vertex_id = p0_vertex_id + 1;

    let p0_point_id: u32 = topology[p0_vertex_id].point_id;
    let p1_point_id: u32 = topology[p1_vertex_id].point_id;

    let p0_mc = vec4(getTuple3F32(p0_point_id, mesh.position.start, &point_data.values), 1);
    let p1_mc = vec4(getTuple3F32(p1_point_id, mesh.position.start, &point_data.values), 1);

    // transform to view and then to clip space.
    let p0_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform* p0_mc;
    let p1_dc = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform* p1_mc;

    // transform to 2-D screen plane.
    let resolution = scene_transform.viewport.zw;
    let p0_screen = resolution * (0.5 * p0_dc.xy / p0_dc.w + 0.5);
    let p1_screen = resolution * (0.5 * p1_dc.xy / p1_dc.w + 0.5);

    let x_basis = normalize(p1_screen - p0_screen);
    let y_basis = vec2(-x_basis.y, x_basis.x);

    let p0_offsetted = p0_screen + local_position.x * x_basis + local_position.y * y_basis * width;
    let p1_offsetted = p1_screen + local_position.x * x_basis + local_position.y * y_basis * width;
    let p = mix(p0_offsetted, p1_offsetted, local_position.x);

    // used to select the z, w coordinate.
    let p_dc = mix(p0_dc, p1_dc, local_position.x);

    output.position = vec4(p_dc.w * ((2.0 * p) / resolution - 1.0), p_dc.z, p_dc.w);
  }
  output.position_vc = scene_transform.inverted_projection * output.position;
  output.distance_from_centerline = local_position.y;

  // Write indices
  output.cell_id = cell_id;
  output.prop_id = actor.color_options.id;
  output.composite_id = mesh.composite_id;
  output.process_id = 0u;

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

  let distance_from_centerline = abs(vertex.distance_from_centerline);

  // adjust z component of normal in order to emulate a tube if necessary.
  var normal_vc: vec3<f32> = normalize(vertex.normal_vc);
  let render_lines_as_tubes = getRenderLinesAsTubes(actor.render_options.flags);
  if (render_lines_as_tubes) {
    normal_vc.z = 1.0 - 2.0 * distance_from_centerline;
  }

  ambient_color = vertex.color.rgb;
  diffuse_color = vertex.color.rgb;
  opacity = vertex.color.a;

  ///------------------------///
  // Lights
  ///------------------------///
  if (actor.color_options.interpolation_type == VTK_FLAT && !render_lines_as_tubes) {
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
