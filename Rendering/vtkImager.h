/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImager.h
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
// .NAME vtkImager - Renders into part of a ImageWindow
// .SECTION Description
// vtkImager is the 2D counterpart to vtkRenderer. An Imager renders
// 2D actors into a viewport of an image window. 

// .SECTION See Also
//  vtkImageWindow vtkViewport
   

#ifndef __vtkImager_h
#define __vtkImager_h

#include "vtkObject.h"
#include "vtkActor2DCollection.h"
#include "vtkActor2D.h"
#include "vtkViewport.h"


class vtkImageWindow;

class VTK_RENDERING_EXPORT vtkImager : public vtkViewport
{ 
public:
  static vtkImager *New();
  vtkTypeMacro(vtkImager,vtkViewport);

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  virtual int RenderOpaqueGeometry();
  virtual int RenderTranslucentGeometry();
  virtual int RenderOverlay();

  // Description:
  // Get the image window that an imager is attached to.
  vtkImageWindow* GetImageWindow() {return (vtkImageWindow*) this->VTKWindow;};
  vtkWindow *GetVTKWindow() {return (vtkWindow*) this->VTKWindow;};

  
  //BTX
  // Description:
  // These set methods are used by the image window, and should not be
  // used by anyone else.  They do not reference count the window.
  void SetImageWindow (vtkImageWindow* win);
  void SetVTKWindow (vtkWindow* win);  
  //ETX
  
  // Description:
  // Erase the contents of the imager in the window.
  virtual void Erase(){vtkErrorMacro(<<"vtkImager::Erase - Not implemented!");};

  virtual vtkAssemblyPath* PickProp(float selectionX, float selectionY);
  virtual float GetPickedZ();

protected:
  vtkImager();
  ~vtkImager();
  vtkImager(const vtkImager&);
  void operator=(const vtkImager&);

  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick(); 
  virtual unsigned int GetPickedId();
  virtual void DevicePickRender();
};


#endif




