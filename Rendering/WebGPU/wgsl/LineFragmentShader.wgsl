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

  ///------------------------///
  // Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  ///------------------------///
  let has_mapped_colors: bool = mesh.point_color.num_tuples > 0u || mesh.cell_color.num_tuples > 0u;
  if (mesh.apply_override_colors == 1u) {
    ambient_color = mesh.ambient_color.rgb;
    diffuse_color = mesh.diffuse_color.rgb;
    opacity = mesh.opacity;
  } else if (has_mapped_colors) {
    ambient_color = vertex.color.rgb;
    diffuse_color = vertex.color.rgb;
    opacity = vertex.color.a;
  } else {
    ambient_color = actor.color_options.ambient_color;
    diffuse_color = actor.color_options.diffuse_color;
    opacity = actor.color_options.opacity;
  }

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
