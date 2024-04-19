// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageCroppingRegionsWidget
 * @brief   widget for cropping an image
 *
 * This widget displays a set of axis aligned lines that can be interactively
 * manipulated to crop a volume. The region to be cropped away is displayed
 * in a different highlight. Much like the vtkVolumeMapper, this widget
 * supports 27 possible configurations of cropping planes. (See
 * CroppingRegionFlags). If a volume mapper is set, the cropping planes
 * are directly propagated to the volume mapper. The widget invokes a
 * CroppingPlanesPositionChangedEvent when the position of any of the
 * cropping planes is changed. The widget also invokes an InteractionEvent
 * in response to user interaction.
 */

#ifndef vtkImageCroppingRegionsWidget_h
#define vtkImageCroppingRegionsWidget_h

#include "vtk3DWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkImageData;
class vtkLineSource;
class vtkVolumeMapper;
class vtkPolyData;

class VTKINTERACTIONWIDGETS_EXPORT vtkImageCroppingRegionsWidget : public vtk3DWidget
{
public:
  ///@{
  /**
   * Standard VTK methods.
   */
  static vtkImageCroppingRegionsWidget* New();
  vtkTypeMacro(vtkImageCroppingRegionsWidget, vtk3DWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Place/Adjust widget within bounds
   */
  using vtk3DWidget::PlaceWidget;
  void PlaceWidget(double bounds[6]) override;
  ///@}

  /**
   * Enable/disable the widget
   */
  void SetEnabled(int enabling) override;

  ///@{
  /**
   * Set/Get the plane positions that represent the cropped region.
   */
  vtkGetVector6Macro(PlanePositions, double);
  virtual void SetPlanePositions(double pos[6])
  {
    this->SetPlanePositions(pos[0], pos[1], pos[2], pos[3], pos[4], pos[5]);
  }
  virtual void SetPlanePositions(float pos[6])
  {
    this->SetPlanePositions(pos[0], pos[1], pos[2], pos[3], pos[4], pos[5]);
  }
  virtual void SetPlanePositions(
    double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  ///@}

  ///@{
  /**
   * Set/Get the cropping region flags
   */
  virtual void SetCroppingRegionFlags(int flags);
  vtkGetMacro(CroppingRegionFlags, int);
  ///@}

  /**
   * Set/get the slice orientation
   */

  enum
  {
    SLICE_ORIENTATION_YZ = 0,
    SLICE_ORIENTATION_XZ = 1,
    SLICE_ORIENTATION_XY = 2
  };

  vtkGetMacro(SliceOrientation, int);
  virtual void SetSliceOrientation(int orientation);
  virtual void SetSliceOrientationToXY()
  {
    this->SetSliceOrientation(vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY);
  }
  virtual void SetSliceOrientationToYZ()
  {
    this->SetSliceOrientation(vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ);
  }
  virtual void SetSliceOrientationToXZ()
  {
    this->SetSliceOrientation(vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ);
  }

  ///@{
  /**
   * Set/Get the slice number
   */
  virtual void SetSlice(int num);
  vtkGetMacro(Slice, int);
  ///@}

  ///@{
  /**
   * Set/Get line 1 color
   */
  virtual void SetLine1Color(double r, double g, double b);
  virtual void SetLine1Color(double rgb[3]) { this->SetLine1Color(rgb[0], rgb[1], rgb[2]); }
  virtual double* GetLine1Color();
  virtual void GetLine1Color(double rgb[3]);
  ///@}

  ///@{
  /**
   * Set/Get line 2 color
   */
  virtual void SetLine2Color(double r, double g, double b);
  virtual void SetLine2Color(double rgb[3]) { this->SetLine2Color(rgb[0], rgb[1], rgb[2]); }
  virtual double* GetLine2Color();
  virtual void GetLine2Color(double rgb[3]);
  ///@}

  ///@{
  /**
   * Set/Get line 3 color
   */
  virtual void SetLine3Color(double r, double g, double b);
  virtual void SetLine3Color(double rgb[3]) { this->SetLine3Color(rgb[0], rgb[1], rgb[2]); }
  virtual double* GetLine3Color();
  virtual void GetLine3Color(double rgb[3]);
  ///@}

  ///@{
  /**
   * Set/Get line 4 color
   */
  virtual void SetLine4Color(double r, double g, double b);
  virtual void SetLine4Color(double rgb[3]) { this->SetLine4Color(rgb[0], rgb[1], rgb[2]); }
  virtual double* GetLine4Color();
  virtual void GetLine4Color(double rgb[3]);
  ///@}

  ///@{
  /**
   * Set/Get the input volume mapper
   * Update the widget according to its mapper
   */
  virtual void SetVolumeMapper(vtkVolumeMapper* mapper);
  vtkGetObjectMacro(VolumeMapper, vtkVolumeMapper);
  virtual void UpdateAccordingToInput();
  ///@}

  ///@{
  /**
   * Callbacks for user interaction.
   */
  void MoveHorizontalLine();
  void MoveVerticalLine();
  void MoveIntersectingLines();
  void UpdateCursorIcon();
  void OnButtonPress();
  void OnButtonRelease();
  void OnMouseMove();
  ///@}

  /**
   * Events invoked by this widget
   */
  enum WidgetEventIds
  {
    CroppingPlanesPositionChangedEvent = 10050
  };

protected:
  vtkImageCroppingRegionsWidget();
  ~vtkImageCroppingRegionsWidget() override;

  vtkVolumeMapper* VolumeMapper;

  vtkLineSource* LineSources[4];
  vtkActor2D* LineActors[4];
  vtkPolyData* RegionPolyData[9];
  vtkActor2D* RegionActors[9];

  double PlanePositions[6];

  int SliceOrientation;
  int Slice;

  double GetSlicePosition();

  int CroppingRegionFlags;

  int MouseCursorState;
  int Moving;

  // Handles the events

  static void ProcessEvents(
    vtkObject* object, unsigned long event, void* clientdata, void* calldata);

  void SetMouseCursor(int state);

  enum WidgetStates
  {
    NoLine = 0,
    MovingH1AndV1,
    MovingH2AndV1,
    MovingH1AndV2,
    MovingH2AndV2,
    MovingV1,
    MovingV2,
    MovingH1,
    MovingH2
  };

  int ComputeWorldCoordinate(int x, int y, double* coord);

  void UpdateOpacity();
  void UpdateGeometry();
  void ConstrainPlanePositions(double positions[6]);

private:
  vtkImageCroppingRegionsWidget(const vtkImageCroppingRegionsWidget&) = delete;
  void operator=(const vtkImageCroppingRegionsWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
