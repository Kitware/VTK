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
struct MeshAttributeArrayDescriptor {
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct MeshDescriptor {
  position: MeshAttributeArrayDescriptor,
  point_color: MeshAttributeArrayDescriptor,
  point_normal: MeshAttributeArrayDescriptor,
  point_tangent: MeshAttributeArrayDescriptor,
  point_uv: MeshAttributeArrayDescriptor,
  cell_color: MeshAttributeArrayDescriptor,
  cell_normal: MeshAttributeArrayDescriptor,
  apply_override_colors: u32,
  opacity: f32,
  composite_id: u32,
  ambient_color: vec3<f32>,
  process_id: u32,
  diffuse_color: vec3<f32>,
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
  let line_segment_id: u32 = vertex.instance_id;

  var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let local_position = TRIANGLE_VERTS[vertex.vertex_id];

  let p = select(2 * line_segment_id, 2 * line_segment_id + 1, local_position.x == 1);

  // pull the point id
  let point_id = topology[p].point_id;
  // get CellID from vertex ID -> VTK cell map.
  let cell_id = topology[p].cell_id;

  // Write indices
  output.cell_id = cell_id;
  output.prop_id = actor.color_options.id;
  output.composite_id = mesh.composite_id;
  output.process_id = 2u;

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
    let p0_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p0_mc;
    let p1_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p1_mc;
    let p2_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p2_mc;

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
    let p0_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p0_mc;
    let p1_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p1_mc;

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

  ///------------------------///
  // Smooth/Flag shading
  ///------------------------///
  if mesh.point_color.num_tuples > 0 {
    // Smooth shading
    output.color = getTuple4F32(point_id, mesh.point_color.start, &point_data.values);
  } else if mesh.cell_color.num_tuples > 0u {
    // Flat shading
    output.color = getTuple4F32(cell_id, mesh.cell_color.start, &cell_data.values);
  }

  ///------------------------///
  // Set Normals/Tangents
  // Basically infers what kind of directional vectors are available to use in lighting calculations.
  ///------------------------///
  if mesh.cell_normal.num_tuples > 0u {
    // pull normal of this vertex from cell normals
    let normal_mc = getTuple3F32(cell_id, mesh.cell_normal.start, &cell_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
  } else if mesh.point_tangent.num_tuples > 0u {
    // pull tangent of this vertex from point tangents
    let tangentMC = getTuple3F32(point_id, mesh.point_tangent.start, &point_data.values);
    output.tangent_vc = scene_transform.normal * actor.transform.normal * tangentMC;
  } else if mesh.point_normal.num_tuples > 0u {
    // this if is after cell normals, so that when both are available, point normals are used.
    // pull normal of this vertex from point normals
    let normal_mc = getTuple3F32(point_id, mesh.point_normal.start, &point_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
  } else {
    output.normal_vc = vec3<f32>(0.0, 0.0, 1.0);
  }
  return output;
}
