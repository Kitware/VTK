/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSweptSurface.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 5,542,036
    "Implicit Modeling of Swept Volumes and Swept Surfaces"
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Jerald Roehling
        GE Licensing
        One Independence Way
        PO Box 2023
        Princeton, N.J. 08540
        phone 609-734-9823
        e-mail:Roehlinj@gerlmo.ge.com
    for more information.

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
// .NAME vtkSweptSurface - given a path and input geometry generate an (implicit) representation of a swept surface
// .SECTION Description
// vtkSweptSurface is a filter that is used to create a surface defined by 
// moving a part along a path. In this implementation, the path is defined as
// a list of transformation matrices (vtkTransform), and the part geometry is
// implicitly defined using a volume (i.e., distance scalars in structured 
// point dataset). The input to the filter is the geometry (i.e., a 
// structured point dataset) and the output is a structured point dataset 
// (i.e., an implicit representation of the swept surface). If you wish to
// generate a polygonal representation of swept surface you will have to 
// use a contouring filter (e.g., vtkContourFilter). (You may also wish to
// use vtkDecimate to reduce mesh size.)
//
// The swept surface algorithm can be summarized as follows. A geometry 
// (i.e. the input) is swept along a path (list of transforms). At each point
// on the path the input is re-sampled into a volume using a union operation.
// (Union means that the minimum scalar value is retained - minimum distance
// value for example.) At the end, an implicit representation of the swept
// surface is defined.
// .SECTION See Also
// vtkImplicitModeller vtkContourFilter vtkDecimate

#ifndef __vtkSweptSurface_h
#define __vtkSweptSurface_h

#include "vtkStructuredPointsToStructuredPointsFilter.h"
#include "vtkTransformCollection.h"

class VTK_PATENTED_EXPORT vtkSweptSurface : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  static vtkSweptSurface *New();
  vtkTypeMacro(vtkSweptSurface,vtkStructuredPointsToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify i-j-k dimensions to sample input with. The higher the resolution
  // the lower the error but the greater the processing time.
  vtkSetVector3Macro(SampleDimensions,int);
  vtkGetVectorMacro(SampleDimensions,int,3);

  // Description:
  // Specify a path (i.e., list of transforms) that the input moves along. At
  // least two transforms must be used to define a path.
  vtkSetObjectMacro(Transforms, vtkTransformCollection);
  vtkGetObjectMacro(Transforms, vtkTransformCollection);

  // Description:
  // Voxels are initialized to this value. By default a large floating point
  // value is used, since the scalar values are assumed to be a distance 
  // function.
  vtkSetMacro(FillValue,float);
  vtkGetMacro(FillValue,float);

  // Description:
  // Value specifies/controls interpolation between the nodes (i.e., 
  // transforms) defining the path. A positive value indicates the number
  // of steps to take between transforms (i.e., interpolation is performed).
  // A negative value indicates that no interpolation to be performed, that is,
  // only the points defined at each transform are used (interpolation not
  // performed). A zero value indicates that automatic interpolation is to be
  // performed, that is, interpolation is computed so that potential errors 
  // fall below the error bounds defined in the text. By default, automatic
  // computation is performed (Interpolation = 0).
  vtkSetMacro(NumberOfInterpolationSteps,int);
  vtkGetMacro(NumberOfInterpolationSteps,int);

  // Description:
  // Set/get the maximum number of interpolation steps to take. This is useful
  // if you are limited in computation time or just know that the number of 
  // computed steps should not exceed a certain value.
  vtkSetMacro(MaximumNumberOfInterpolationSteps,int);
  vtkGetMacro(MaximumNumberOfInterpolationSteps,int);

  // Description:
  // The outer boundary of the sampling volume can be capped (i.e., assigned 
  // fill value). This will "close" the implicit model if the geometry 
  // approaches close to or passes through the boundary of the volume (i.e.,
  // defined by ModelBounds instance variable). Capping turns on/off this 
  // capability. By default capping is on.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  // Description:
  // Define the volume (in world coordinates) in which the sampling is to 
  // occur. Make sure that the volume is large enough to accommodate the 
  // motion of the geometry along the path. If the model bounds are set to
  // all zero values, the model bounds will be computed automatically from
  // the input geometry and path.
  vtkSetVectorMacro(ModelBounds,float,6);
  vtkGetVectorMacro(ModelBounds,float,6);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, 
                      float zmin, float zmax);

  // Description:
  // Control how the model bounds are computed. If the ivar AdjustBounds
  // is set, then the bounds specified (or computed automatically) is modified
  // by the fraction given by AdjustDistance. This means that the model
  // bounds is expanded in each of the x-y-z directions.
  vtkSetMacro(AdjustBounds,int);
  vtkGetMacro(AdjustBounds,int);
  vtkBooleanMacro(AdjustBounds,int);
  
  // Description:
  // Specify the amount to grow the model bounds (if the ivar AdjustBounds
  // is set). The value is a fraction of the maximum length of the sides
  // of the box specified by the model bounds.
  vtkSetClampMacro(AdjustDistance,float,-1.0,1.0);
  vtkGetMacro(AdjustDistance,float);

  //overload to check transformation matrices
  unsigned long int GetMTime();

protected:
  vtkSweptSurface();
  ~vtkSweptSurface();
  vtkSweptSurface(const vtkSweptSurface&);
  void operator=(const vtkSweptSurface&);

  void Execute();
  void ExecuteInformation();

  void ComputeBounds(float origin[3], float ar[3], float bbox[24]);
  int ComputeNumberOfSteps(vtkTransform *t1, vtkTransform *t2, float bbox[24]);
  void SampleInput(vtkMatrix4x4 *m, int inDim[3], float inOrigin[3],
                   float inAr[3], vtkDataArray *in, vtkDataArray *out);
  void ComputeFootprint (vtkMatrix4x4 *m, int inDim[3], float inOrigin[3],
			 float inSpacing[3], int Indicies[6]);
  void Cap(vtkDataArray *s);
  void GetRelativePosition(vtkTransform &t, float *origin, float *position);
  vtkMatrix4x4* GetActorMatrixPointer(vtkTransform &t,
                                      float origin[3],
				     float position[3], float orientation[3]);
  virtual void InterpolateStates(float *pos1, float *pos2, float *euler1, 
                                 float *euler2, float t, float *posOut,
                                 float *eulerOut);

  int SampleDimensions[3];
  float FillValue;
  float ModelBounds[6];
  int NumberOfInterpolationSteps;
  int MaximumNumberOfInterpolationSteps;
  int Capping;
  int AdjustBounds;
  float AdjustDistance;

  vtkTransformCollection *Transforms;

private:
  //used to perform computations
  vtkTransform *T;
};

#endif
