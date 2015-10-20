//VTK::System::Dec

// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSSAAPassFS.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// thanks to Brad Larson for posting sample code that helped get this started

uniform sampler2D source;

// the output of this shader
//VTK::Output::Dec

uniform float texelWidthOffset;
uniform float texelHeightOffset;

varying vec2 tcoordVC;

// Note that the texel offsets should be 3/8 of a pixel in the
// resulting image not the source image. Also note that this
// filter is meant to be run one dimension at a time.
// in the equation below 1.5 corresponds to 4 texel offsets
// aka 3/8 * 4 = 1.5

// sinc(x) * sinc(x/a) = (a * sin(pi * x) * sin(pi * x / a)) / (pi^2 * x^2)
// Assuming a Lanczos constant of 2.0, and scaling values to max out at x = +/- 1.5

void main()
{
  vec2 firstOffset = vec2(texelWidthOffset, texelHeightOffset);

  vec4 fragmentColor = texture2D(source, tcoordVC) * 0.38026;

  fragmentColor += texture2D(source, tcoordVC - firstOffset) * 0.27667;
  fragmentColor += texture2D(source, tcoordVC + firstOffset) * 0.27667;

  fragmentColor += texture2D(source, tcoordVC - 2.0*firstOffset) * 0.08074;
  fragmentColor += texture2D(source, tcoordVC + 2.0*firstOffset) * 0.08074;

  fragmentColor += texture2D(source, tcoordVC - 3.0*firstOffset) * -0.02612;
  fragmentColor += texture2D(source, tcoordVC + 3.0*firstOffset) * -0.02612;

  fragmentColor += texture2D(source, tcoordVC - 4.0*firstOffset) * -0.02143;
  fragmentColor += texture2D(source, tcoordVC + 4.0*firstOffset) * -0.02143;

  gl_FragData[0] = fragmentColor;
}
