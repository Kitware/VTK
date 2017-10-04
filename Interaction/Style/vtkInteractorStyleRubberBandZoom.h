/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleRubberBandZoom.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInteractorStyleRubberBandZoom
 * @brief   zoom in by amount indicated by rubber band box
 *
 * This interactor style allows the user to draw a rectangle in the render
 * window using the left mouse button.  When the mouse button is released,
 * the current camera zooms by an amount determined from the shorter side of
 * the drawn rectangle.
*/

#ifndef vtkInteractorStyleRubberBandZoom_h
#define vtkInteractorStyleRubberBandZoom_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkRect.h" // for vtkRecti

class vtkUnsignedCharArray;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleRubberBandZoom : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleRubberBandZoom *New();
  vtkTypeMacro(vtkInteractorStyleRubberBandZoom, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * When set to true (default, false), the interactor will lock the rendered box to the
   * viewport's aspect ratio.
   */
  vtkSetMacro(LockAspectToViewport, bool);
  vtkGetMacro(LockAspectToViewport, bool);
  vtkBooleanMacro(LockAspectToViewport, bool);
  //@}

  //@{
  /**
   * When set to true (default, false), the position where the user starts the
   * interaction is treated as the center of the box rather that one of the
   * corners of the box.
   */
  vtkSetMacro(CenterAtStartPosition, bool);
  vtkGetMacro(CenterAtStartPosition, bool);
  vtkBooleanMacro(CenterAtStartPosition, bool);
  //@}

  //@{
  /**
   * If camera is in perspective projection mode, this interactor style uses
   * vtkCamera::Dolly to dolly the camera ahead for zooming. However, that can
   * have unintended consequences such as the camera entering into the data.
   * Another option is to use vtkCamera::Zoom instead. In that case, the camera
   * position is left unchanged, instead the focal point is changed to the
   * center of the target box and then the view angle is changed to zoom in.
   * To use this approach, set this parameter to false (default, true).
   */
  vtkSetMacro(UseDollyForPerspectiveProjection, bool);
  vtkGetMacro(UseDollyForPerspectiveProjection, bool);
  vtkBooleanMacro(UseDollyForPerspectiveProjection, bool);
  //@}

  //@{
  /**
   * Event bindings
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  //@}

protected:
  vtkInteractorStyleRubberBandZoom();
  ~vtkInteractorStyleRubberBandZoom() override;

  void Zoom() override;

  int StartPosition[2];
  int EndPosition[2];
  int Moving;
  bool LockAspectToViewport;
  bool CenterAtStartPosition;
  bool UseDollyForPerspectiveProjection;
  vtkUnsignedCharArray *PixelArray;
private:
  vtkInteractorStyleRubberBandZoom(const vtkInteractorStyleRubberBandZoom&) = delete;
  void operator=(const vtkInteractorStyleRubberBandZoom&) = delete;

  /**
   * Adjust the box based on this->LockAspectToViewport and
   * this->CenterAtStartPosition state. This may modify startPosition,
   * endPosition or both.
   */
  void AdjustBox(int startPosition[2], int endPosition[2]) const;

  void ZoomTraditional(const vtkRecti& box);
  void ZoomPerspectiveProjectionUsingViewAngle(const vtkRecti& box);
};

#endif
