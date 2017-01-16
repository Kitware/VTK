/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResliceCursorLineRepresentation
 * @brief   represent the vtkResliceCursorWidget
 *
 * This class provides a representation for the reslice cursor widget. It
 * consists of two cross sectional hairs, with an optional thickness. The
 * hairs may have a hole in the center. These may be translated or rotated
 * independent of each other in the view. The result is used to reslice
 * the data along these cross sections. This allows the user to perform
 * multi-planar thin or thick reformat of the data on an image view, rather
 * than a 3D view.
 * @sa
 * vtkResliceCursorWidget vtkResliceCursor vtkResliceCursorPolyDataAlgorithm
 * vtkResliceCursorRepresentation vtkResliceCursorThickLineRepresentation
 * vtkResliceCursorActor vtkImagePlaneWidget
*/

#ifndef vtkResliceCursorLineRepresentation_h
#define vtkResliceCursorLineRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkResliceCursorRepresentation.h"

class vtkPolyData;
class vtkResliceCursorActor;
class vtkResliceCursorPolyDataAlgorithm;
class vtkResliceCursorPicker;
class vtkResliceCursor;


class VTKINTERACTIONWIDGETS_EXPORT vtkResliceCursorLineRepresentation : public vtkResliceCursorRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkResliceCursorLineRepresentation *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkResliceCursorLineRepresentation,vtkResliceCursorRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * These are methods that satisfy vtkWidgetRepresentation's API.
   */
  void BuildRepresentation() VTK_OVERRIDE;
  int  ComputeInteractionState(int X, int Y, int modify=0) VTK_OVERRIDE;
  void StartWidgetInteraction(double startEventPos[2]) VTK_OVERRIDE;
  void WidgetInteraction(double e[2]) VTK_OVERRIDE;
  void Highlight(int highlightOn) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Methods required by vtkProp superclass.
   */
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;
  //@}

  /**
   * Get the bounds of this prop. This simply returns the bounds of the
   * reslice cursor object.
   */
  double * GetBounds() VTK_OVERRIDE;

  //@{
  /**
   * Get the reslice cursor actor. You must set the reslice cursor on this
   * class
   */
  vtkGetObjectMacro( ResliceCursorActor, vtkResliceCursorActor );
  //@}

  /**
   * Get the reslice cursor.
   */
  vtkResliceCursor * GetResliceCursor() VTK_OVERRIDE;

  /**
   * Set the user matrix on all the internal actors.
   */
  virtual void SetUserMatrix( vtkMatrix4x4 *matrix);

protected:
  vtkResliceCursorLineRepresentation();
  ~vtkResliceCursorLineRepresentation() VTK_OVERRIDE;

  vtkResliceCursorPolyDataAlgorithm * GetCursorAlgorithm() VTK_OVERRIDE;

  double RotateAxis( double evenPos[2], int axis );

  void RotateAxis( int axis, double angle );

  void RotateVectorAboutVector( double vectorToBeRotated[3],
                                double axis[3], // vector about which we rotate
                                double angle, // angle in radians
                                double o[3] );
  int DisplayToReslicePlaneIntersection(
    double displayPos[2], double intersectionPos[3] );

  vtkResliceCursorActor  * ResliceCursorActor;
  vtkResliceCursorPicker * Picker;

  double                   StartPickPosition[3];
  double                   StartCenterPosition[3];

  // Transformation matrices. These have no offset. Offset is recomputed
  // based on the cursor, so that the center of the cursor has the same
  // location in transformed space as it does in physical space.
  vtkMatrix4x4           * MatrixReslice;
  vtkMatrix4x4           * MatrixView;
  vtkMatrix4x4           * MatrixReslicedView;

private:
  vtkResliceCursorLineRepresentation(const vtkResliceCursorLineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResliceCursorLineRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
