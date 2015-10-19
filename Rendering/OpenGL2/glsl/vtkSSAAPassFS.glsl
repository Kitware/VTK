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

uniform sampler2D source;

// the output of this shader
//VTK::Output::Dec


varying vec2 centerTextureCoordinate;
varying vec2 oneStepLeftTextureCoordinate;
varying vec2 twoStepsLeftTextureCoordinate;
varying vec2 threeStepsLeftTextureCoordinate;
varying vec2 fourStepsLeftTextureCoordinate;
varying vec2 oneStepRightTextureCoordinate;
varying vec2 twoStepsRightTextureCoordinate;
varying vec2 threeStepsRightTextureCoordinate;
varying vec2 fourStepsRightTextureCoordinate;

// Note that the texel offsets should be 3/8 of a pixel in the
// resulting image not the source image. Also note that this
// filter is meant to be run one dimension at a time.
// in the equation below 1.5 corresponds to 4 texel offsets
// aka 3/8 * 4 = 1.5

// sinc(x) * sinc(x/a) = (a * sin(pi * x) * sin(pi * x / a)) / (pi^2 * x^2)
// Assuming a Lanczos constant of 2.0, and scaling values to max out at x = +/- 1.5

void main()
{
  lowp vec4 fragmentColor = texture2D(source, centerTextureCoordinate) * 0.38026;

  fragmentColor += texture2D(source, oneStepLeftTextureCoordinate) * 0.27667;
  fragmentColor += texture2D(source, oneStepRightTextureCoordinate) * 0.27667;

  fragmentColor += texture2D(source, twoStepsLeftTextureCoordinate) * 0.08074;
  fragmentColor += texture2D(source, twoStepsRightTextureCoordinate) * 0.08074;

  fragmentColor += texture2D(source, threeStepsLeftTextureCoordinate) * -0.02612;
  fragmentColor += texture2D(source, threeStepsRightTextureCoordinate) * -0.02612;

  fragmentColor += texture2D(source, fourStepsLeftTextureCoordinate) * -0.02143;
  fragmentColor += texture2D(source, fourStepsRightTextureCoordinate) * -0.02143;

  gl_FragData[0] = fragmentColor;
}