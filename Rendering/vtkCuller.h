/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCuller.h
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
// .NAME vtkCuller - a superclass for prop cullers
// .SECTION Description
// A culler has a cull method called by the vtkRenderer. The cull 
// method is called before any rendering is performed,
// and it allows the culler to do some processing on the props and 
// to modify their AllocatedRenderTime and re-order them in the prop list. 

// .SECTION see also
// vtkFrustumCoverageCuller

#ifndef __vtkCuller_h
#define __vtkCuller_h

#include "vtkObject.h"

class vtkProp;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkCuller : public vtkObject
{
public:
  vtkTypeMacro(vtkCuller,vtkObject);

  // Description:
  // This is called outside the render loop by vtkRenderer
  virtual float Cull( vtkRenderer *ren, vtkProp **propList,
		      int& listLength, int& initialized )=0;

protected:
  vtkCuller() {};
  ~vtkCuller() {};
private:
  vtkCuller(const vtkCuller&);  // Not implemented.
  void operator=(const vtkCuller&);    // Not implemented.
};
                                         
#endif
