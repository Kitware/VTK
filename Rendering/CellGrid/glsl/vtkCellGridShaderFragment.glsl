//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//VTK::Camera::Dec

//VTK::Output::Dec

// Position of vertex in view coordinates.
//VTK::PositionVC::Dec

// The normal of the output primitive in view coordinates.
//VTK::Normal::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

// handle coincident offsets
//VTK::Coincident::Dec

// Value raster
//VTK::ValuePass::Dec

// Lights
//VTK::Light::Dec

{commonDefs}
{cellEval}
{cellUtil}

smooth in vec3 pcoordVSOutput;

uniform sampler2D color_map;
uniform vec3 color_range;
uniform int color_component;
// basic material shading capabilities
uniform vec3 color_ambient;
uniform vec3 color_diffuse;
uniform vec3 color_specular;
uniform vec3 color_ambient_bf; // backface
uniform vec3 color_diffuse_bf; // backface
uniform vec3 color_specular_bf; // backface
uniform float intensity_ambient;
uniform float intensity_diffuse;
uniform float intensity_specular;
uniform float intensity_opacity;
uniform float intensity_ambient_bf; // backface
uniform float intensity_diffuse_bf; // backface
uniform float intensity_specular_bf; // backface
uniform float intensity_opacity_bf; // backface
uniform float power_specular;
uniform float power_specular_bf; // backface
uniform int enable_specular;

flat in float shapeValuesVSOutput[{ShapeCoeffPerCell}];
flat in float colorValuesVSOutput[{ColorCoeffPerCell}];

void main()
{{
  //VTK::PositionVC::Impl

  // Place any calls that require uniform flow (e.g. dFdx) here.
  //VTK::UniformFlow::Impl

  // Set gl_FragDepth here (gl_FragCoord.z by default)
  //VTK::Depth::Impl

  // Early depth peeling abort:
  //VTK::DepthPeeling::PreColor

  float fieldValue[{ColorNumValPP}];
  float scalar; // The non-normalized scalar value computed from a fieldValue tuple.
  vec2 texCoord; // Used for color lookup.

  if ({HaveColors})
  {{
    // Evaluate the basis function at this fragment's parametric coords (pcoordVSOutput),
    // yielding the \a fieldValue tuple.
    colorEvaluateAt(pcoordVSOutput, shapeValuesVSOutput, colorValuesVSOutput, fieldValue);

    // Choose how we map the \a fieldValue tuple into a texture-map coordinate.
    switch (color_component)
    {{
    case -2:
      // L₂ norm:
      {{
        float mag = 0.0;
        for (int cc = 0; cc < {ColorNumValPP}; ++cc)
        {{
          mag += fieldValue[cc] * fieldValue[cc];
        }}
        scalar = sqrt(mag);
      }}
    case -1:
      // L₁ norm (choose the maximum across components):
      {{
        scalar = fieldValue[0];
        for (int cc = 1; cc < {ColorNumValPP}; ++cc)
        {{
          if (fieldValue[cc] > scalar)
          {{
            scalar = fieldValue[cc];
          }}
        }}
      }}
    default:
      // Choose a single component.
      scalar = fieldValue[color_component];
    }}
    if (color_range[2] > 0.0)
    {{
      texCoord = vec2((scalar - color_range[0]) / color_range[2], 0.0);
    }}
    else
    {{
      texCoord = vec2((scalar - color_range[0]), 0.0);
    }}
    // texCoord = vec2(colorValuesVSOutput[3], 0.);
  }}

  vec4 color;
  vec3 ambientColor;
  vec3 diffuseColor;
  vec3 specularColor;
  float opacity;
  float specularPower = 0.0f;
  if ({HaveColors})
  {{
    color = texture(color_map, texCoord);
    // color = vec4((colorValuesVSOutput[0] - color_range[0])/color_range[2], (colorValuesVSOutput[1] - color_range[0])/color_range[2], (colorValuesVSOutput[2] - color_range[0])/color_range[2], 1.0);
    // color = vec4(texCoord.s, texCoord.s, texCoord.s, 1.0);
    ambientColor = intensity_ambient * color.rgb;
    diffuseColor = intensity_diffuse * color.rgb;
    specularColor = intensity_specular * color.rgb;
    opacity = intensity_opacity * color.a;
  }}
  else
  {{
    color = vec4(1, 1, 1, 1);
    // FIXME: gl_FrontFacing cannot be relied upon for some reason.
    //        1. Dual depth peeling works and sometimes doesn't.
    //        2. Backfaces are not rendered.
    // if (gl_FrontFacing == false)
    // {{
    //   ambientColor = intensity_ambient_bf * color_ambient_bf;
    //   diffuseColor = intensity_diffuse_bf * color_diffuse_bf;
    //   specularColor = intensity_specular_bf * color_specular_bf;
    //   opacity = intensity_opacity_bf;
    //   specularPower = power_specular_bf;
    // }}
    // else
    {{
      ambientColor = intensity_ambient * color_ambient;
      diffuseColor = intensity_diffuse * color_diffuse;
      specularColor = intensity_specular * color_specular;
      opacity = intensity_opacity;
      specularPower = power_specular;
    }}
  }}

  //VTK::Normal::Impl

  //VTK::Light::Impl

  // Discard fully transparent pixels
  if (gl_FragData[0].a <= 0.0) discard;

  //VTK::DepthPeeling::Impl
}}
