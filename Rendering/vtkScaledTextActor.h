/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScaledTextActor.h
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
// .NAME vtkScaledTextActor - create text that will scale as needed
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
  vtkTypeMacro(vtkScaledTextActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkScaledTextActor *New();
  
  // Description:
  // Set/Get the vtkTextMapper that defines the text to be drawn.
  void SetMapper(vtkTextMapper *mapper);

  // Description:
  // Set/Get the minimum size in pixels for this actor.
  // Defaults to 10,10.
  vtkSetVector2Macro(MinimumSize,int);
  vtkGetVector2Macro(MinimumSize,int);
  
  // Description:
  // Set/Get the maximum height of a line of text as a 
  // percentage of the vertical area allocated to this
  // scaled text actor. Defaults to 1.0.
  vtkSetMacro(MaximumLineHeight,float);
  vtkGetMacro(MaximumLineHeight,float);
  
  // Description:
  // Shallow copy of this scaled text actor. Overloads the virtual 
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Draw the scaled text actor to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
//ETX

protected:
  vtkScaledTextActor();
  ~vtkScaledTextActor();
  vtkScaledTextActor(const vtkScaledTextActor&);
  void operator=(const vtkScaledTextActor&);

  int MinimumSize[2];
  float MaximumLineHeight;

  vtkActor2D *TextActor;
  vtkTimeStamp  BuildTime;
  int LastSize[2];
  int LastOrigin[2];

private:
  // hide the superclass' SetMapper method from the user and the compiler
  void SetMapper(vtkMapper2D *mapper) {this->vtkActor2D::SetMapper( mapper );};
};


#endif

