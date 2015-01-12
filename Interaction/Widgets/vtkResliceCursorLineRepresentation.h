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
// .NAME vtkResliceCursorLineRepresentation - represent the vtkResliceCursorWidget
// .SECTION Description
// This class provides a representation for the reslice cursor widget. It
// consists of two cross sectional hairs, with an optional thickness. The
// hairs may have a hole in the center. These may be translated or rotated
// independent of each other in the view. The result is used to reslice
// the data along these cross sections. This allows the user to perform
// multi-planar thin or thick reformat of the data on an image view, rather
// than a 3D view.
// .SECTION See Also
// vtkResliceCursorWidget vtkResliceCursor vtkResliceCursorPolyDataAlgorithm
// vtkResliceCursorRepresentation vtkResliceCursorThickLineRepresentation
// vtkResliceCursorActor vtkImagePlaneWidget


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
  // Description:
  // Instantiate the class.
  static vtkResliceCursorLineRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkResliceCursorLineRepresentation,vtkResliceCursorRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation();
  virtual int  ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double startEventPos[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual void Highlight(int highlightOn);

  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Get the bounds of this prop. This simply returns the bounds of the
  // reslice cursor object.
  virtual double * GetBounds();

  // Description:
  // Get the reslice cursor actor. You must set the reslice cursor on this
  // class
  vtkGetObjectMacro( ResliceCursorActor, vtkResliceCursorActor );

  // Description:
  // Get the reslice cursor.
  virtual vtkResliceCursor * GetResliceCursor();

  // Description:
  // Set the user matrix on all the internal actors.
  virtual void SetUserMatrix( vtkMatrix4x4 *matrix);

protected:
  vtkResliceCursorLineRepresentation();
  ~vtkResliceCursorLineRepresentation();

  virtual vtkResliceCursorPolyDataAlgorithm * GetCursorAlgorithm();

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
  vtkResliceCursorLineRepresentation(const vtkResliceCursorLineRepresentation&);  //Not implemented
  void operator=(const vtkResliceCursorLineRepresentation&);  //Not implemented
};

#endif
