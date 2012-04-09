//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// Filename: vtkLineIntegralConvolution2D_fs.glsl
// Filename is useful when using gldb-gui

// Provides a set of methods that the shaders can use.
 
#version 110

uniform sampler2D texVectorField; // TEXTURE0
uniform sampler2D texNoise;       // TEXTURE1

uniform vec2 uNoise2VecScaling;   // scale = vector / noise
uniform vec2 uVectorTransform2;
uniform vec2 uVectorShiftScale;
uniform vec4 uVTCordRenderBBox;   // Bounding box of vector texture coordinates
uniform int  uNTCordShiftScale;   // to shift and scale noise texture coordinates
                                  // when the output of pass #1 LIC is high-pass 
                                  // filtered and taken as the input 'noise' of 
                                  // pass #2 LIC
vec2         noiseTexCordShift = vec2( -uVTCordRenderBBox.x, -uVTCordRenderBBox.z );
vec2         noiseTexCordScale = 
                         vec2(  1.0 / ( uVTCordRenderBBox.y - uVTCordRenderBBox.x ),
                                1.0 / ( uVTCordRenderBBox.w - uVTCordRenderBBox.z )
                             );   // the texture coordinate scale factor
                             
vec2         miniVectorTCoords = vec2(  uVTCordRenderBBox.x,  uVTCordRenderBBox.z );
vec2         maxiVectorTCoords = vec2(  uVTCordRenderBBox.y,  uVTCordRenderBBox.w );
                             
// the range (of the vector field) that a single copy of the noise texture (note
// that the output of pass #1 LIC, after high-pass filtering, is just an extent / 
// sub-range of the virtual full noise texture) covers --- the reciprocal of 
// uNoise2VecScaling
vec2         NoiseTexOccupancy = vec2( 1.0, 1.0 ) / uNoise2VecScaling;

// to save division
float        vcScaleReciprocal = 1.0 / uVectorShiftScale.y;
float        rungeKutta_1Sixth = 1.0 / 6.0;  // for rk4

// Define prototype. 
// This function is compiled in to select the two components that form
// the surface vector (see vtkLineIntegralConvolution2D.cxx for the
// actual code).
vec2 getSelectedComponents(vec4 color);

// Given a vector field based coordinate tcords, this function returns
// the vector in "Normalized Image" space.
vec2 getVector( vec2 tcords )
{
  vec4 color  = texture2D( texVectorField, tcords );
  vec2 vector = getSelectedComponents( color );
  
  // since the forward tranformation is y = ( x + shift ) * scale,
  // now we perform backward transformation x = y / scale - shift
  // to obtain the original vector 
  // note: vcScaleReciprocal = 1.0 / uVectorShiftScale.y
  vector = ( vector * vcScaleReciprocal ) - uVectorShiftScale.x;
  return vector * uVectorTransform2;
}

// get the normalized vector at a given point
// note that direct use of the built-in function normalize( vec2 ) causes
// problems as it fails to handle zero-length vectors (division by zero)
vec2 getNormalizedVector( vec2 tcoord )
{ 
  vec2   vector = getVector( tcoord );
  float  vecLen = length( vector );
  vec2   retVec = ( vecLen == 0.0 ) ? vec2( 0.0, 0.0 ) : ( vector / vecLen );
    
  // in case of an invalid vector texture coordinate
  bvec2  beLess = lessThan   ( tcoord, miniVectorTCoords );
  bvec2  greatr = greaterThan( tcoord, maxiVectorTCoords );
  int    error0 = int(  any( beLess )  );
  int    error1 = int(  any( greatr )  );
  int    errors = ( error0 + error1  + 1 ) / 2;
  
  return retVec * float( 1 - errors );
}

// fourth-order Runge-Kutta streamline integration
vec2 rk2( vec2 point0, float fStep0 )
{
  vec2   vectr0 = getNormalizedVector(  point0                              );
  vec2   vectr1 = getNormalizedVector(  point0 + vectr0 * ( fStep0 * 0.5 )  );
  return point0 + vectr1 * fStep0; 
}

// fourth-order Runge-Kutta streamline integration
vec2 rk4( vec2 point0, float fStep0 )
{
  float  dtHalf = fStep0 * 0.5;
  vec2   vectr0 = getNormalizedVector( point0                   );
  vec2   vectr1 = getNormalizedVector( point0 + vectr0 * dtHalf );
  vec2   vectr2 = getNormalizedVector( point0 + vectr1 * dtHalf );
  vec2   vectr3 = getNormalizedVector( point0 + vectr2 * fStep0 );
  return (  point0 + ( vectr0 + vectr1 + vectr1 + vectr2 + vectr2 + vectr3 ) 
                   * ( fStep0 * rungeKutta_1Sixth )  ); 
}

// given a vector field-based texture coordinate vectrTCord, this function
// accesses the noise texture (with a different size from that of the vector
// field for pass #1 LIC, or the same size for pass #2 LIC) to locate the 
// target value
vec3 getNoiseColor( vec2 vectrTCord )
{
  // 'mod' tells the position (still vector field based) to which the current
  // fractional copy of the noise texture needs to be mapped (after possibly 
  // several full copies) and this position is then transformed to the noise
  // texture space --- noiseTCord
  vec2 noiseTCord = mod( vectrTCord, NoiseTexOccupancy ) * uNoise2VecScaling;
  
  // When the output of pass #1 LIC is high-pass filtered and then taken
  // to pass #2 LIC as the input 'noise', the size of this 'noise' texture
  // (uVTCordRenderBBox) is equal to the current extent of the vector 
  // field x this->Magnification (see vtkLineIntegralConvolution2D.cxx).
  // Since uNoise2VecScaling involves this->Magnification and hence the
  // value of uNoise2VecScaling for pass #2 LIC is just vec2(1.0, 1.0) AS
  // LONG AS we take this 'noise' texture as an extent (uVTCordRenderBBox) 
  // of the virtual full 'noise' texture (for which the out-of-extent part
  // is just not defined / provided --- 'virtual'). To compensate for the 
  // concept of this 'extent', the INITIAL (since uNoise2VecScaling is 1.0
  // by 1.0 above) vector field-based noise texture coordinate noiseTCord 
  // needs to be shifted and scaled below to index this 'noise' texture (an 
  // extent of the virtual full 'noise' texture) properly.
  vec2 tempTCoord = ( noiseTCord + noiseTexCordShift ) * noiseTexCordScale;
  noiseTCord = noiseTCord * float( 1 - uNTCordShiftScale ) + 
               tempTCoord * float(     uNTCordShiftScale );
               
  // Given the 200 x 200 white noise (VTKData\Data\Data\noise.png) currently
  // in use, half is actually used below (by multiplying the tcoord with 0.5)
  // for better image quality.
  noiseTCord = noiseTCord * (   float( uNTCordShiftScale + 1 ) * 0.5   );
  
  // now given a noise texture based coordinate, return the value
  return texture2D( texNoise, noiseTCord ).rgb;
}
