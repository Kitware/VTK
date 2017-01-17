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
/**
 * @class   vtkGridTransform
 * @brief   a nonlinear warp transformation
 *
 * vtkGridTransform describes a nonlinear warp transformation as a set
 * of displacement vectors sampled along a uniform 3D grid.
 * @warning
 * The inverse grid transform is calculated using an iterative method,
 * and is several times more expensive than the forward transform.
 * @sa
 * vtkThinPlateSplineTransform vtkGeneralTransform vtkTransformToGrid
*/

#ifndef vtkGridTransform_h
#define vtkGridTransform_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkWarpTransform.h"

class vtkAlgorithmOutput;
class vtkGridTransformConnectionHolder;
class vtkImageData;

#define VTK_GRID_NEAREST VTK_NEAREST_INTERPOLATION
#define VTK_GRID_LINEAR VTK_LINEAR_INTERPOLATION
#define VTK_GRID_CUBIC VTK_CUBIC_INTERPOLATION

class VTKFILTERSHYBRID_EXPORT vtkGridTransform : public vtkWarpTransform
{
public:
  static vtkGridTransform *New();
  vtkTypeMacro(vtkGridTransform,vtkWarpTransform);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the grid transform (the grid transform must have three
   * components for displacement in x, y, and z respectively).
   * The vtkGridTransform class will never modify the data.
   * Note that SetDisplacementGridData() does not setup a pipeline
   * connection whereas SetDisplacementGridConnection does.
   */
  virtual void SetDisplacementGridConnection(vtkAlgorithmOutput*);
  virtual void SetDisplacementGridData(vtkImageData*);
  virtual vtkImageData* GetDisplacementGrid();
  //@}

  //@{
  /**
   * Set scale factor to be applied to the displacements.
   * This is used primarily for grids which contain integer
   * data types.  Default: 1
   */
  vtkSetMacro(DisplacementScale,double);
  vtkGetMacro(DisplacementScale,double);
  //@}

  //@{
  /**
   * Set a shift to be applied to the displacements.  The shift
   * is applied after the scale, i.e. x = scale*y + shift.
   * Default: 0
   */
  vtkSetMacro(DisplacementShift,double);
  vtkGetMacro(DisplacementShift,double);
  //@}

  //@{
  /**
   * Set interpolation mode for sampling the grid.  Higher-order
   * interpolation allows you to use a sparser grid.
   * Default: Linear.
   */
  void SetInterpolationMode(int mode);
  vtkGetMacro(InterpolationMode,int);
  void SetInterpolationModeToNearestNeighbor()
    { this->SetInterpolationMode(VTK_NEAREST_INTERPOLATION); };
  void SetInterpolationModeToLinear()
    { this->SetInterpolationMode(VTK_LINEAR_INTERPOLATION); };
  void SetInterpolationModeToCubic()
    { this->SetInterpolationMode(VTK_CUBIC_INTERPOLATION); };
  const char *GetInterpolationModeAsString();
  //@}

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform *MakeTransform() VTK_OVERRIDE;

  /**
   * Get the MTime.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkGridTransform();
  ~vtkGridTransform() VTK_OVERRIDE;

  /**
   * Update the displacement grid.
   */
  void InternalUpdate() VTK_OVERRIDE;

  /**
   * Copy this transform from another of the same type.
   */
  void InternalDeepCopy(vtkAbstractTransform *transform) VTK_OVERRIDE;

  //@{
  /**
   * Internal functions for calculating the transformation.
   */
  void ForwardTransformPoint(const float in[3], float out[3]) VTK_OVERRIDE;
  void ForwardTransformPoint(const double in[3], double out[3]) VTK_OVERRIDE;
  //@}

  void ForwardTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]) VTK_OVERRIDE;
  void ForwardTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) VTK_OVERRIDE;

  void InverseTransformPoint(const float in[3], float out[3]) VTK_OVERRIDE;
  void InverseTransformPoint(const double in[3], double out[3]) VTK_OVERRIDE;

  void InverseTransformDerivative(const float in[3], float out[3],
                                  float derivative[3][3]) VTK_OVERRIDE;
  void InverseTransformDerivative(const double in[3], double out[3],
                                  double derivative[3][3]) VTK_OVERRIDE;

  void (*InterpolationFunction)(double point[3], double displacement[3],
                                double derivatives[3][3],
                                void *gridPtr, int gridType,
                                int inExt[6], vtkIdType inInc[3]);

  int InterpolationMode;
  double DisplacementScale;
  double DisplacementShift;

  void *GridPointer;
  int GridScalarType;
  double GridSpacing[3];
  double GridOrigin[3];
  int GridExtent[6];
  vtkIdType GridIncrements[3];

private:
  vtkGridTransform(const vtkGridTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGridTransform&) VTK_DELETE_FUNCTION;

  vtkGridTransformConnectionHolder* ConnectionHolder;
};

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

#endif





