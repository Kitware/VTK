/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderer.h
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
// .NAME vtkMesaRenderer - OpenGL renderer
// .SECTION Description
// vtkMesaRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkMesaRenderer interfaces to the mesa graphics library.
// This file is created, by a copy of vtkOpenGLRenderer

#ifndef __vtkMesaRenderer_h
#define __vtkMesaRenderer_h

#include <stdlib.h>
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkMesaRenderer : public vtkRenderer
{
protected:
  int NumberOfLightsBound;

public:
  static vtkMesaRenderer *New();
  vtkTypeMacro(vtkMesaRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Concrete open gl render method.
  void DeviceRender(void); 

  // Description:
  // Internal method temporarily removes lights before reloading them
  // into graphics pipeline.
  void ClearLights(void);

  void Clear(void);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  int UpdateLights(void);
  

  
  // Create a vtkMesaCamera, will be used by the super class
  // to create the correct camera object.
  virtual vtkCamera* MakeCamera();
  
  // Create a vtkMesaLight, will be used by the super class
  // to create the correct light object.
  virtual vtkLight* MakeLight();
  
protected:
  vtkMesaRenderer();
  ~vtkMesaRenderer();

  //BTX
  // Picking functions to be implemented by sub-classes
  virtual void DevicePickRender();
  virtual void StartPick(unsigned int pickFromSize);
  virtual void UpdatePickId();
  virtual void DonePick();
  virtual unsigned int GetPickedId();
  virtual float GetPickedZ();
  // Ivars used in picking
  class vtkGLPickInfo* PickInfo;
  //ETX
  float PickedZ;
private:
  vtkMesaRenderer(const vtkMesaRenderer&);  // Not implemented.
  void operator=(const vtkMesaRenderer&);  // Not implemented.
};

#endif
