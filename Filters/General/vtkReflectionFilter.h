/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkReflectionFilter
 * @brief   reflects a data set across a plane
 *
 * The vtkReflectionFilter reflects a data set across one of the
 * planes formed by the data set's bounding box.
 * Since it converts data sets into unstructured grids, it is not effeicient
 * for structured data sets.
*/

#ifndef vtkReflectionFilter_h
#define vtkReflectionFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"
class vtkUnstructuredGrid;
class vtkDataSet;

class VTKFILTERSGENERAL_EXPORT vtkReflectionFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkReflectionFilter *New();

  vtkTypeMacro(vtkReflectionFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  enum ReflectionPlane
  {
    USE_X_MIN = 0,
    USE_Y_MIN = 1,
    USE_Z_MIN = 2,
    USE_X_MAX = 3,
    USE_Y_MAX = 4,
    USE_Z_MAX = 5,
    USE_X = 6,
    USE_Y = 7,
    USE_Z = 8
  };

  //@{
  /**
   * Set the normal of the plane to use as mirror.
   */
  vtkSetClampMacro(Plane, int, 0, 8);
  vtkGetMacro(Plane, int);
  void SetPlaneToX() { this->SetPlane(USE_X); };
  void SetPlaneToY() { this->SetPlane(USE_Y); };
  void SetPlaneToZ() { this->SetPlane(USE_Z); };
  void SetPlaneToXMin() { this->SetPlane(USE_X_MIN); };
  void SetPlaneToYMin() { this->SetPlane(USE_Y_MIN); };
  void SetPlaneToZMin() { this->SetPlane(USE_Z_MIN); };
  void SetPlaneToXMax() { this->SetPlane(USE_X_MAX); };
  void SetPlaneToYMax() { this->SetPlane(USE_Y_MAX); };
  void SetPlaneToZMax() { this->SetPlane(USE_Z_MAX); };
  //@}

  //@{
  /**
   * If the reflection plane is set to X, Y or Z, this variable
   * is use to set the position of the plane.
   */
  vtkSetMacro(Center, double);
  vtkGetMacro(Center, double);
  //@}

  //@{
  /**
   * If on (the default), copy the input geometry to the output. If off,
   * the output will only contain the reflection.
   */
  vtkSetMacro(CopyInput, int);
  vtkGetMacro(CopyInput, int);
  vtkBooleanMacro(CopyInput, int);
  //@}

protected:
  vtkReflectionFilter();
  ~vtkReflectionFilter() VTK_OVERRIDE;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   * Overridden to create the correct type of output.
   */
  int RequestDataObject(vtkInformation*,
                        vtkInformationVector**,
                        vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Actual implementation for reflection.
   */
  virtual int RequestDataInternal(vtkDataSet* input, vtkUnstructuredGrid* output,
    double bounds[6]);

  /**
   * Internal method to compute bounds.
   */
  virtual int ComputeBounds(vtkDataObject* input, double bounds[6]);

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int Plane;
  double Center;
  int CopyInput;

  void FlipVector(double tuple[3], int mirrorDir[3]);

private:
  vtkReflectionFilter(const vtkReflectionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkReflectionFilter&) VTK_DELETE_FUNCTION;
};

#endif


