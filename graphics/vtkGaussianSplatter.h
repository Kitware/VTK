/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianSplatter.h
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
// .NAME vtkGaussianSplatter - splat points into a volume with an elliptical, Gaussian distribution
// .SECTION Description
// vtkGaussianSplatter is a filter that injects input points into a
// structured points (volume) dataset. As each point is injected, it "splats"
// or distributes values to nearby voxels. Data is distributed using an
// elliptical, Gaussian distribution function. The distribution function is
// modified using scalar values (expands distribution) or normals
// (creates ellipsoidal distribution rather than spherical).
//
// In general, the Gaussian distribution function f(x) around a given
// splat point p is given by
//
//     f(x) = ScaleFactor * exp( ExponentFactor*((r/Radius)**2) )
//
// where x is the current voxel sample point; r is the distance |x-p|
// ExponentFactor <= 0.0, and ScaleFactor can be multiplied by the scalar 
// value of the point p that is currently being splatted.
//
// If points normals are present (and NormalWarping is on), then the splat 
// function becomes elliptical (as compared to the spherical one described
// by the previous equation). The Gaussian distribution function then
// becomes:
// 
//     f(x) = ScaleFactor * 
//               exp( ExponentFactor*( ((rxy/E)**2 + z**2)/R**2) )
//
// where E is a user-defined eccentricity factor that controls the elliptical
// shape of the splat; z is the distance of the current voxel sample point
// along normal N; and rxy is the distance of x in the direction
// prependicular to N.
//
// This class is typically used to convert point-valued distributions into
// a volume representation. The volume is then usually iso-surfaced or
// volume rendered to generate a visualization. It can be used to create
// surfaces from point distributions, or to create structure (i.e.,
// topology) when none exists.

// .SECTION Caveats
// The input to this filter is any dataset type. This filter can be used 
// to resample any form of data, i.e., the input data need not be 
// unstructured. 
//
// Some voxels may never receive a contribution during the splatting process.
// The final value of these points can be specified with the "NullValue" 
// instance variable.

// .SECTION See Also
// vtkShepardMethod

#ifndef __vtkGaussianSplatter_h
#define __vtkGaussianSplatter_h

#include "vtkDataSetToStructuredPointsFilter.h"

#define VTK_ACCUMULATION_MODE_MIN 0
#define VTK_ACCUMULATION_MODE_MAX 1
#define VTK_ACCUMULATION_MODE_SUM 2

class vtkFloatAray;

