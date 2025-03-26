//-------------------------------------------------------------------
struct SceneTransform {
  // origin and dimensions of view area.
  viewport: vec4<f32>,
  // world space -> camera space
  view: mat4x4<f32>,
  // camera space -> clip space
  projection: mat4x4<f32>,
  // inverse of the transpose of view matrix
  normal: mat3x3<f32>,
  // clip-space -> camera-space
  inverted_projection: mat4x4<f32>
}

//-------------------------------------------------------------------
struct SceneLight {
  // 0 : deferred,
  // 1 : VTK_LIGHT_TYPE_HEADLIGHT,
  // 2 : VTK_LIGHT_TYPE_CAMERA_LIGHT,
  // 3 : VTK_LIGHT_TYPE_SCENE_LIGHT
  // FIXME: this shader does not implement lights other than headlight
  light_type: u32,
  // 0 : not positional,
  // 1 : positional
  positional: u32,
  cone_angle: f32,
  exponent: f32,
  color: vec3<f32>,
  direction_vc: vec3<f32>,
  position_vc: vec3<f32>,
  attenuation: vec3<f32>
}

//-------------------------------------------------------------------
struct SceneLights {
  count: u32,
  values: array<SceneLight>
}

//-------------------------------------------------------------------
struct ActorTransform {
  // model space -> world space
  world: mat4x4<f32>,
  // inverse of transposed world matrix
  normal: mat3x3<f32>
}

//-------------------------------------------------------------------
struct ActorRenderOptions {
  // 0 : VTK_POINTS
  // 1 : VTK_WIREFRAME
  // 2 : VTK_SURFACE
  representation: u32,
  // Point size in pixels - applicable when points are visible.
  point_size: f32,
  // Line width in pixels - applicable when edges are visible.
  line_width: f32,
  // Edge visibility - applicable for representation = VTK_SURFACE
  edge_visibility: u32
}

//-------------------------------------------------------------------
struct ActorShadeOptions {
  // Material ambient color intensity
  ambient_intensity: f32,
  // Material diffuse color intensity
  diffuse_intensity: f32,
  // Material specular color intensity
  specular_intensity: f32,
  // Material specular power
  specular_power: f32,
  // Opacity level
  opacity: f32,
  // 0: Global shading - global color for all primitives.
  // 1: Smooth shading - point based colors interpolated for in-between fragments.
  // 2: Flat shading - cell based colors
  shading_type: u32,
  // What kind of directional vectors are available to use for lighting?
  directional_mask: u32,
  // Material ambient color
  ambient_color: vec3<f32>,
  // Material diffuse color
  diffuse_color: vec3<f32>,
  // Material specular color
  specular_color: vec3<f32>,
  // Edge color
  edge_color: vec3<f32>
}

struct ActorBlock {
  transform: ActorTransform,
  render_options: ActorRenderOptions,
  shade_options: ActorShadeOptions
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
struct MeshAttributeDescriptor {
  position: MeshAttributeArrayDescriptor,
  @align(16) point_color: MeshAttributeArrayDescriptor,
  @align(16) point_normal: MeshAttributeArrayDescriptor,
  @align(16) point_tangent: MeshAttributeArrayDescriptor,
  @align(16) point_uv: MeshAttributeArrayDescriptor,
  @align(16) cell_color: MeshAttributeArrayDescriptor,
  @align(16) cell_normal: MeshAttributeArrayDescriptor,
  @align(16) cell_edge_array: MeshAttributeArrayDescriptor
}

//-------------------------------------------------------------------
struct Topology {
  // the vtk cell ID for this index. used to index into a cell attribute.
  cell_id: u32,
  // the vtk point ID for this index. used to index into a point attribute.
  point_id: u32,
  // unused right now
  // cell_type: u32,
  // unused right now
  // cell_size: u32
}

///-----------------------------------------------------------------///
// Renderer controlled descriptor set
///-----------------------------------------------------------------///
@group(0) @binding(0) var<uniform> scene_transform: SceneTransform;
@group(0) @binding(1) var<storage, read> scene_lights: SceneLights;

///-------------------------------------------------------------------
// Everything shader needs from the vtkActor and it's vtkProperty
///-------------------------------------------------------------------
@group(1) @binding(0) var<uniform> actor: ActorBlock;

///-----------------------------------------------------------------///
// Mesh attributes.
///-----------------------------------------------------------------///
@group(2) @binding(0) var<uniform> mesh: MeshAttributeDescriptor;
@group(2) @binding(1) var<storage, read> point_data: F32s;
@group(2) @binding(2) var<storage, read> cell_data: F32s;

///-----------------------------------------------------------------///
// Topology
///-----------------------------------------------------------------///
@group(3) @binding(0) var<uniform> primitive_size: u32;
@group(3) @binding(1) var<storage, read> topology: array<Topology>;

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
  @location(4) edge_dists: vec3<f32>,
  @location(5) @interpolate(flat) hide_edge: f32,
  @location(6) @interpolate(flat) cell_id: u32,
  @location(7) @interpolate(flat) primitive_size: u32
}

