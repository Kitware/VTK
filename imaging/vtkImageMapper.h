/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageMapper - 2D image display
// .SECTION Description
// vtkImageMapper provides 2D image display support for vtk.
// It is a Mapper2D that can be associated with an Actor2D
// and placed withint a RenderWindow or ImageWindow.

// .SECTION See Also
// vtkMapper2D vtkActor2D

#ifndef __vtkImageMapper_h
#define __vtkImageMapper_h

#include "vtkMapper2D.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"

class vtkWindow;
class vtkViewport;
class vtkImageCache;
class vtkActor2D;
class vtkImageData;

class VTK_EXPORT vtkImageMapper : public vtkMapper2D
{
public:
  vtkImageMapper();

  const char *GetClassName() {return "vtkImageMapper";};
  static vtkImageMapper *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window value for window/level
  vtkSetMacro(ColorWindow, float);
  vtkGetMacro(ColorWindow, float);
  
  // Description:
  // Set/Get the level value for window/level
  vtkSetMacro(ColorLevel, float);
  vtkGetMacro(ColorLevel, float);

  // Description:
  // Set/Get the input for the image mapper.  The 
  // input can be either a vtkImageCache or a 
  // vtkStructuredPoints data set.
  vtkSetObjectMacro(Input, vtkImageCache);
  vtkGetObjectMacro(Input,vtkImageCache);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}

  // Description:
  // Set/Get the current slice number. The axis Z in ZSlice does not
  // neccessarily have any relation to the z axis of the data on disk.
  // It is simply the axis orthogonal to the x,y, display plane.
  // GetWholeZMax and Min are convinience methods for obtaining
  // the number of slices that can be displayed. Again the number
  // of slices is in reference to the display z axis, which is not
  // neccessarily the z axis on disk. (ue to reformating etc)
  vtkSetMacro(ZSlice,int);
  vtkGetMacro(ZSlice,int);
  int GetWholeZMin();
  int GetWholeZMax();

  // Description:
  // Draw the image to the screen.
  void Render(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Function called by Render to actually draw the image to
  // to the screen
  virtual void RenderData(vtkViewport* , vtkImageData *, vtkActor2D* )=0; 

  float GetColorShift();
  float GetColorScale();

protected:
  vtkImageCache* Input;
  float ColorWindow;
  float ColorLevel;
 
  int PositionAdjustment[2];
  int ZSlice;
};



#endif


