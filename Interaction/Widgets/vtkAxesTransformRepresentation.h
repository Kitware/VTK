/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxesTransformRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAxesTransformRepresentation - represent the vtkAxesTransformWidget
// .SECTION Description
// The vtkAxesTransformRepresentation is a representation for the
// vtkAxesTransformWidget. This representation consists of a origin sphere
// with three tubed axes with cones at the end of the axes. In addition an
// optional lable provides delta values of motion. Note that this particular
// widget draws its representation in 3D space, so the widget can be
// occluded.
// .SECTION See Also
// vtkDistanceWidget vtkDistanceRepresentation vtkDistanceRepresentation2D


#ifndef vtkAxesTransformRepresentation_h
#define vtkAxesTransformRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkHandleRepresentation;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkVectorText;
class vtkFollower;
class vtkBox;
class vtkCylinderSource;
class vtkGlyph3D;
class vtkDoubleArray;
class vtkTransformPolyDataFilter;
class vtkProperty;


class VTKINTERACTIONWIDGETS_EXPORT vtkAxesTransformRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkAxesTransformRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkAxesTransformRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the two handle representations used for the
  // vtkAxesTransformWidget. (Note: properties can be set by grabbing these
  // representations and setting the properties appropriately.)
  vtkGetObjectMacro(OriginRepresentation,vtkHandleRepresentation);
  vtkGetObjectMacro(SelectionRepresentation,vtkHandleRepresentation);

  // Description:
  // Methods to Set/Get the coordinates of the two points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  double* GetOriginWorldPosition();
  void GetOriginWorldPosition(double pos[3]);
  void SetOriginWorldPosition(double pos[3]);
  void SetOriginDisplayPosition(double pos[3]);
  void GetOriginDisplayPosition(double pos[3]);

  // Description:
  // Specify a scale to control the size of the widget. Large values make the
  // the widget larger.

  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the end points of
  // the widget to be active.
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);

  // Description:
  // Specify the format to use for labelling information during
  // transformation. Note that an empty string results in no label, or a
  // format string without a "%" character will not print numeric values.
  vtkSetStringMacro(LabelFormat);
  vtkGetStringMacro(LabelFormat);

  // Description:
  // Enum used to communicate interaction state.
  enum {Outside=0,OnOrigin,OnX,OnY,OnZ,OnXEnd,OnYEnd,OnZEnd};

  // Description:
  // The interaction state may be set from a widget (e.g., vtkLineWidget2) or
  // other object. This controls how the interaction with the widget
  // proceeds. Normally this method is used as part of a handshaking
  // process with the widget: First ComputeInteractionState() is invoked that
  // returns a state based on geometric considerations (i.e., cursor near a
  // widget feature), then based on events, the widget may modify this
  // further.
  vtkSetClampMacro(InteractionState,int,Outside,OnZEnd);

  // Description:
  // Method to satisfy superclasses' API.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual double *GetBounds();

  // Description:
  // Methods required by vtkProp superclass.
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);

  // Description:
  // Scale text (font size along each dimension). This helps control
  // the appearance of the 3D text.
  void SetLabelScale(double x, double y, double z)
  {
    double scale[3];
    scale[0] = x;
    scale[1] = y;
    scale[2] = z;
    this->SetLabelScale(scale);
  }
  virtual void SetLabelScale( double scale[3] );
  virtual double * GetLabelScale();

  // Description:
  // Get the distance annotation property
  virtual vtkProperty *GetLabelProperty();

protected:
  vtkAxesTransformRepresentation();
  ~vtkAxesTransformRepresentation();

  // The handle and the rep used to close the handles
  vtkHandleRepresentation *OriginRepresentation;
  vtkHandleRepresentation *SelectionRepresentation;

  // Selection tolerance for the handles
  int Tolerance;

  // Format for printing the distance
  char *LabelFormat;

  // The line
  vtkPoints         *LinePoints;
  vtkPolyData       *LinePolyData;
  vtkPolyDataMapper *LineMapper;
  vtkActor          *LineActor;

  // The distance label
  vtkVectorText     *LabelText;
  vtkPolyDataMapper *LabelMapper;
  vtkFollower       *LabelActor;

  // The 3D disk tick marks
  vtkPoints         *GlyphPoints;
  vtkDoubleArray    *GlyphVectors;
  vtkPolyData       *GlyphPolyData;
  vtkCylinderSource *GlyphCylinder;
  vtkTransformPolyDataFilter *GlyphXForm;
  vtkGlyph3D        *Glyph3D;
  vtkPolyDataMapper *GlyphMapper;
  vtkActor          *GlyphActor;

  // Support GetBounds() method
  vtkBox *BoundingBox;

  double LastEventPosition[3];

private:
  vtkAxesTransformRepresentation(const vtkAxesTransformRepresentation&);  //Not implemented
  void operator=(const vtkAxesTransformRepresentation&);  //Not implemented
};

#endif
