/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkXImageMapper - 2D image display support for X windows
// .SECTION Description
// vtkXImageMapper is a concrete subclass of vtkImageMapper that
// renders images under X windows.

// .SECTION See Also
// vtkImageMapper

#ifndef __vtkXImageMapper_h
#define __vtkXImageMapper_h

#include "vtkImageMapper.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkImageData.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/X.h>
#include <X11/keysym.h>

class vtkActor2D;

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkXImageMapper : public vtkImageMapper
{
public:
  static vtkXImageMapper *New();

  vtkTypeMacro(vtkXImageMapper,vtkImageMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Handle the render method.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) {
    this->RenderStart(viewport,actor);}

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, vtkActor2D* actor);

  // Description:
  // Returns the depth of the X window
  int GetXWindowDepth(vtkWindow* window);

  // Description:
  // Returns the visual id of the window
  void GetXWindowVisualId(vtkWindow* window, Visual* visualID);

  // Description:
  // Returns the visual class of the window
  int GetXWindowVisualClass(vtkWindow* window);
 
  // Description:
  // Returns a pseudo color mapping from 0 255 to 50 199
  void GetXColors(int colors[]);

  // Description:
  // Returns the color masks used by the window.
  void GetXWindowColorMasks(vtkWindow *window, unsigned long *rmask,
      			    unsigned long *gmask, unsigned long *bmask);

  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);

protected:
  vtkXImageMapper();
  ~vtkXImageMapper();
  vtkXImageMapper(const vtkXImageMapper&);
  void operator=(const vtkXImageMapper&);

  XImage          *Image;
  unsigned char   *DataOut;
  int             DataOutSize;
  int             NumberOfColors;
};
#endif

#endif