//-------------------------------------------------------------------
@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput {
    var output: VertexOutput;
  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
    var vertex_id_denominator: u32 = 1u;
    if primitive_size == 1u {
        if actor.render_options.representation == 0u {
            vertex_id_denominator = 6u;
        } else if actor.render_options.representation == 1u {
            vertex_id_denominator = 1u; // invalid (should never be here, points cannot be represented as lines)
        } else if actor.render_options.representation == 2u {
            vertex_id_denominator = 6u;
        }
    } else if primitive_size == 2u {
        if actor.render_options.representation == 0u {
            vertex_id_denominator = 6u;
        }
    } else if primitive_size == 3u {
        if actor.render_options.representation == 0u {
            vertex_id_denominator = 6u;
        }
    }
    output.primitive_size = primitive_size;
    let pull_vertex_id: u32 = vertex.vertex_id / vertex_id_denominator;
  // get CellID from vertex ID -> VTK cell map.
    output.cell_id = topology[pull_vertex_id].cell_id;
  // pull the point id
    let point_id: u32 = topology[pull_vertex_id].point_id;
  // pull the position for this vertex.
    var vertex_mc: vec4<f32> = vec4<f32>(
        point_data.values[3u * point_id + 0u + mesh.position.start],
        point_data.values[3u * point_id + 1u + mesh.position.start],
        point_data.values[3u * point_id + 2u + mesh.position.start],
        1.0
    );

  ///------------------------///
  // NDC transforms
  ///------------------------///
    output.position_vc = scene_transform.view * actor.transform.world * vertex_mc;
    output.position = scene_transform.projection * output.position_vc;

  ///------------------------///
  // PrimitivtType:  VTK_POINT
  // OR
  // Representation: VTK_POINTS
  ///------------------------///
    if primitive_size == 1u || actor.render_options.representation == 0u {
    // the four corners of a quad, constructed from two triangles,
    // stacked along the longest side.
        let offset_multipliers: array<vec2<f32>, 6> = array<vec2<f32>, 6>(
            vec2<f32>(-1., -1.), // 0
            vec2<f32>(1., -1.),  // 1
            vec2<f32>(1., 1.),   // 2
            vec2<f32>(-1., -1.), // 0
            vec2<f32>(1., 1.),   // 2
            vec2<f32>(-1., 1.)   // 3
        );
    // into NDC space [-1, 1]
        let position_ndc_2d = output.position.xy / output.position.w;
    // an index into offset_multipliers
        let local_offset_id: u32 = vertex.vertex_id % 6u;
        output.cell_id = point_id;
    // fetch an offset
        var local_offset: vec2<f32>;
        if local_offset_id == 0u {
            local_offset = offset_multipliers[0];
        } else if local_offset_id == 1u {
            local_offset = offset_multipliers[1];
        } else if local_offset_id == 2u {
            local_offset = offset_multipliers[2];
        } else if local_offset_id == 3u {
            local_offset = offset_multipliers[3];
        } else if local_offset_id == 4u {
            local_offset = offset_multipliers[4];
        } else if local_offset_id == 5u {
            local_offset = offset_multipliers[5];
        }
        var point_size: vec2<f32> = vec2<f32>(actor.render_options.point_size, actor.render_options.point_size);
    // squish the 'point size' value given as number of pixels into NDC space.
    // This done by scaling it from viewport space -> NDC space.
        point_size = point_size * 0.5 / scene_transform.viewport.zw;
    // push the vertex in a suitable direction while we're still in the NDC space.
        var pushed_vertex: vec2<f32> = position_ndc_2d + point_size * local_offset;
    // undo perspective division, so that vertex shader output is unaware of our tricks.
        pushed_vertex.x = pushed_vertex.x * output.position.w;
        pushed_vertex.y = pushed_vertex.y * output.position.w;
    // update this shader function's output position with the pushed vertex coordinate.
        output.position = vec4<f32>(pushed_vertex.xy, output.position.zw);
        output.position_vc = scene_transform.inverted_projection * output.position;
    }

  ///------------------------///
  // PrimitivtType:  VTK_LINE
  // AND
  // Representation: VTK_WIREFREAME OR VTK_SURFACE
  ///------------------------///
    if primitive_size == 2u && actor.render_options.representation != 0u {
        let line_width: f32 = actor.render_options.line_width;
        let line_width_step_size = line_width / ceil(line_width);
        let distance: f32 = f32(vertex.instance_id / 2u) * line_width_step_size - (0.5 * line_width);
        let offset: vec2<f32> = vec2<f32>(
            2.0 * (f32(vertex.instance_id) % 2.0) * distance / scene_transform.viewport.z,
            2.0 * (f32(vertex.instance_id + 1u) % 2.0) * distance / scene_transform.viewport.w,
        );
        let position_ndc_2d = output.position.xy / output.position.w;
        var pushed_vertex: vec2<f32> = position_ndc_2d + offset;
    // undo perspective division, so that vertex shader output is unaware of our tricks.
        pushed_vertex.x = pushed_vertex.x * output.position.w;
        pushed_vertex.y = pushed_vertex.y * output.position.w;
    // update this shader function's output position with the pushed vertex coordinate.
        output.position = vec4<f32>(pushed_vertex.xy, output.position.zw);
        output.position_vc = scene_transform.inverted_projection * output.position;
    }

  ///------------------------///
  // PrimitivtType:  VTK_TRIANGLE
  // Representation:
  // VTK_SURFACE + Edges OR VTK_WIREFRAME
  ///------------------------///
    let wireframe: bool = (actor.render_options.representation == 1u);
    let surface_plus_edges: bool = (actor.render_options.representation == 2u && actor.render_options.edge_visibility == 1u);
    if primitive_size == 3u && (wireframe || surface_plus_edges) {
        let triangle_id: u32 = pull_vertex_id / 3u;
        let i0 = triangle_id * 3u;
        let pt0 = topology[i0].point_id;
        let pt1 = topology[i0 + 1u].point_id;
        let pt2 = topology[i0 + 2u].point_id;
        let p0_mc: vec4<f32> = vec4<f32>(
            point_data.values[3u * pt0 + 0u + mesh.position.start],
            point_data.values[3u * pt0 + 1u + mesh.position.start],
            point_data.values[3u * pt0 + 2u + mesh.position.start],
            1.0
        );
        let p1_mc: vec4<f32> = vec4<f32>(
            point_data.values[3u * pt1 + 0u + mesh.position.start],
            point_data.values[3u * pt1 + 1u + mesh.position.start],
            point_data.values[3u * pt1 + 2u + mesh.position.start],
            1.0
        );
        let p2_mc: vec4<f32> = vec4<f32>(
            point_data.values[3u * pt2 + 0u + mesh.position.start],
            point_data.values[3u * pt2 + 1u + mesh.position.start],
            point_data.values[3u * pt2 + 2u + mesh.position.start],
            1.0
        );
        let mvp = scene_transform.projection * scene_transform.view * actor.transform.world;
        let p0_3d: vec4<f32> = mvp * p0_mc;
        let p1_3d: vec4<f32> = mvp * p1_mc;
        let p2_3d: vec4<f32> = mvp * p2_mc;
        let p0: vec2<f32> = p0_3d.xy / p0_3d.w;
        let p1: vec2<f32> = p1_3d.xy / p1_3d.w;
        let p2: vec2<f32> = p2_3d.xy / p2_3d.w;
        let use_id: u32 = pull_vertex_id % 3u;
        let win_scale = scene_transform.viewport.zw * 0.5;
        let edge_value: f32 = cell_data.values[triangle_id + mesh.cell_edge_array.start];
        if use_id == 0u {
            let v10 = win_scale * (p1 - p0);
            let v20 = win_scale * (p2 - p0) ;
            let area0: f32 = abs(v10.x * v20.y - v10.y * v20.x);
            let h0: f32 = area0 / length(v10 - v20);
            output.edge_dists = vec3<f32>(h0 * p0_3d.w, 0.0, 0.0);
        } else if use_id == 1u {
            let v01 = win_scale * (p0 - p1);
            let v21 = win_scale * (p2 - p1) ;
            let area1: f32 = abs(v01.x * v21.y - v01.y * v21.x);
            let h1: f32 = area1 / length(v01 - v21);
            output.edge_dists = vec3<f32>(0.0, h1 * p1_3d.w, 0.0);
        } else if use_id == 2u {
            let v02 = win_scale * (p0 - p2);
            let v12 = win_scale * (p1 - p2) ;
            let area2: f32 = abs(v02.x * v12.y - v02.y * v12.x);
            let h2: f32 = area2 / length(v02 - v12);
            output.edge_dists = vec3<f32>(0.0, 0.0, h2 * p2_3d.w);
        }
        output.hide_edge = edge_value;
    }

    if actor.shade_options.shading_type == 1u {
    ///------------------------///
    // Smooth shading
    // A vertex color is pulled
    // out from the `point_data`
    // The in-between fragments
    // receive a smooth interpolated
    // color.
    ///------------------------///
        output.color = vec4<f32>(
            point_data.values[4u * point_id + 0u + mesh.point_color.start],
            point_data.values[4u * point_id + 1u + mesh.point_color.start],
            point_data.values[4u * point_id + 2u + mesh.point_color.start],
            point_data.values[4u * point_id + 3u + mesh.point_color.start],
        );
    } else if actor.shade_options.shading_type == 2u {
    ///------------------------///
    // Flat shading
    // A cell color is pulled
    // out from the `cell_data`
    // The in-between fragments
    // receive same color as the vertices.
    ///------------------------///
        output.color = vec4<f32>(
            cell_data.values[4u * output.cell_id + 0u + mesh.cell_color.start],
            cell_data.values[4u * output.cell_id + 1u + mesh.cell_color.start],
            cell_data.values[4u * output.cell_id + 2u + mesh.cell_color.start],
            cell_data.values[4u * output.cell_id + 3u + mesh.cell_color.start]
        );
    }

  ///------------------------///
  // Directionals
  // Basically infers what kind of
  // directional vectors are available
  // to light up the geometry.
  // FIXME: needs better definition.
  // 1 << 0 No normals
  // 1 << 1 point normals
  // 1 << 2 point tangents
  // 1 << 3 cell normals
  ///------------------------///
    if (actor.shade_options.directional_mask & (1u << 3u)) == (1u << 3u) {
    // pull normal of this vertex from cell normals
        let normal_mc = vec3<f32>(
            cell_data.values[output.cell_id + mesh.cell_normal.start],
            cell_data.values[output.cell_id + mesh.cell_normal.start],
            cell_data.values[output.cell_id + mesh.cell_normal.start]
        );
        output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
    } else if (actor.shade_options.directional_mask & (1u << 2u)) == (1u << 2u) {
    // pull tangent of this vertex from point tangents
        let tangentMC = vec3<f32>(
            point_data.values[3u * point_id + 0u + mesh.point_tangent.start],
            point_data.values[3u * point_id + 1u + mesh.point_tangent.start],
            point_data.values[3u * point_id + 2u + mesh.point_tangent.start]
        );
        output.tangent_vc = scene_transform.normal * actor.transform.normal * tangentMC;
    } else if (actor.shade_options.directional_mask & (1u << 1u)) == (1u << 1u) {
    // this if is after cell normals, so that when both are available, point normals are used.
    // pull normal of this vertex from point normals
        let normal_mc = vec3<f32>(
            point_data.values[3u * point_id + 0u + mesh.point_normal.start],
            point_data.values[3u * point_id + 1u + mesh.point_normal.start],
            point_data.values[3u * point_id + 2u + mesh.point_normal.start]
        );
        output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
    } else if (actor.shade_options.directional_mask & (1u << 0u)) == (1u << 0u) {
    // let's calculate normal for this primitive.
        if actor.render_options.representation == 0u || primitive_size == 1u {
            output.normal_vc = vec3<f32>(0.0, 0.0, 1.0);
        } else if primitive_size == 2u {
            let normal_mc = vec3<f32>(0.0, 0.0, 1.0);
            output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
        } else if primitive_size == 3u {
            let next_id: u32 = (pull_vertex_id + 1u) % 3u;
            let prev_id: u32 = (pull_vertex_id + 2u) % 3u;
            let triangle_id: u32 = pull_vertex_id / 3u;
            let next_pt_id = topology[triangle_id * 3u + next_id].point_id;
            let prev_pt_id = topology[triangle_id * 3u + prev_id].point_id;
            let next_mc: vec3<f32> = vec3<f32>(
                point_data.values[3u * next_pt_id + 0u + mesh.position.start],
                point_data.values[3u * next_pt_id + 1u + mesh.position.start],
                point_data.values[3u * next_pt_id + 2u + mesh.position.start]
            );
            let prev_mc: vec3<f32> = vec3<f32>(
                point_data.values[3u * prev_pt_id + 0u + mesh.position.start],
                point_data.values[3u * prev_pt_id + 1u + mesh.position.start],
                point_data.values[3u * prev_pt_id + 2u + mesh.position.start]
            );
            let ab: vec3<f32> = next_mc.xyz - vertex_mc.xyz;
            let ac: vec3<f32> = prev_mc.xyz - vertex_mc.xyz;
            let normal_mc = cross(ab, ac);
            output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
        }
    }
    return output;
}

