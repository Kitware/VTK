/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientEstimator.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// .NAME vtkEncodedGradientEstimator - Superclass for gradient estimation
// .SECTION Description
// vtkEncodedGradientEstimator is an abstract superclass for gradient 
// estimation. It takes a scalar input of vtkImageData, computes
// a gradient value for every point, and encodes this value into a 
// three byte value (2 for direction, 1 for magnitude) using the 
// vtkDirectionEncoder. The direction encoder is defaulted to a
// vtkRecursiveSphereDirectionEncoder, but can be overridden with the
// SetDirectionEncoder method. The scale and the bias values for the gradient
// magnitude are used to convert it into a one byte value according to
// v = m*scale + bias where m is the magnitude and v is the resulting
// one byte value.
// .SECTION see also
// vtkFiniteDifferenceGradientEstimator vtkDirectionEncoder

#ifndef __vtkEncodedGradientEstimator_h
#define __vtkEncodedGradientEstimator_h

#include "vtkObject.h"
#include "vtkMultiThreader.h"
#include "vtkImageData.h"
#include "vtkDirectionEncoder.h"

class VTK_RENDERING_EXPORT vtkEncodedGradientEstimator : public vtkObject
{
public:
  vtkTypeMacro(vtkEncodedGradientEstimator,vtkObject);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Set/Get the scalar input for which the normals will be 
  // calculated
  vtkSetObjectMacro( Input, vtkImageData );
  vtkGetObjectMacro( Input, vtkImageData );

  // Description:
  // Set/Get the scale and bias for the gradient magnitude
  vtkSetMacro( GradientMagnitudeScale, float );
  vtkGetMacro( GradientMagnitudeScale, float );
  vtkSetMacro( GradientMagnitudeBias, float );
  vtkGetMacro( GradientMagnitudeBias, float );

  // Description:
  // Turn on / off the bounding of the normal computation by
  // the this->Bounds bounding box
  vtkSetClampMacro( BoundsClip, int, 0, 1 );
  vtkGetMacro( BoundsClip, int );
  vtkBooleanMacro( BoundsClip, int );
  
  // Description:
  // Set / Get the bounds of the computation (used if 
  // this->ComputationBounds is 1.) The bounds are specified
  // xmin, xmax, ymin, ymax, zmin, zmax.
  vtkSetVector6Macro( Bounds, int );
  vtkGetVectorMacro(  Bounds, int, 6 );

  // Description:
  // Recompute the encoded normals and gradient magnitudes.
  void  Update( void );

  // Description:
  // Get the encoded normals.
  unsigned short  *GetEncodedNormals( void ); 

  // Description:
  // Get the encoded normal at an x,y,z location in the volume
  int   GetEncodedNormalIndex( int xyz_index );
  int   GetEncodedNormalIndex( int x_index, int y_index, int z_index );

  // Description:
  // Get the gradient magnitudes
  unsigned char *GetGradientMagnitudes(void);

  // Description:
  // Get/Set the number of threads to create when encoding normals
  // This defaults to the number of available processors on the machine
  vtkSetClampMacro( NumberOfThreads, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( NumberOfThreads, int );

  // Description:
  // Set / Get the direction encoder used to encode normal directions
  // to fit within two bytes
  void SetDirectionEncoder( vtkDirectionEncoder *direnc );
  vtkGetObjectMacro( DirectionEncoder, vtkDirectionEncoder );

  // Description:
  // If you don't want to compute gradient magnitudes (but you
  // do want normals for shading) this can be used. Be careful - if
  // if you a non-constant gradient magnitude transfer function and
  // you turn this on, it may crash
  vtkSetMacro( ComputeGradientMagnitudes, int );
  vtkGetMacro( ComputeGradientMagnitudes, int );
  vtkBooleanMacro( ComputeGradientMagnitudes, int );

  // Description:
  // If the data in each slice is only contained within a circle circumscribed
  // within the slice, and the slice is square, then don't compute anything
  // outside the circle. This circle through the slices forms a cylinder.
  vtkSetMacro( CylinderClip, int );
  vtkGetMacro( CylinderClip, int );
  vtkBooleanMacro( CylinderClip, int );

  // Description:
  // Get the time required for the last update in seconds or cpu seconds
  vtkGetMacro( LastUpdateTimeInSeconds, float );
  vtkGetMacro( LastUpdateTimeInCPUSeconds, float );

  vtkGetMacro( UseCylinderClip, int );
  int *GetCircleLimits() { return this->CircleLimits; };

  // Description:
  // Set / Get the ZeroNormalThreshold - this defines the minimum magnitude 
  // of a gradient that is considered sufficient to define a 
  // direction. Gradients with magnitudes at or less than this value are given
  // a "zero normal" index. These are handled specially in the shader, 
  // and you can set the intensity of light for these zero normals in
  // the gradient shader.
  void SetZeroNormalThreshold( float v );
  vtkGetMacro( ZeroNormalThreshold, float );

  // Description:
  // Assume that the data value outside the volume is zero when
  // computing normals.
  vtkSetClampMacro( ZeroPad, int, 0, 1 );
  vtkGetMacro( ZeroPad, int );
  vtkBooleanMacro( ZeroPad, int );
  
  
  // These variables should be protected but are being
  // made public to be accessible to the templated function.
  // We used to have the templated function as a friend, but
  // this does not work with all compilers

  // The input scalar data on which the normals are computed
  vtkImageData         *Input;

  // The encoded normals (2 bytes) and the size of the encoded normals
  unsigned short        *EncodedNormals;
  int                   EncodedNormalsSize[3];

  // The magnitude of the gradient array and the size of this array
  unsigned char         *GradientMagnitudes;

  // The time at which the normals were last built
  vtkTimeStamp          BuildTime;

//BTX
  vtkGetVectorMacro( InputSize, int, 3 );
  vtkGetVectorMacro( InputAspect, float, 3 );
//ETX
  
protected:
  vtkEncodedGradientEstimator();
  ~vtkEncodedGradientEstimator();
  vtkEncodedGradientEstimator(const vtkEncodedGradientEstimator&);
  void operator=(const vtkEncodedGradientEstimator&);

  // The number of threads to use when encoding normals
  int                        NumberOfThreads;

  vtkMultiThreader           *Threader;

  vtkDirectionEncoder        *DirectionEncoder;

  virtual void               UpdateNormals( void ) = 0;

  float                      GradientMagnitudeScale;
  float                      GradientMagnitudeBias;

  float                      LastUpdateTimeInSeconds;
  float                      LastUpdateTimeInCPUSeconds;

  float                      ZeroNormalThreshold;

  int                        CylinderClip;
  int                        *CircleLimits;
  int                        CircleLimitsSize;
  int                        UseCylinderClip;
  void                       ComputeCircleLimits( int size );
  
  int                        BoundsClip;
  int                        Bounds[6];

  int                        InputSize[3];
  float                      InputAspect[3];

  int                        ComputeGradientMagnitudes;
  
  int                        ZeroPad;
  
}; 


#endif

