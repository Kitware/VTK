/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianSplatter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  vtkGaussianSplatter();
  static vtkGaussianSplatter *New() {return new vtkGaussianSplatter;};
  const char *GetClassName() {return "vtkGaussianSplatter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ComputeModelBounds();

  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify the radius of propagation of the splat. This value is expressed
  // as a percentage  of the sampling structured point set. Smaller numbers 
  // greatly reduce execution time.
  vtkSetClampMacro(Radius,float,0.0,1.0);
  vtkGetMacro(Radius,float);

  // Description:
  // Multiply Gaussian splat distribution by this value.
  vtkSetClampMacro(ScaleFactor,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Specify sharpness of decay of splat
  vtkSetMacro(ExponentFactor,float);
  vtkGetMacro(ExponentFactor,float);

  // Description:
  // Control the shape of elliptical splatting. Eccentricity is the ratio
  // of the major axis (aligned along normal) to the minor (axes) aligned
  // along other two axes.
  vtkSetClampMacro(Eccentricity,float,0.001,VTK_LARGE_FLOAT);
  vtkGetMacro(Eccentricity,float);

  // Description:
  // Set the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which the 
  // sampling is performed.
  vtkSetVectorMacro(ModelBounds,float,6);
  vtkGetVectorMacro(ModelBounds,float,6);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, 
                      float zmin, float zmax);

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
  void Execute();
  void Cap(vtkFloatScalars *s);

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


