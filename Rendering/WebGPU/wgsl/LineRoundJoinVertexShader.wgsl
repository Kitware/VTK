// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"

/**
 * (0, 0.5) |-------------------------------|(1, 0.5)
 *         /|-                              |\
 *       /  |    -                          |  \
 *     /  \ |        -                      | /  \
 * (0, 0)---|              -                |-----|
 *     \  / |                   -           |    /
 *       \  |                        -      | \ /
 *         \|                              -|/
 * (0,-0.5) |-------------------------------|(1, -0.5)
 * The semicircle points are generated using this python snippet.
  import math
  def generate_instance_round_round(instanceRoundRound, resolution):
    for step in range(resolution):
      theta0 = math.pi / 2 + (step * math.pi) / resolution
      theta1 = math.pi / 2 + ((step + 1) * math.pi) / resolution
      instanceRoundRound.append([0, 0, 0])
      instanceRoundRound.append([0.5 * math.cos(theta0), 0.5 * math.sin(theta0), 0])
      instanceRoundRound.append([0.5 * math.cos(theta1), 0.5 * math.sin(theta1), 0])
    for step in range(resolution):
      theta0 = (3 * math.pi) / 2 + (step * math.pi) / resolution
      theta1 = (3 * math.pi) / 2 + ((step + 1) * math.pi) / resolution
      instanceRoundRound.append([0, 0, 1])
      instanceRoundRound.append([0.5 * math.cos(theta0), 0.5 * math.sin(theta0), 1])
      instanceRoundRound.append([0.5 * math.cos(theta1), 0.5 * math.sin(theta1), 1])
  instanceRoundRound = []
  resolution = 10  # example resolution
  generate_instance_round_round(instanceRoundRound, resolution)
  print(instanceRoundRound)
 */
const TRIANGLE_VERTS = array(
  vec3(0, -0.5, 0),
  vec3(0, -0.5, 1),
  vec3(0, 0.5, 1),
  vec3(0, -0.5, 0),
  vec3(0, 0.5, 1),
  vec3(0, 0.5, 0),
  // left semicircle
  vec3(0, 0, 0),
  vec3(3.061616997868383e-17, 0.5, 0),
  vec3(-0.2938926261462365, 0.4045084971874737, 0),
  vec3(0, 0, 0),
  vec3(-0.2938926261462365, 0.4045084971874737, 0),
  vec3(-0.47552825814757677, 0.15450849718747375, 0),
  vec3(0, 0, 0),
  vec3(-0.47552825814757677, 0.15450849718747375, 0),
  vec3(-0.4755282581475768, -0.15450849718747364, 0),
  vec3(0, 0, 0),
  vec3(-0.4755282581475768, -0.15450849718747364, 0),
  vec3(-0.2938926261462366, -0.40450849718747367, 0),
  vec3(0, 0, 0),
  vec3(-0.2938926261462366, -0.40450849718747367, 0),
  vec3(-9.184850993605148e-17, -0.5, 0),
  // right semicircle
  vec3(0, 0, 1),
  vec3(-9.184850993605148e-17, -0.5, 1),
  vec3(0.29389262614623646, -0.4045084971874738, 1),
  vec3(0, 0, 1),
  vec3(0.29389262614623646, -0.4045084971874738, 1),
  vec3(0.47552825814757677, -0.1545084971874738, 1),
  vec3(0, 0, 1),
  vec3(0.47552825814757677, -0.1545084971874738, 1),
  vec3(0.4755282581475768, 0.1545084971874736, 1),
  vec3(0, 0, 1),
  vec3(0.4755282581475768, 0.1545084971874736, 1),
  vec3(0.2938926261462367, 0.4045084971874736, 1),
  vec3(0, 0, 1),
  vec3(0.2938926261462367, 0.4045084971874736, 1),
  vec3(1.5308084989341916e-16, 0.5, 1)
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

  let p0_offsetted = p0_screen + (local_position.x * x_basis + local_position.y * y_basis) * width;
  let p1_offsetted = p1_screen + (local_position.x * x_basis + local_position.y * y_basis) * width;
  let p = mix(p0_offsetted, p1_offsetted, local_position.z);

  // used to select the z, w coordinate.
  let p_dc = mix(p0_dc, p1_dc, local_position.z);

  output.position = vec4(p_dc.w * ((2.0 * p) / resolution - 1.0), p_dc.z, p_dc.w);
  output.position_vc = scene_transform.inverted_projection * output.position;
  output.distance_from_centerline = local_position.y;

  let vertex_id = select(p0_vertex_id, p1_vertex_id, local_position.z == 1);
  // pull the point id
  let point_id = topology[vertex_id].point_id;
  // get CellID from vertex ID -> VTK cell map.
  let cell_id = topology[vertex_id].cell_id;

  // Write indices
  output.cell_id = cell_id;
  output.prop_id = actor.color_options.id;
  output.composite_id = mesh.composite_id;
  output.process_id = 2u;

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
