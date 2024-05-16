// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMapper
 * @brief   2D image display
 *
 * vtkImageMapper provides 2D image display support for vtk.
 * It is a Mapper2D subclass that can be associated with an Actor2D
 * and placed within a RenderWindow or ImageWindow.
 * The vtkImageMapper is a 2D mapper, which means that it displays images
 * in display coordinates. In display coordinates, one image pixel is
 * always one screen pixel.
 *
 * @sa
 * vtkMapper2D vtkActor2D
 */

#ifndef vtkImageMapper_h
#define vtkImageMapper_h

#include "vtkMapper2D.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkWindow;
class vtkViewport;
class vtkActor2D;
class vtkImageData;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkImageMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkImageMapper, vtkMapper2D);
  static vtkImageMapper* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Override Modifiedtime as we have added a lookuptable
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the window value for window/level
   */
  vtkSetMacro(ColorWindow, double);
  vtkGetMacro(ColorWindow, double);
  ///@}

  ///@{
  /**
   * Set/Get the level value for window/level
   */
  vtkSetMacro(ColorLevel, double);
  vtkGetMacro(ColorLevel, double);
  ///@}

  ///@{
  /**
   * Set/Get the current slice number. The axis Z in ZSlice does not
   * necessarily have any relation to the z axis of the data on disk.
   * It is simply the axis orthogonal to the x,y, display plane.
   * GetWholeZMax and Min are convenience methods for obtaining
   * the number of slices that can be displayed. Again the number
   * of slices is in reference to the display z axis, which is not
   * necessarily the z axis on disk. (due to reformatting etc)
   */
  vtkSetMacro(ZSlice, int);
  vtkGetMacro(ZSlice, int);
  int GetWholeZMin();
  int GetWholeZMax();
  ///@}

  /**
   * Draw the image to the screen.
   */
  void RenderStart(vtkViewport* viewport, vtkActor2D* actor);

  /**
   * Function called by Render to actually draw the image to to the screen
   */
  virtual void RenderData(vtkViewport*, vtkImageData*, vtkActor2D*) {}

  ///@{
  /**
   * Methods used internally for performing the Window/Level mapping.
   */
  double GetColorShift();
  double GetColorScale();
  ///@}

  // Public for templated functions. * *  Should remove this * *
  int DisplayExtent[6];

  ///@{
  /**
   * Set the Input of a filter.
   */
  virtual void SetInputData(vtkImageData* input);
  vtkImageData* GetInput();
  ///@}

  ///@{
  /**
   * If RenderToRectangle is set (by default not), then the imagemapper
   * will render the image into the rectangle supplied by the Actor2D's
   * PositionCoordinate and Position2Coordinate
   */
  vtkSetMacro(RenderToRectangle, vtkTypeBool);
  vtkGetMacro(RenderToRectangle, vtkTypeBool);
  vtkBooleanMacro(RenderToRectangle, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Usually, the entire image is displayed, if UseCustomExtents
   * is set (by default not), then the region supplied in the
   * CustomDisplayExtents is used in preference.
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  vtkSetMacro(UseCustomExtents, vtkTypeBool);
  vtkGetMacro(UseCustomExtents, vtkTypeBool);
  vtkBooleanMacro(UseCustomExtents, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The image extents which should be displayed with UseCustomExtents
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  vtkSetVectorMacro(CustomDisplayExtents, int, 4);
  vtkGetVectorMacro(CustomDisplayExtents, int, 4);
  ///@}

protected:
  vtkImageMapper();
  ~vtkImageMapper() override;

  double ColorWindow;
  double ColorLevel;

  int PositionAdjustment[2];
  int ZSlice;
  vtkTypeBool UseCustomExtents;
  int CustomDisplayExtents[4];
  vtkTypeBool RenderToRectangle;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkImageMapper(const vtkImageMapper&) = delete;
  void operator=(const vtkImageMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
