/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegendScaleActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLegendScaleActor - annotate the render window with scale and distance information
// .SECTION Description
// This class is used to annotate the render window. Its basic goal is to
// provide an indication of the scale of the scene. Four axes surrounding the
// render window indicate (in a variety of ways) the scale of what the camera
// is viewing. An option also exists for displaying a scale legend.
//
// The axes can be programmed either to display distance scales or x-y
// coordinate values. By default, the scales display a distance. However,
// if you know that the view is down the z-axis, the scales can be programmed
// to display x-y coordinate values.
//
// .SECTION Caveats
// Please be aware that the axes and scale values are subject to perspective
// effects. The distances are computed in the focal plane of the camera.
// When there are large view angles (i.e., perspective projection), the
// computed distances may provide users the wrong sense of scale. These
// effects are not present when parallel projection is enabled.

#ifndef __vtkLegendScaleActor_h
#define __vtkLegendScaleActor_h

#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkProp.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkAxisActor2D;
class vtkTextProperty;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkActor2D;
class vtkTextMapper;
class vtkPoints;
class vtkCoordinate;

class VTKRENDERINGANNOTATION_EXPORT vtkLegendScaleActor : public vtkProp
{
public:
  // Description:
  // Instantiate the class.
  static vtkLegendScaleActor *New();

  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkLegendScaleActor,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum AttributeLocation
  {
    DISTANCE=0,
    XY_COORDINATES=1
  };
//ETX

  // Description:
  // Specify the mode for labeling the scale axes. By default, the axes are
  // labeled with the distance between points (centered at a distance of
  // 0.0). Alternatively if you know that the view is down the z-axis; the
  // axes can be labeled with x-y coordinate values.
  vtkSetClampMacro(LabelMode,int,DISTANCE,XY_COORDINATES);
  vtkGetMacro(LabelMode,int);
  void SetLabelModeToDistance() {this->SetLabelMode(DISTANCE);}
  void SetLabelModeToXYCoordinates() {this->SetLabelMode(XY_COORDINATES);}

  // Description:
  // Set/Get the flags that control which of the four axes to display (top,
  // bottom, left and right). By default, all the axes are displayed.
  vtkSetMacro(RightAxisVisibility,int);
  vtkGetMacro(RightAxisVisibility,int);
  vtkBooleanMacro(RightAxisVisibility,int);
  vtkSetMacro(TopAxisVisibility,int);
  vtkGetMacro(TopAxisVisibility,int);
  vtkBooleanMacro(TopAxisVisibility,int);
  vtkSetMacro(LeftAxisVisibility,int);
  vtkGetMacro(LeftAxisVisibility,int);
  vtkBooleanMacro(LeftAxisVisibility,int);
  vtkSetMacro(BottomAxisVisibility,int);
  vtkGetMacro(BottomAxisVisibility,int);
  vtkBooleanMacro(BottomAxisVisibility,int);

  // Description:
  // Indicate whether the legend scale should be displayed or not.
  // The default is On.
  vtkSetMacro(LegendVisibility,int);
  vtkGetMacro(LegendVisibility,int);
  vtkBooleanMacro(LegendVisibility,int);

  // Description:
  // Convenience method that turns all the axes either on or off.
  void AllAxesOn();
  void AllAxesOff();

  // Description:
  // Convenience method that turns all the axes and the legend scale.
  void AllAnnotationsOn();
  void AllAnnotationsOff();

  // Description:
  // Set/Get the offset of the right axis from the border. This number is expressed in
  // pixels, and represents the approximate distance of the axes from the sides
  // of the renderer. The default is 50.
  vtkSetClampMacro(RightBorderOffset,int,5,VTK_LARGE_INTEGER);
  vtkGetMacro(RightBorderOffset,int);

  // Description:
  // Set/Get the offset of the top axis from the border. This number is expressed in
  // pixels, and represents the approximate distance of the axes from the sides
  // of the renderer. The default is 30.
  vtkSetClampMacro(TopBorderOffset,int,5,VTK_LARGE_INTEGER);
  vtkGetMacro(TopBorderOffset,int);

  // Description:
  // Set/Get the offset of the left axis from the border. This number is expressed in
  // pixels, and represents the approximate distance of the axes from the sides
  // of the renderer. The default is 50.
  vtkSetClampMacro(LeftBorderOffset,int,5,VTK_LARGE_INTEGER);
  vtkGetMacro(LeftBorderOffset,int);

  // Description:
  // Set/Get the offset of the bottom axis from the border. This number is expressed in
  // pixels, and represents the approximate distance of the axes from the sides
  // of the renderer. The default is 30.
  vtkSetClampMacro(BottomBorderOffset,int,5,VTK_LARGE_INTEGER);
  vtkGetMacro(BottomBorderOffset,int);

  // Description:
  // Get/Set the corner offset. This is the offset factor used to offset the
  // axes at the corners. Default value is 2.0.
  vtkSetClampMacro(CornerOffsetFactor, double, 1.0, 10.0);
  vtkGetMacro(CornerOffsetFactor, double);

  // Description:
  // Set/Get the labels text properties for the legend title and labels.
  vtkGetObjectMacro(LegendTitleProperty,vtkTextProperty);
  vtkGetObjectMacro(LegendLabelProperty,vtkTextProperty);

  // Description:
  // These are methods to retrieve the vtkAxisActors used to represent
  // the four axes that form this representation. Users may retrieve and
  // then modify these axes to control their appearance.
  vtkGetObjectMacro(RightAxis,vtkAxisActor2D);
  vtkGetObjectMacro(TopAxis,vtkAxisActor2D);
  vtkGetObjectMacro(LeftAxis,vtkAxisActor2D);
  vtkGetObjectMacro(BottomAxis,vtkAxisActor2D);

  // Decsription:
  // Standard methods supporting the rendering process.
  virtual void BuildRepresentation(vtkViewport *viewport);
  virtual void GetActors2D(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);

protected:
  vtkLegendScaleActor();
  ~vtkLegendScaleActor();

  int    LabelMode;
  int    RightBorderOffset;
  int    TopBorderOffset;
  int    LeftBorderOffset;
  int    BottomBorderOffset;
  double CornerOffsetFactor;

  // The four axes around the borders of the renderer
  vtkAxisActor2D *RightAxis;
  vtkAxisActor2D *TopAxis;
  vtkAxisActor2D *LeftAxis;
  vtkAxisActor2D *BottomAxis;

  // Control the display of the axes
  int RightAxisVisibility;
  int TopAxisVisibility;
  int LeftAxisVisibility;
  int BottomAxisVisibility;

  // Support for the legend.
  int                  LegendVisibility;
  vtkPolyData         *Legend;
  vtkPoints           *LegendPoints;
  vtkPolyDataMapper2D *LegendMapper;
  vtkActor2D          *LegendActor;
  vtkTextMapper       *LabelMappers[6];
  vtkActor2D          *LabelActors[6];
  vtkTextProperty     *LegendTitleProperty;
  vtkTextProperty     *LegendLabelProperty;
  vtkCoordinate       *Coordinate;

  vtkTimeStamp         BuildTime;

private:
  vtkLegendScaleActor(const vtkLegendScaleActor&);  //Not implemented
  void operator=(const vtkLegendScaleActor&);  //Not implemented
};

#endif
