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
// .NAME vtkImageMapper - 2D image display
// .SECTION Description
// vtkImageMapper provides 2D image display support for vtk.
// It is a Mapper2D subclass that can be associated with an Actor2D
// and placed within a RenderWindow or ImageWindow.

// .SECTION See Also
// vtkMapper2D vtkActor2D

#ifndef __vtkImageMapper_h
#define __vtkImageMapper_h

#include "vtkMapper2D.h"

class vtkWindow;
class vtkViewport;
class vtkActor2D;
class vtkImageData;

class VTK_RENDERING_EXPORT vtkImageMapper : public vtkMapper2D
{
public:
  vtkTypeMacro(vtkImageMapper,vtkMapper2D);
  static vtkImageMapper *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Override Modifiedtime as we have added a lookuptable
  unsigned long int GetMTime();

  // Description:
  // Set/Get the window value for window/level
  vtkSetMacro(ColorWindow, double);
  vtkGetMacro(ColorWindow, double);
  
  // Description:
  // Set/Get the level value for window/level
  vtkSetMacro(ColorLevel, double);
  vtkGetMacro(ColorLevel, double);

  // Description:
  // Set/Get the current slice number. The axis Z in ZSlice does not
  // necessarily have any relation to the z axis of the data on disk.
  // It is simply the axis orthogonal to the x,y, display plane.
  // GetWholeZMax and Min are convenience methods for obtaining
  // the number of slices that can be displayed. Again the number
  // of slices is in reference to the display z axis, which is not
  // necessarily the z axis on disk. (due to reformatting etc)
  vtkSetMacro(ZSlice,int);
  vtkGetMacro(ZSlice,int);
  int GetWholeZMin();
  int GetWholeZMax();

  // Description:
  // Draw the image to the screen.
  void RenderStart(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Function called by Render to actually draw the image to to the screen
  virtual void RenderData(vtkViewport*, vtkImageData*, vtkActor2D* )=0;

  // Description:
  // Methods used internally for performing the Window/Level mapping.
  double GetColorShift();
  double GetColorScale();

  // Public for templated functions. * *  Should remove this * *
  int DisplayExtent[6];

  // Description:
  // Set the Input of a filter. 
  virtual void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // If RenderToRectangle is set (by default not), then the imagemapper
  // will render the image into the rectangle supplied by the Actor2D's
  // PositionCoordinate and Position2Coordinate
  vtkSetMacro(RenderToRectangle,int);
  vtkGetMacro(RenderToRectangle,int);
  vtkBooleanMacro(RenderToRectangle,int);

  // Description:
  // Usually, the entire image is displayed, if UseCustomExtents
  // is set (by default not), then the region supplied in the
  // CustomDisplayExtents is used in preference.
  // Note that the Custom extents are x,y only and the zslice is still
  // applied
  vtkSetMacro(UseCustomExtents,int);
  vtkGetMacro(UseCustomExtents,int);
  vtkBooleanMacro(UseCustomExtents,int);

  // Description:
  // The image extents which should be displayed with UseCustomExtents
  // Note that the Custom extents are x,y only and the zslice is still
  // applied
  vtkSetVectorMacro(CustomDisplayExtents,int,4);
  vtkGetVectorMacro(CustomDisplayExtents,int,4);

protected:
  vtkImageMapper();
  ~vtkImageMapper();

  double ColorWindow;
  double ColorLevel;

  int PositionAdjustment[2];
  int ZSlice;
  int UseCustomExtents;
  int CustomDisplayExtents[4];
  int RenderToRectangle;

  virtual int FillInputPortInformation(int, vtkInformation*);
private:
  vtkImageMapper(const vtkImageMapper&);  // Not implemented.
  void operator=(const vtkImageMapper&);  // Not implemented.
};



#endif


