/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkScaledTextActor - Create a text that will scale as needed
// .SECTION Description
// vtkScaledTextActor can be used to place text annotation into a window
// and have the font size scale so that the text always bounded by 
// a specified rectangle.
//
// .SECTION See Also
// vtkActor2D vtkTextMapper

#ifndef __vtkScaledTextActor_h
#define __vtkScaledTextActor_h

#include "vtkActor2D.h"
#include "vtkTextMapper.h"

class VTK_EXPORT vtkScaledTextActor : public vtkActor2D
{
public:
  vtkScaledTextActor();
  ~vtkScaledTextActor();
  const char *GetClassName() {return "vtkScaledTextActor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkScaledTextActor *New() {return new vtkScaledTextActor;};
  
  // Description:
  // Access the Position2 instance variable. This variable controls
  // the upper right corner of the ScaledText. It is by default
  // relative to Position1 and in Normalized Viewport coordinates.
  void SetPosition2(float,float);
  void SetPosition2(float x[2]);
  vtkCoordinate *GetPosition2Coordinate();
  float *GetPosition2();
  
  // Description:
  // Set/Get the vtkMapper2D which defines the data to be drawn.
  void SetMapper(vtkTextMapper *mapper);

  // Description:
  // Draw the scalar bar and annotation text to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* viewport) {return 0;};
  int RenderOverlay(vtkViewport* viewport);

  // Description:
  // Set/Get the height and width of the scalar bar. The value is expressed
  // as a fraction of the viewport. This really is just another way of
  // setting the Position2 instance variable.
  void SetWidth(float w);
  float GetWidth();
  void SetHeight(float h);
  float GetHeight();
  
protected:
  vtkActor2D *TextActor;
  vtkCoordinate *Position2Coordinate;
  vtkTimeStamp  BuildTime;
  int LastSize[2];
  int LastOrigin[2];
};


#endif

