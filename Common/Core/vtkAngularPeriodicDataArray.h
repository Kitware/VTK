/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngularPeriodicDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkAngularPeriodicDataArray
 * @brief   Map native an Array into an angulat
 * periodic array
 *
 *
 * Map an array into a periodic array. Data from the original array aare
 * rotated (on the fly) by the specified angle along the specified axis
 * around the specified point. Lookup is not implemented.
 * Creating the array is virtually free, accessing a tuple require some
 * computation.
*/

#ifndef vtkAngularPeriodicDataArray_h
#define vtkAngularPeriodicDataArray_h

#include "vtkPeriodicDataArray.h"   // Parent

#define VTK_PERIODIC_ARRAY_AXIS_X 0
#define VTK_PERIODIC_ARRAY_AXIS_Y 1
#define VTK_PERIODIC_ARRAY_AXIS_Z 2

class vtkMatrix3x3;

template <class Scalar>
class vtkAngularPeriodicDataArray: public vtkPeriodicDataArray<Scalar>
{
public:
  vtkAbstractTemplateTypeMacro(vtkAngularPeriodicDataArray<Scalar>,
                               vtkPeriodicDataArray<Scalar>)
  vtkAOSArrayNewInstanceMacro(vtkAngularPeriodicDataArray<Scalar>)
  static vtkAngularPeriodicDataArray *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize the mapped array with the original input data array.
   */
  void InitializeArray(vtkAOSDataArrayTemplate<Scalar>* inputData);

  //@{
  /**
   * Set/Get the rotation angle in degrees. Default is 0.
   */
  void SetAngle(double angle);
  vtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Set/Get the rotation center. Default is 0,0,0.
   */
  void SetCenter(double* center);
  vtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the rotation axis. Default is VTK_PERIODIC_ARRAY_AXIS_X axis.
   */
  void SetAxis(int axis);
  vtkGetMacro(Axis, int);
  void SetAxisToX(void) { this->SetAxisType(VTK_PERIODIC_ARRAY_AXIS_X); }
  void SetAxisToY(void) { this->SetAxisType(VTK_PERIODIC_ARRAY_AXIS_Y); }
  void SetAxisToZ(void) { this->SetAxisType(VTK_PERIODIC_ARRAY_AXIS_Z); }
  //@}

protected:
  vtkAngularPeriodicDataArray();
  ~vtkAngularPeriodicDataArray();

  /**
   * Transform the provided tuple
   */
  void Transform(Scalar* tuple) const VTK_OVERRIDE;

  /**
   * Update rotation matrix from Axis, Angle and Center
   */
  void UpdateRotationMatrix();

private:
  vtkAngularPeriodicDataArray(const vtkAngularPeriodicDataArray &) VTK_DELETE_FUNCTION;
  void operator=(const vtkAngularPeriodicDataArray &) VTK_DELETE_FUNCTION;

  double Angle;            // Rotation angle in degrees
  double AngleInRadians;   // Rotation angle in radians
  double Center[3];        // Rotation center
  int Axis;                // Rotation Axis

  vtkMatrix3x3* RotationMatrix;
};

#include "vtkAngularPeriodicDataArray.txx"

#endif //vtkAngularPeriodicDataArray_h
// VTK-HeaderTest-Exclude: vtkAngularPeriodicDataArray.h
