/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCheckerboardRepresentation - represent the vtkCheckerboardWidget
// .SECTION Description
// The vtkCheckerboardRepresentation is used to implement the representation of
// the vtkCheckerboardWidget. The user can adjust the number of divisions in
// each of the i-j directions in a 2D image. A frame appears around the
// vtkImageActor with sliders along each side of the frame. The user can
// interactively adjust the sliders to the desired number of checkerboard
// subdivisions. The representation uses four instances of
// vtkSliderRepresentation3D to implement itself.

// .SECTION See Also
// vtkCheckerboardWidget vtkImageCheckerboard vtkImageActor vtkSliderWidget
// vtkRectilinearWipeWidget


#ifndef vtkCheckerboardRepresentation_h
#define vtkCheckerboardRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"

class vtkImageCheckerboard;
class vtkImageActor;
class vtkSliderRepresentation3D;


class VTKINTERACTIONWIDGETS_EXPORT vtkCheckerboardRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkCheckerboardRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkCheckerboardRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkImageCheckerboard to manipulate.
  void SetCheckerboard(vtkImageCheckerboard *chkrbrd);
  vtkGetObjectMacro(Checkerboard,vtkImageCheckerboard);

  // Description:
  // Specify an instance of vtkImageActor to decorate.
  void SetImageActor(vtkImageActor *imageActor);
  vtkGetObjectMacro(ImageActor,vtkImageActor);

  // Description:
  // Specify the offset of the ends of the sliders (on the boundary edges of
  // the image) from the corner of the image. The offset is expressed as a
  // normalized fraction of the border edges.
  vtkSetClampMacro(CornerOffset,double,0.0,0.4);
  vtkGetMacro(CornerOffset,double);

//BTX
  enum {
    TopSlider=0,
    RightSlider,
    BottomSlider,
    LeftSlider
  };
//ETX

  // Description:
  // This method is invoked by the vtkCheckerboardWidget() when a value of some
  // slider has changed.
  void SliderValueChanged(int sliderNum);

  // Description:
  // Set and get the instances of vtkSliderRepresention used to implement this
  // representation. Normally default representations are created, but you can
  // specify the ones you want to use.
  void SetTopRepresentation(vtkSliderRepresentation3D*);
  void SetRightRepresentation(vtkSliderRepresentation3D*);
  void SetBottomRepresentation(vtkSliderRepresentation3D*);
  void SetLeftRepresentation(vtkSliderRepresentation3D*);
  vtkGetObjectMacro(TopRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(RightRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(BottomRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(LeftRepresentation,vtkSliderRepresentation3D);

  // Description:
  // Methods required by superclass.
  virtual void BuildRepresentation();
  virtual void GetActors(vtkPropCollection*);
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  vtkCheckerboardRepresentation();
  ~vtkCheckerboardRepresentation();

  // Instances that this class manipulates
  vtkImageCheckerboard *Checkerboard;
  vtkImageActor        *ImageActor;

  // The internal widgets for each side
  vtkSliderRepresentation3D *TopRepresentation;
  vtkSliderRepresentation3D *RightRepresentation;
  vtkSliderRepresentation3D *BottomRepresentation;
  vtkSliderRepresentation3D *LeftRepresentation;

  // The corner offset
  double CornerOffset;

  // Direction index of image actor's plane normal
  int OrthoAxis;

private:
  vtkCheckerboardRepresentation(const vtkCheckerboardRepresentation&);  //Not implemented
  void operator=(const vtkCheckerboardRepresentation&);  //Not implemented
};

#endif