//-------------------------------------------------------------------
struct FragmentInput {
  @builtin(position) frag_coord: vec4<f32>,
  @builtin(front_facing) is_front_facing: bool,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>, // in view coordinate system.
  @location(2) normal_vc: vec3<f32>, // in view coordinate system.
  @location(3) tangent_vc: vec3<f32>, // in view coordinate system.
  @location(4) edge_dists: vec3<f32>,
  @location(5) @interpolate(flat) hide_edge: f32,
  @location(6) @interpolate(flat) cell_id: u32,
  @location(7) @interpolate(flat) primitive_size: u32
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @location(0) color: vec4<f32>,
  @location(1) cell_id: u32
  // As per WebGPU spec, it doesn't matter if fragment shader writes
  // into non existent render targets.
  // @location(2) position_vc: vec4<f32>, // in view coordinate system.
  // @location(3) normal_vc: vec3<f32>, // in view coordinate system.
  // @location(4) tangent_vc: vec3<f32>, // in view coordinate system.
  // @location(5) specular: vec4<f32>, // specular_color.rgb, specular_power

}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(fragment: FragmentInput) -> FragmentOutput {
    var output: FragmentOutput;
    var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);
    var normal_vc: vec3<f32> = normalize(fragment.normal_vc);

    var opacity: f32;

  ///------------------------///
  // Colors are acquired
  // either from a global per-actor color,
  // or from per-vertex colors,
  // or from cell colors.
  ///------------------------///
    if actor.shade_options.shading_type == 0u {
        ambient_color = actor.shade_options.ambient_color;
        diffuse_color = actor.shade_options.diffuse_color;
        opacity = actor.shade_options.opacity;
    } else {
        ambient_color = fragment.color.rgb;
        diffuse_color = fragment.color.rgb;
        opacity = fragment.color.a;
    }

  ///------------------------///
  // PrimitivtType:  VTK_TRIANGLE
  // Representation:
  // VTK_SURFACE + Edges OR VTK_WIREFRAME
  ///------------------------///
    let wireframe: bool = (actor.render_options.representation == 1u);
    let surface_plus_edges: bool = (actor.render_options.representation == 2u && actor.render_options.edge_visibility == 1u);
    let linewidth: f32 = actor.render_options.line_width;
    if fragment.primitive_size == 3u && (wireframe || surface_plus_edges) {
        // Undo perspective correction.
        let dist_vec = fragment.edge_dists.xyz * fragment.frag_coord.w;
        var d: f32 = 0.0;
        // Compute the shortest distance to the edge
        if fragment.hide_edge == 2.0 {
            d = min(dist_vec[0], dist_vec[2]);
        } else if fragment.hide_edge == 1.0 {
            d = dist_vec[0];
        } else if fragment.hide_edge == 0.0 {
            d = min(dist_vec[0], dist_vec[1]);
        } else { // no edge is hidden
            d = min(dist_vec[0], min(dist_vec[1], dist_vec[2]));
        }
        if wireframe {
            if d > linewidth { // NOT 0.5*linewidth as it draws very thin lines.
              discard;
            } else {
                // when wireframe is rendered, we do not color the wire with edge color.
                // let I: f32 = exp2(-2.0 * d * d);
                // diffuse_color = mix(vec3(0.0, 0.0, 0.0), actor.shade_options.edge_color, I);
                // ambient_color = mix(vec3(0.0, 0.0, 0.0), actor.shade_options.edge_color, I);
            }
        } else {
            let I: f32 = exp2(-2.0 * d * d / (linewidth * linewidth));
            diffuse_color = mix(diffuse_color, actor.shade_options.edge_color, I);
            ambient_color = mix(ambient_color, actor.shade_options.edge_color, I);
        }
    }

  ///------------------------///
  // Normals
  ///------------------------///
    if !fragment.is_front_facing {
        normal_vc = -fragment.normal_vc;
        normal_vc = normalize(normal_vc);
    } else if normal_vc.z < 0.0 {
        normal_vc.z = -normal_vc.z;
    }

  ///------------------------///
  // Lights
  ///------------------------///
    if scene_lights.count == 0u {
    // using G-buffer
        output.color = vec4<f32>(
            actor.shade_options.ambient_intensity * ambient_color + actor.shade_options.diffuse_intensity * diffuse_color,
            opacity
        );
    } else if scene_lights.count == 1u {
        let light: SceneLight = scene_lights.values[0];
        if light.positional == 1u {
      // TODO: positional
            output.color = vec4<f32>(
                actor.shade_options.ambient_intensity * ambient_color + actor.shade_options.diffuse_intensity * diffuse_color,
                opacity
            );
        } else {
      // headlight
            let df: f32 = max(0.000001f, normal_vc.z);
            let sf: f32 = pow(df, actor.shade_options.specular_power);
            diffuse_color = df * diffuse_color * light.color;
            specular_color = sf * actor.shade_options.specular_intensity * actor.shade_options.specular_color * light.color;
            output.color = vec4<f32>(
                actor.shade_options.ambient_intensity * ambient_color + actor.shade_options.diffuse_intensity * diffuse_color + specular_color,
                opacity
            );
        }
    } else {
  // TODO: light kit
        output.color = vec4<f32>(
            actor.shade_options.ambient_intensity * ambient_color + actor.shade_options.diffuse_intensity * diffuse_color,
            opacity
        );
    }
    output.cell_id = fragment.cell_id;
    return output;
}