class VTK_EXPORT vtkGaussianSplatter : public vtkDataSetToStructuredPointsFilter 
{
public:
  vtkTypeMacro(vtkGaussianSplatter,vtkDataSetToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with dimensions=(50,50,50); automatic computation of 
  // bounds; a splat radius of 0.1; an exponent factor of -5; and normal and 
  // scalar warping turned on.
  static vtkGaussianSplatter *New();

  // Description:
  // Set / get the dimensions of the sampling structured point set. Higher
  // values produce better results but are much slower.
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Set / get the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which
  // the sampling is performed. If any of the (min,max) bounds values are
  // min >= max, then the bounds will be computed automatically from the input
  // data. Otherwise, the user-specified bounds will be used.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Set / get the radius of propagation of the splat. This value is expressed
  // as a percentage of the length of the longest side of the sampling
  // volume. Smaller numbers greatly reduce execution time.
  vtkSetClampMacro(Radius,float,0.0,1.0);
  vtkGetMacro(Radius,float);

  // Description:
  // Multiply Gaussian splat distribution by this value. If ScalarWarping
  // is on, then the Scalar value will be multiplied by the ScaleFactor
  // times the Gaussian function.
  vtkSetClampMacro(ScaleFactor,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set / get the sharpness of decay of the splats. This is the
  // exponent constant in the Gaussian equation. Normally this is
  // a negative value.
  vtkSetMacro(ExponentFactor,float);
  vtkGetMacro(ExponentFactor,float);

  // Description:
  // Turn on/off the generation of elliptical splats. If normal warping is
  // on, then the input normals affect the distribution of the splat. This
  // boolean is used in combination with the Eccentricity ivar.
  vtkSetMacro(NormalWarping,int);
  vtkGetMacro(NormalWarping,int);
  vtkBooleanMacro(NormalWarping,int);

  // Description:
  // Control the shape of elliptical splatting. Eccentricity is the ratio
  // of the major axis (aligned along normal) to the minor (axes) aligned
  // along other two axes. So Eccentricity > 1 creates needles with the
  // long axis in the direction of the normal; Eccentricity<1 creates
  // pancakes perpendicular to the normal vector.
  vtkSetClampMacro(Eccentricity,float,0.001,VTK_LARGE_FLOAT);
  vtkGetMacro(Eccentricity,float);

  // Description:
  // Turn on/off the scaling of splats by scalar value.
  vtkSetMacro(ScalarWarping,int);
  vtkGetMacro(ScalarWarping,int);
  vtkBooleanMacro(ScalarWarping,int);

  // Description:
  // Turn on/off the capping of the outer boundary of the volume
  // to a specified cap value. This can be used to close surfaces
  // (after iso-surfacing) and create other effects.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Specify the cap value to use. (This instance variable only has effect 
  // if the ivar Capping is on.)
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

  // Description:
  // Specify the scalar accumulation mode. This mode expresses how scalar
  // values are combined when splats are overlapped. The Max mode acts
  // like a set union operation and is the most commonly used; the Min
  // mode acts like a set intersection, and the sum is just weird.
  vtkSetClampMacro(AccumulationMode,int,
                   VTK_ACCUMULATION_MODE_MIN,VTK_ACCUMULATION_MODE_SUM);
  vtkGetMacro(AccumulationMode,int);
  void SetAccumulationModeToMin()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MIN);}
  void SetAccumulationModeToMax()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MAX);}
  void SetAccumulationModeToSum()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_SUM);}
  const char *GetAccumulationModeAsString();

  // Description:
  // Set the Null value for output points not receiving a contribution from the
  // input points. (This is the initial value of the voxel samples.)
  vtkSetMacro(NullValue,float);
  vtkGetMacro(NullValue,float);

  // Description:
  // Compute the size of the sample bounding box automatically from the
  // input data. This is an internal helper function.
  void ComputeModelBounds();

protected:
  vtkGaussianSplatter();
  ~vtkGaussianSplatter() {};
  vtkGaussianSplatter(const vtkGaussianSplatter&) {};
  void operator=(const vtkGaussianSplatter&) {};

  void Execute();
  void Cap(vtkFloatArray *s);

  int SampleDimensions[3]; // dimensions of volume to splat into
  float Radius; // maximum distance splat propagates (as fraction 0->1)
  float ExponentFactor; // scale exponent of gaussian function
  float ModelBounds[6]; // bounding box of splatting dimensions
  int NormalWarping; // on/off warping of splat via normal
  float Eccentricity;// elliptic distortion due to normals
  int ScalarWarping; // on/off warping of splat via scalar
  float ScaleFactor; // splat size influenced by scale factor
  int Capping; // Cap side of volume to close surfaces
  float CapValue; // value to use for capping
  int AccumulationMode; // how to combine scalar values

  float Gaussian(float x[3]);  
  float EccentricGaussian(float x[3]);  
  float ScalarSampling(float s) 
    {return this->ScaleFactor * s;}
  float PositionSampling(float) 
    {return this->ScaleFactor;}
  void SetScalar(int idx, float dist2);

//BTX
private:
  vtkFloatArray *NewScalars;
  float Radius2;
  float (vtkGaussianSplatter::*Sample)(float x[3]);
  float (vtkGaussianSplatter::*SampleFactor)(float s);
  char *Visited;
  float Eccentricity2;
  float *P;
  float *N;
  float S;
  float Origin[3];
  float Spacing[3];
  float SplatDistance[3];
  float NullValue;
//ETX

};

#endif


