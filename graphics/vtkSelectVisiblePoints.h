/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkSelectVisiblePoints - extract points that are visible (based on z-buffer calculation)
// .SECTION Description
// vtkSelectVisiblePoints is a filter that selects points based on
// whether they are visible or not. Visibility is determined by
// accessing the z-buffer of a rendering window. (The position of each
// input point is converted into display coordinates, and then the
// z-value at that point is obtained. If within the user-specified
// tolerance, the point is considered visible.)
//
// Points that are visible (or if the ivar SelectInvisible is on,
// invisible points) are passed to the output. Associated data
// attributes are passed to the output as well.
//
// This filter also allows you to specify a rectangular window in display
// (pixel) coordinates in which the visible points must lie. This can be
// used as a sort of local "brushing" operation to select just data within
// a window.
// 
// .SECTION Caveats
// You must carefully synchronize the execution of this filter. The
// filter refers to a renderer, which is modified everytime a render
// occurs. Therefore, the filter is always out of date, and always
// executes. You may have to perform two rendering passes, or if you
// are using this filter in conjunction with vtkLabeledPointMapper,
// things work out because 2D rendering occurs after the 3D rendering.

#ifndef __vtkSelectVisiblePoints_h
#define __vtkSelectVisiblePoints_h

#include "vtkDataSetToPolyDataFilter.h"
class vtkRenderer;

class VTK_EXPORT vtkSelectVisiblePoints : public vtkDataSetToPolyDataFilter
{
public:

// Description:
// Instantiate object with no renderer; window selection turned off; 
// tolerance set to 0.01; and select invisible off.
  vtkSelectVisiblePoints();

  static vtkSelectVisiblePoints *New() {return new vtkSelectVisiblePoints;};
  const char *GetClassName() {return "vtkSelectVisiblePoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the renderer in which the visibility computation is to be
  // performed.
  vtkSetObjectMacro(Renderer,vtkRenderer);
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // Set/Get the flag which enables selection in a rectangular display
  // region.
  vtkSetMacro(SelectionWindow,int);
  vtkGetMacro(SelectionWindow,int);
  vtkBooleanMacro(SelectionWindow,int);

  // Description:
  // Specify the selection window in display coordinates. You must specify
  // a rectangular region using (xmin,xmax,ymin,ymax).
  vtkSetVector4Macro(Selection,int);
  vtkGetVectorMacro(Selection,int,4);

  // Description:
  // Set/Get the flag which enables inverse selection; i.e., invisible points
  // are selected.
  vtkSetMacro(SelectInvisible,int);
  vtkGetMacro(SelectInvisible,int);
  vtkBooleanMacro(SelectInvisible,int);

  // Description:
  // Set/Get a tolerance to use to determine whether a point is visible. A
  // tolerance is usually required because the conversion from world space
  // to display space during rendering introduces numerical round-off.
  vtkSetClampMacro(Tolerance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Tolerance,float);

  // Overload because we depend on renderer
  unsigned long int GetMTime();

protected:
  void Execute();

  vtkRenderer *Renderer;

  int SelectionWindow;
  int Selection[4];
  int SelectInvisible;
  float Tolerance;

};

#endif


