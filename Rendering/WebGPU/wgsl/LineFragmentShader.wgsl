//-------------------------------------------------------------------
struct FragmentInput {
  @builtin(position) frag_coord: vec4<f32>,
  @builtin(front_facing) is_front_facing: bool,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>, // in view coordinate system.
  @location(2) normal_vc: vec3<f32>, // in view coordinate system.
  @location(3) tangent_vc: vec3<f32>, // in view coordinate system.
  @location(4) @interpolate(flat) cell_id: u32,
  @location(5) distance_from_centerline: f32,
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @location(0) color: vec4<f32>,
  @location(1) cell_id: u32
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(fragment: FragmentInput) -> FragmentOutput {
  var output: FragmentOutput;
  var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);

  var opacity: f32;

  let distance_from_centerline = abs(fragment.distance_from_centerline);

  // adjust z component of normal in order to emulate a tube if necessary.
  var normal_vc: vec3<f32> = normalize(fragment.normal_vc);
  let render_lines_as_tubes = getRenderLinesAsTubes(actor.render_options.flags);
  if (render_lines_as_tubes) {
    normal_vc.z = 1.0 - 2.0 * distance_from_centerline;
  }

  ///------------------------///
  // Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  ///------------------------///
  let has_mapped_colors: bool = mesh.point_color.num_tuples > 0u || mesh.cell_color.num_tuples > 0u;
  if (mesh.override_colors.apply_override_colors == 1u) {
    ambient_color = mesh.override_colors.ambient_color.rgb;
    diffuse_color = mesh.override_colors.diffuse_color.rgb;
    opacity = mesh.override_colors.opacity;
  } else if (has_mapped_colors) {
    ambient_color = fragment.color.rgb;
    diffuse_color = fragment.color.rgb;
    opacity = fragment.color.a;
  } else {
    ambient_color = actor.color_options.ambient_color;
    diffuse_color = actor.color_options.diffuse_color;
    opacity = actor.color_options.opacity;
  }

  ///------------------------///
  // Lights
  ///------------------------///
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
  // pre-multiply colors
  output.color = vec4(output.color.rgb * opacity, opacity);
  output.cell_id = fragment.cell_id;
  return output;
}
