/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaActor.h
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
// .NAME vtkMesaActor - Mesa actor
// .SECTION Description
// vtkMesaActor is a concrete implementation of the abstract class vtkActor.
// vtkMesaActor interfaces to the Mesa rendering library.

#ifndef __vtkMesaActor_h
#define __vtkMesaActor_h

#include "vtkActor.h"

class vtkMesaRenderer;

class VTK_RENDERING_EXPORT vtkMesaActor : public vtkActor
{
protected:
  
public:
  static vtkMesaActor *New();
  vtkTypeMacro(vtkMesaActor,vtkActor);

  // Description:
  // Actual actor render method.
  void Render(vtkRenderer *ren, vtkMapper *mapper);
  
  // Description:
  // Create a vtkMesaProperty, used by the super class to 
  // create a compatible class with the vtkMesaActor.
  vtkProperty* MakeProperty();
protected:
  vtkMesaActor() {};
  ~vtkMesaActor() {};
  vtkMesaActor(const vtkMesaActor&);
  void operator=(const vtkMesaActor&);

};

#endif

