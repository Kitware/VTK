/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageActor.h
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
// .NAME vtkMesaImageActor - Mesa texture map
// .SECTION Description
// vtkMesaImageActor is a concrete implementation of the abstract class 
// vtkImageActor. vtkMesaImageActor interfaces to the Mesa rendering library.

#ifndef __vtkMesaImageActor_h
#define __vtkMesaImageActor_h

#include "vtkImageActor.h"

class vtkWindow;
class vtkMesaRenderer;
class vtkRenderWindow;

class VTK_RENDERING_EXPORT vtkMesaImageActor : public vtkImageActor
{
public:
  static vtkMesaImageActor *New();
  vtkTypeMacro(vtkMesaImageActor,vtkImageActor);

  // Description:
  // Implement base class method.
  void Load(vtkRenderer *ren);
  
  // Description:
  // Release any graphics resources that are being consumed by this texture.
  // The parameter window could be used to determine which graphic
  // resources to release. Using the same texture object in multiple
  // render windows is NOT currently supported. 
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkMesaImageActor();
  ~vtkMesaImageActor();
  vtkMesaImageActor(const vtkMesaImageActor&);
  void operator=(const vtkMesaImageActor&);

  unsigned char *MakeDataSuitable(int &xsize, int &ysize, int &release);

  vtkTimeStamp   LoadTime;
  long          Index;
  static   long GlobalIndex;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for previous render
  float Coords[12];
  float TCoords[8];
};

#endif
