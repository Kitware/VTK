/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDistanceRepresentation3D - represent the vtkDistanceWidget
// .SECTION Description
// The vtkDistanceRepresentation3D is a representation for the
// vtkDistanceWidget. This representation consists of a measuring line (axis)
// and two vtkHandleWidgets to place the end points of the line. Note that
// this particular widget draws its representation in 3D space, so the widget
// can be occluded.

// .SECTION See Also
// vtkDistanceWidget vtkDistanceRepresentation vtkDistanceRepresentation2D


#ifndef __vtkDistanceRepresentation3D_h
#define __vtkDistanceRepresentation3D_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkDistanceRepresentation.h"

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


class VTKINTERACTIONWIDGETS_EXPORT vtkDistanceRepresentation3D : public vtkDistanceRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkDistanceRepresentation3D *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkDistanceRepresentation3D,vtkDistanceRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Satisfy the superclasses API.
  virtual double GetDistance()
    {return this->Distance;}

  // Description:
  // Scale the glyphs used as tick marks. By default it is
  // 1/40th of the length.
  void SetGlyphScale(double scale);
  vtkGetMacro(GlyphScale, double);

  // Description:
  // Convenience method to get the line actor property.
  virtual vtkProperty *GetLineProperty();

  // Description:
  // Set/Get position of the label title in normalized coordinates [0,1].
  // 0 is at the start of the line whereas 1 is at the end.
  void SetLabelPosition(double labelPosition);
  vtkGetMacro(LabelPosition, double);

  // Description:
  // Set/Get the maximum number of ticks in ruler mode.
  vtkSetClampMacro(MaximumNumberOfRulerTicks, int, 1, VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfRulerTicks, int);

  // Description:
  // Convenience method to get the glyph actor. Using this it is
  // possible to control the appearance of the glyphs.
  vtkGetObjectMacro(GlyphActor, vtkActor);

  // Description:
  // Convenience method Get the label actor. It is possible to
  // control the appearance of the label.
  vtkGetObjectMacro(LabelActor, vtkFollower);

  // Description:
  // Methods to Set/Get the coordinates of the two points defining
  // this representation. Note that methods are available for both
  // display and world coordinates.
  double* GetPoint1WorldPosition();
  double* GetPoint2WorldPosition();
  void GetPoint1WorldPosition(double pos[3]);
  void GetPoint2WorldPosition(double pos[3]);
  void SetPoint1WorldPosition(double pos[3]);
  void SetPoint2WorldPosition(double pos[3]);

  void SetPoint1DisplayPosition(double pos[3]);
  void SetPoint2DisplayPosition(double pos[3]);
  void GetPoint1DisplayPosition(double pos[3]);
  void GetPoint2DisplayPosition(double pos[3]);

  // Description:
  // Method to satisfy superclasses' API.
  virtual void BuildRepresentation();
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
  vtkDistanceRepresentation3D();
  ~vtkDistanceRepresentation3D();

  // The line
  vtkPoints         *LinePoints;
  vtkPolyData       *LinePolyData;
  vtkPolyDataMapper *LineMapper;
  vtkActor          *LineActor;

  // The distance label
  vtkVectorText     *LabelText;
  vtkPolyDataMapper *LabelMapper;
  vtkFollower       *LabelActor;

  // Support internal operations
  bool LabelScaleSpecified;

  // The 3D disk tick marks
  vtkPoints         *GlyphPoints;
  vtkDoubleArray    *GlyphVectors;
  vtkPolyData       *GlyphPolyData;
  vtkCylinderSource *GlyphCylinder;
  vtkTransformPolyDataFilter *GlyphXForm;
  vtkGlyph3D        *Glyph3D;
  vtkPolyDataMapper *GlyphMapper;
  vtkActor          *GlyphActor;

  // Glyph3D scale
  double GlyphScale;
  bool   GlyphScaleSpecified;

  // The distance between the two points
  double Distance;

  // Support GetBounds() method
  vtkBox *BoundingBox;

  // Maximum number of ticks on the 3d ruler
  int MaximumNumberOfRulerTicks;

  // Label title position
  double LabelPosition;

private:
  vtkDistanceRepresentation3D(const vtkDistanceRepresentation3D&);  //Not implemented
  void operator=(const vtkDistanceRepresentation3D&);  //Not implemented

  // Internal method to update the position of the label.
  void UpdateLabelPosition();
};

#endif
