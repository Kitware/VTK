/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianSplatter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkGaussianSplatter - splat points with Gaussian distribution
// .SECTION Description
// vtkGaussianSplatter is a filter that injects input points into a structured 
// points dataset. As each point is injected, it "splats" or distributes 
// values to neighboring voxels in the structured points dataset. Data is
// distributed using a Gaussian distribution function. The distribution
// function is modified using scalar values (expands distribution) or 
// normals/vectors (creates ellipsoidal distribution rather than spherical).

#ifndef __vtkGaussianSplatter_h
#define __vtkGaussianSplatter_h

#include "vtkDataSetToStructuredPointsFilter.h"

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
  // Compute the size of the sample bounding box automatically from the
  // input data.
  void ComputeModelBounds();

  // Description:
  // Set / get the dimensions of the sampling structured point set.
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Set / get the radius of propagation of the splat. This value is expressed
  // as a percentage  of the sampling structured point set. Smaller numbers 
  // greatly reduce execution time.
  vtkSetClampMacro(Radius,float,0.0,1.0);
  vtkGetMacro(Radius,float);

  // Description:
  // Multiply Gaussian splat distribution by this value.
  vtkSetClampMacro(ScaleFactor,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Set / get the sharpness of decay of the splats
  vtkSetMacro(ExponentFactor,float);
  vtkGetMacro(ExponentFactor,float);

  // Description:
  // Control the shape of elliptical splatting. Eccentricity is the ratio
  // of the major axis (aligned along normal) to the minor (axes) aligned
  // along other two axes.
  vtkSetClampMacro(Eccentricity,float,0.001,VTK_LARGE_FLOAT);
  vtkGetMacro(Eccentricity,float);

  // Description:
  // Set / get the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which
  // the sampling is performed.
  vtkSetVector6Macro(ModelBounds,float);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Turn on/off the generation of elliptical splats.
  vtkSetMacro(NormalWarping,int);
  vtkGetMacro(NormalWarping,int);
  vtkBooleanMacro(NormalWarping,int);

  // Description:
  // Turn on/off the scaling of splats by scalar value.
  vtkSetMacro(ScalarWarping,int);
  vtkGetMacro(ScalarWarping,int);
  vtkBooleanMacro(ScalarWarping,int);

  // Description:
  // Turn on/off the capping of the outside parts of the structured point
  // set by setting to a specified cap value.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Specify the cap value to use.
  vtkSetMacro(CapValue,float);
  vtkGetMacro(CapValue,float);

protected:
  vtkGaussianSplatter();
  ~vtkGaussianSplatter() {};
  vtkGaussianSplatter(const vtkGaussianSplatter&) {};
  void operator=(const vtkGaussianSplatter&) {};

  void Execute();
  void Cap(vtkScalars *s);

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

  // recursive propagation of splat
  void SplitIJK(int i, int idir, int j, int jdir, int k, int kdir);
  void SplitIJ(int i, int idir, int j, int jdir, int k);
  void SplitIK(int i, int idir, int j, int k, int kdir);
  void SplitJK(int i, int j, int jdir, int k, int kdir);
  void SplitI(int i, int idir, int j, int k);
  void SplitJ(int i, int j, int jdir, int k);
  void SplitK(int i, int j, int k, int kdir);

  float Gaussian(float x[3]);  
  float EccentricGaussian(float x[3]);  
  float ScalarSampling(float s) {return this->ScaleFactor * s;};
  float PositionSampling(float) {return this->ScaleFactor;};
  void SetScalar(int idx, float dist2);

};

#endif


