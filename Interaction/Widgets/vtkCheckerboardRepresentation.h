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
/**
 * @class   vtkCheckerboardRepresentation
 * @brief   represent the vtkCheckerboardWidget
 *
 * The vtkCheckerboardRepresentation is used to implement the representation of
 * the vtkCheckerboardWidget. The user can adjust the number of divisions in
 * each of the i-j directions in a 2D image. A frame appears around the
 * vtkImageActor with sliders along each side of the frame. The user can
 * interactively adjust the sliders to the desired number of checkerboard
 * subdivisions. The representation uses four instances of
 * vtkSliderRepresentation3D to implement itself.
 *
 * @sa
 * vtkCheckerboardWidget vtkImageCheckerboard vtkImageActor vtkSliderWidget
 * vtkRectilinearWipeWidget
*/

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
  /**
   * Instantiate class.
   */
  static vtkCheckerboardRepresentation *New();

  //@{
  /**
   * Standard VTK methods.
   */
  vtkTypeMacro(vtkCheckerboardRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify an instance of vtkImageCheckerboard to manipulate.
   */
  void SetCheckerboard(vtkImageCheckerboard *chkrbrd);
  vtkGetObjectMacro(Checkerboard,vtkImageCheckerboard);
  //@}

  //@{
  /**
   * Specify an instance of vtkImageActor to decorate.
   */
  void SetImageActor(vtkImageActor *imageActor);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  //@}

  //@{
  /**
   * Specify the offset of the ends of the sliders (on the boundary edges of
   * the image) from the corner of the image. The offset is expressed as a
   * normalized fraction of the border edges.
   */
  vtkSetClampMacro(CornerOffset,double,0.0,0.4);
  vtkGetMacro(CornerOffset,double);
  //@}

  enum {
    TopSlider=0,
    RightSlider,
    BottomSlider,
    LeftSlider
  };

  /**
   * This method is invoked by the vtkCheckerboardWidget() when a value of some
   * slider has changed.
   */
  void SliderValueChanged(int sliderNum);

  //@{
  /**
   * Set and get the instances of vtkSliderRepresention used to implement this
   * representation. Normally default representations are created, but you can
   * specify the ones you want to use.
   */
  void SetTopRepresentation(vtkSliderRepresentation3D*);
  void SetRightRepresentation(vtkSliderRepresentation3D*);
  void SetBottomRepresentation(vtkSliderRepresentation3D*);
  void SetLeftRepresentation(vtkSliderRepresentation3D*);
  vtkGetObjectMacro(TopRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(RightRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(BottomRepresentation,vtkSliderRepresentation3D);
  vtkGetObjectMacro(LeftRepresentation,vtkSliderRepresentation3D);
  //@}

  //@{
  /**
   * Methods required by superclass.
   */
  void BuildRepresentation() override;
  void GetActors(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow *w) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  vtkCheckerboardRepresentation();
  ~vtkCheckerboardRepresentation() override;

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
  vtkCheckerboardRepresentation(const vtkCheckerboardRepresentation&) = delete;
  void operator=(const vtkCheckerboardRepresentation&) = delete;
};

#endif
