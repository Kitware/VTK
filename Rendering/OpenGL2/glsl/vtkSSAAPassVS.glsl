//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSSAAPassVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

attribute vec4 vertexMC;
attribute vec2 tcoordMC;

// Note that the texel offsets should be 3/8 of a pixel in the
// resulting image not the source image. Also note that this
// filter is meant to be run one dimension at a time.
uniform float texelWidthOffset;
uniform float texelHeightOffset;

varying vec2 centerTextureCoordinate;
varying vec2 oneStepLeftTextureCoordinate;
varying vec2 twoStepsLeftTextureCoordinate;
varying vec2 threeStepsLeftTextureCoordinate;
varying vec2 fourStepsLeftTextureCoordinate;
varying vec2 oneStepRightTextureCoordinate;
varying vec2 twoStepsRightTextureCoordinate;
varying vec2 threeStepsRightTextureCoordinate;
varying vec2 fourStepsRightTextureCoordinate;

void main()
{
  vec2 firstOffset = vec2(texelWidthOffset, texelHeightOffset);
  vec2 secondOffset = vec2(2.0 * texelWidthOffset, 2.0 * texelHeightOffset);
  vec2 thirdOffset = vec2(3.0 * texelWidthOffset, 3.0 * texelHeightOffset);
  vec2 fourthOffset = vec2(4.0 * texelWidthOffset, 4.0 * texelHeightOffset);

  centerTextureCoordinate = tcoordMC;
  oneStepLeftTextureCoordinate = tcoordMC - firstOffset;
  twoStepsLeftTextureCoordinate = tcoordMC - secondOffset;
  threeStepsLeftTextureCoordinate = tcoordMC - thirdOffset;
  fourStepsLeftTextureCoordinate = tcoordMC - fourthOffset;
  oneStepRightTextureCoordinate = tcoordMC + firstOffset;
  twoStepsRightTextureCoordinate = tcoordMC + secondOffset;
  threeStepsRightTextureCoordinate = tcoordMC + thirdOffset;
  fourStepsRightTextureCoordinate = tcoordMC + fourthOffset;

  gl_Position = vertexMC;
}
