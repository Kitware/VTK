/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectVisiblePoints.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
// filter refers to a renderer, which is modified every time a render
// occurs. Therefore, the filter is always out of date, and always
// executes. You may have to perform two rendering passes, or if you
// are using this filter in conjunction with vtkLabeledPointMapper,
// things work out because 2D rendering occurs after the 3D rendering.

#ifndef __vtkSelectVisiblePoints_h
#define __vtkSelectVisiblePoints_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkSelectVisiblePoints : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkSelectVisiblePoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with no renderer; window selection turned off; 
  // tolerance set to 0.01; and select invisible off.
  static vtkSelectVisiblePoints *New();

  // Description:
  // Specify the renderer in which the visibility computation is to be
  // performed.
  void SetRenderer(vtkRenderer* ren)
    {
      if (this->Renderer != ren)
	{
	this->Renderer = ren;
	this->Modified();
	}
    }
  vtkRenderer* GetRenderer() { return this->Renderer; }

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

  // Description:
  // Return MTime also considering the renderer.
  unsigned long GetMTime();

protected:
  vtkSelectVisiblePoints();
  ~vtkSelectVisiblePoints();
  vtkSelectVisiblePoints(const vtkSelectVisiblePoints&);
  void operator=(const vtkSelectVisiblePoints&);

  void Execute();

  vtkRenderer *Renderer;

  int SelectionWindow;
  int Selection[4];
  int SelectInvisible;
  float Tolerance;

};

#endif


