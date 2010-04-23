/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridTransform - a nonlinear warp transformation
// .SECTION Description
// vtkGridTransform describes a nonlinear warp transformation as a set
// of displacement vectors sampled along a uniform 3D grid.
// .SECTION Caveats
// The inverse grid transform is calculated using an iterative method,
// and is several times more expensive than the forward transform.
// .SECTION see also
// vtkThinPlateSplineTransform vtkGeneralTransform vtkTransformToGrid


#ifndef __vtkGridTransform_h
#define __vtkGridTransform_h

#include "vtkWarpTransform.h"

class vtkImageData;

#define VTK_GRID_NEAREST 0
#define VTK_GRID_LINEAR 1
#define VTK_GRID_CUBIC 3

class VTK_HYBRID_EXPORT vtkGridTransform : public vtkWarpTransform
{
public:
  static vtkGridTransform *New();
  vtkTypeMacro(vtkGridTransform,vtkWarpTransform);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the grid transform (the grid transform must have three 
  // components for displacement in x, y, and z respectively).
  // The vtkGridTransform class will never modify the data.
  virtual void SetDisplacementGrid(vtkImageData*);
  vtkGetObjectMacro(DisplacementGrid,vtkImageData);

  // Description:
  // Set scale factor to be applied to the displacements.
  // This is used primarily for grids which contain integer
  // data types.  Default: 1
  vtkSetMacro(DisplacementScale,double);
  vtkGetMacro(DisplacementScale,double);

  // Description:
  // Set a shift to be applied to the displacements.  The shift
  // is applied after the scale, i.e. x = scale*y + shift.
  // Default: 0
  vtkSetMacro(DisplacementShift,double);
  vtkGetMacro(DisplacementShift,double);

  // Description:
  // Set interpolation mode for sampling the grid.  Higher-order
  // interpolation allows you to use a sparser grid.
  // Default: Linear.
  void SetInterpolationMode(int mode);
  vtkGetMacro(InterpolationMode,int);
  void SetInterpolationModeToNearestNeighbor()
    { this->SetInterpolationMode(VTK_GRID_NEAREST); };
  void SetInterpolationModeToLinear()
    { this->SetInterpolationMode(VTK_GRID_LINEAR); };
  void SetInterpolationModeToCubic()
    { this->SetInterpolationMode(VTK_GRID_CUBIC); };
  const char *GetInterpolationModeAsString();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

  // Description:
  // Get the MTime.
  unsigned long GetMTime();

protected:
  vtkGridTransform();
  ~vtkGridTransform();

  // Description:
  // Update the displacement grid.
  void InternalUpdate();

  // Description:
  // Copy this transform from another of the same type.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  // Description:
  // Internal functions for calculating the transformation.
  void ForwardTransformPoint(const float in[3], float out[3]);
  void ForwardTransformPoint(const double in[3], double out[3]);

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

  void InverseTransformPoint(const float in[3], float out[3]);
  void InverseTransformPoint(const double in[3], double out[3]);

  void InverseTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]);
  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]);

//BTX
  void (*InterpolationFunction)(double point[3], double displacement[3],
                                double derivatives[3][3],
                                void *gridPtr, int gridType,
                                int inExt[6], vtkIdType inInc[3]);
//ETX
  int InterpolationMode;
  vtkImageData *DisplacementGrid;
  double DisplacementScale;
  double DisplacementShift;
  
  void *GridPointer;
  int GridScalarType;
  double GridSpacing[3];
  double GridOrigin[3];
  int GridExtent[6];
  vtkIdType GridIncrements[3];

private:
  vtkGridTransform(const vtkGridTransform&);  // Not implemented.
  void operator=(const vtkGridTransform&);  // Not implemented.
};

//BTX

//----------------------------------------------------------------------------
inline const char *vtkGridTransform::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_GRID_NEAREST:
      return "NearestNeighbor";
    case VTK_GRID_LINEAR:
      return "Linear";
    case VTK_GRID_CUBIC:
      return "Cubic";
    default:
      return "";
    }
}  
//ETX

#endif





