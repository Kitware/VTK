/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceCursorPicker
 * @brief   ray-cast cell picker for the reslice cursor
 *
 * This class is used by the vtkResliceCursorWidget to pick reslice axes
 * drawn by a vtkResliceCursorActor. The class returns the axes picked if
 * any, whether one has picked the center. It takes as input an instance
 * of vtkResliceCursorPolyDataAlgorithm. This is all done internally by
 * vtkResliceCursorWidget and as such users are not expected to use this
 * class directly, unless they are overriding the behaviour of
 * vtkResliceCursorWidget.
 * @sa
 * vtkResliceCursor vtkResliceCursorWidget
*/

#ifndef vtkResliceCursorPicker_h
#define vtkResliceCursorPicker_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkPicker.h"

class vtkPolyData;
class vtkGenericCell;
class vtkResliceCursorPolyDataAlgorithm;
class vtkMatrix4x4;
class vtkPlane;

class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorPicker : public vtkPicker
{
public:
  static vtkResliceCursorPicker *New();
  vtkTypeMacro(vtkResliceCursorPicker, vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform pick operation with selection point provided. Normally the
   * first two values are the (x,y) pixel coordinates for the pick, and
   * the third value is z=0. The return value will be non-zero if
   * something was successfully picked.
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
                   vtkRenderer *renderer) override;

  //@{
  /**
   * Get the picked axis
   */
  vtkGetMacro( PickedAxis1, int );
  vtkGetMacro( PickedAxis2, int );
  vtkGetMacro( PickedCenter, int );
  //@}

  //@{
  /**
   * Set the reslice cursor algorithm. One must be set
   */
  virtual void SetResliceCursorAlgorithm(
      vtkResliceCursorPolyDataAlgorithm * );
  vtkGetObjectMacro( ResliceCursorAlgorithm,
                     vtkResliceCursorPolyDataAlgorithm );
  //@}

  virtual void SetTransformMatrix( vtkMatrix4x4 * );

  /**
   * Overloaded pick method that returns the picked coordinates of the current
   * resliced plane in world coordinates when given a display position
   */
  void Pick(
    double displayPos[2], double world[3], vtkRenderer *ren );

protected:
  vtkResliceCursorPicker();
  ~vtkResliceCursorPicker() override;

  virtual int IntersectPolyDataWithLine(
      double p1[3], double p2[3], vtkPolyData *, double tol );
  virtual int IntersectPointWithLine(
    double p1[3], double p2[3], double X[3], double tol );

  void TransformPlane();
  void TransformPoint( double pIn[4], double pOut[4] );
  void InverseTransformPoint( double pIn[4], double pOut[4] );

private:

  vtkGenericCell *Cell; //used to accelerate picking
  vtkResliceCursorPolyDataAlgorithm * ResliceCursorAlgorithm;

  int PickedAxis1;
  int PickedAxis2;
  int PickedCenter;
  vtkMatrix4x4 * TransformMatrix;
  vtkPlane     * Plane;

private:
  vtkResliceCursorPicker(const vtkResliceCursorPicker&) = delete;
  void operator=(const vtkResliceCursorPicker&) = delete;
};

#endif
