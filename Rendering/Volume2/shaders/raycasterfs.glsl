#version 330 core

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates form vertex shader
in vec3 m_texture_coords;
in vec3 m_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
layout(location = 0) out vec4 m_frag_color;

//////////////////////////////////////////////////////////////////////////////
///
/// Attributes
///
//////////////////////////////////////////////////////////////////////////////

@BASE_ATTRIBUTES_FRAG@
@TERMINATION_ATTRIBUTES_FRAG@
@SHADING_ATTRIBUTES_FRAG@

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

@BASE_UNIFORMS_FRAG@
@TERMINATION_UNIFORMS_FRAG@
@SHADING_UNIFORMS_FRAG@

uniform float cropping_planes[6];
uniform int cropping_flags;

float cropping_planes_ts[6];

int computeRegionXCoord(vec3 pos)
{
  if (pos[0] > m_texture_extents_min[0] && pos[0] < cropping_planes_ts[0])
    {
    return 1;
    }
  else if (pos[0] > cropping_planes_ts[0] && pos[0] < cropping_planes_ts[1])
    {
    return 2;
    }
  else if (pos[0] > cropping_planes_ts[1] && pos[0] < m_texture_extents_max[0])
    {
    return 3;
    }
  return 0;
}

int computeRegionYCoord(vec3 pos)
{
  if (pos[1] > m_texture_extents_min[1] && pos[1] < cropping_planes_ts[2])
    {
    return 1;
    }
  else if (pos[1] > cropping_planes_ts[2] && pos[1]  < cropping_planes_ts[3])
    {
    return 2;
    }
  else if (pos[1] > cropping_planes_ts[3] && pos[1] < m_texture_extents_max[1])
    {
    return 3;
    }
  return 0;
}

int computeRegionZCoord(vec3 pos)
{
  if (pos[2] > m_texture_extents_min[2] && pos[2] < cropping_planes_ts[4])
    {
    return 1;
    }
  else if (pos[2] > cropping_planes_ts[4] && pos[2]  < cropping_planes_ts[5])
    {
    return 2;
    }
  else if (pos[2] > cropping_planes_ts[5] && pos[2] < m_texture_extents_max[2])
    {
    return 3;
    }
  return 0;
}

int computeRegion(vec3 pos)
{
  return (computeRegionXCoord(pos) * computeRegionYCoord(pos) *
          computeRegionZCoord(pos));
}

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Initialize m_frag_color (output) to 0
  m_frag_color = vec4(0.0);

  @BASE_INIT@
  @TERMINATE_INIT@
  @SHADING_INIT@

  /// Convert cropping region to texture space
  cropping_planes_ts[0] = cropping_planes[0] * m_step_size[0];
  cropping_planes_ts[1] = cropping_planes[1] * m_step_size[0];

  cropping_planes_ts[2] = cropping_planes[2] * m_step_size[1];
  cropping_planes_ts[3] = cropping_planes[3] * m_step_size[1];

  cropping_planes_ts[4] = cropping_planes[4] * m_step_size[2];
  cropping_planes_ts[5] = cropping_planes[5] * m_step_size[2];

  /// For all samples along the ray
  while (true)
    {
    @BASE_LOOP@
    @TERMINATE_LOOP@

    /// Determine region
    int regionNo = computeRegion(l_data_pos);
    regionNo = max(0, regionNo - 1);

    /// Do & operation with cropping flags
    if (((1 << regionNo) & cropping_flags) == 0)
      {
      /// Advance ray by m_dir_step
      l_data_pos += m_dir_step;

      continue;
      }

    @SHADING_LOOP@

    /// Advance ray by m_dir_step
    l_data_pos += m_dir_step;
    };


  @BASE_EXIT@
  @TERMINATE_EXIT@
  @SHADING_EXIT@
}

