/*=========================================================================

  Program:   Visualization Library
  Module:    SweptSur.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSweptSurface - given a path and input geometry generate an (implicit) representation of a swept surface
// .SECTION Description
// vtkSweptSurface is a filter that is used to create a surface defined by 
// moving a part along a path. In this implementation the path is defined as
// a list of transformation matrices (vtkTransform), and the part geometry is
// implicitly defined using a volume (i.e., distance scalars in structured 
// point dataset). The input to the filter is the geometry (i.e., a 
// structured point dataset) and the output is a structured point dataset 
// (i.e., an implicit representation of the swept surface). If you wish to
// generate a polygonal representation of swept surface you will have to 
// use a contouring filter (e.g., vtkMarchingCubes). (You may also wish to
// use vtkDecimate to reduce mesh size).
//    The swept surface algorithm can be summarized as follows. A geometry 
// (i.e. the input) is swept along a path (list of transforms). At each point
// on the path the input is re-sampled into a volume using a union operation.
// (Union means that the minumum scalar value is retained - minimum distance
// value for example). At the end an implicit representation of the swept
// surface is defined.

#ifndef __vtkSweptSurface_h
#define __vtkSweptSurface_h

#include "SPt2SPtF.hh"
#include "TransC.hh"

class vtkSweptSurface : public vtkStructuredPointsToStructuredPointsFilter
{
public:
  vtkSweptSurface();
  ~vtkSweptSurface() {};
  char *GetClassName() {return "vtkSweptSurface";};
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
  // value is used since the scalar values are assumed to be a distance 
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
  vtkSetMacro(Interpolation,int);
  vtkGetMacro(Interpolation,int);

  // Description:
  // Description:
  // The outer boundary of the sampling volume can be capped (i.e., assigned 
  // fill value). This will "close" the implicit model if the geometry 
  // approaches close to or passes through the boundary of the volume (i.e.,
  // defined by ModelBounds instance variable). Capping turns on/off this 
  // capability. By default capping is on.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  
  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  //overload to check trnasformation matrices
  unsigned long int GetMTime();

protected:
  void Execute();
  void ComputeBounds();
  int ComputeNumberOfSteps(vtkTransform *t1, vtkTransform *t2);
  void SampleInput(vtkMatrix4x4& m, int inDim[3], float inOrigin[3],
                   float inAr[3], vtkScalars *in, vtkScalars *out);
  void Cap(vtkFloatScalars *s);

  int SampleDimensions[3];
  float FillValue;
  float ModelBounds[6];
  int Interpolation;
  int Capping;

  vtkTransformCollection *Transforms;
};

#endif


