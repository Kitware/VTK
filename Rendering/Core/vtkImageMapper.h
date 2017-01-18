/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper2D.h"

class vtkWindow;
class vtkViewport;
class vtkActor2D;
class vtkImageData;

class VTKRENDERINGCORE_EXPORT vtkImageMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkImageMapper, vtkMapper2D);
  static vtkImageMapper *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Override Modifiedtime as we have added a lookuptable
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the window value for window/level
   */
  vtkSetMacro(ColorWindow, double);
  vtkGetMacro(ColorWindow, double);
  //@}

  //@{
  /**
   * Set/Get the level value for window/level
   */
  vtkSetMacro(ColorLevel, double);
  vtkGetMacro(ColorLevel, double);
  //@}

  //@{
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
  //@}

  /**
   * Draw the image to the screen.
   */
  void RenderStart(vtkViewport* viewport, vtkActor2D* actor);

  /**
   * Function called by Render to actually draw the image to to the screen
   */
  virtual void RenderData(vtkViewport*, vtkImageData*, vtkActor2D* )=0;

  //@{
  /**
   * Methods used internally for performing the Window/Level mapping.
   */
  double GetColorShift();
  double GetColorScale();
  //@}

  // Public for templated functions. * *  Should remove this * *
  int DisplayExtent[6];

  //@{
  /**
   * Set the Input of a filter.
   */
  virtual void SetInputData(vtkImageData *input);
  vtkImageData *GetInput();
  //@}

  //@{
  /**
   * If RenderToRectangle is set (by default not), then the imagemapper
   * will render the image into the rectangle supplied by the Actor2D's
   * PositionCoordinate and Position2Coordinate
   */
  vtkSetMacro(RenderToRectangle, int);
  vtkGetMacro(RenderToRectangle, int);
  vtkBooleanMacro(RenderToRectangle, int);
  //@}

  //@{
  /**
   * Usually, the entire image is displayed, if UseCustomExtents
   * is set (by default not), then the region supplied in the
   * CustomDisplayExtents is used in preference.
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  vtkSetMacro(UseCustomExtents, int);
  vtkGetMacro(UseCustomExtents, int);
  vtkBooleanMacro(UseCustomExtents, int);
  //@}

  //@{
  /**
   * The image extents which should be displayed with UseCustomExtents
   * Note that the Custom extents are x,y only and the zslice is still
   * applied
   */
  vtkSetVectorMacro(CustomDisplayExtents, int, 4);
  vtkGetVectorMacro(CustomDisplayExtents, int, 4);
  //@}

protected:
  vtkImageMapper();
  ~vtkImageMapper() VTK_OVERRIDE;

  double ColorWindow;
  double ColorLevel;

  int PositionAdjustment[2];
  int ZSlice;
  int UseCustomExtents;
  int CustomDisplayExtents[4];
  int RenderToRectangle;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
private:
  vtkImageMapper(const vtkImageMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageMapper&) VTK_DELETE_FUNCTION;
};

#endif
