/*=========================================================================
  
   Program:   Visualization Toolkit
   Module:    vtkOOGLExporter.h
   Language:  C++
   Date:      $Date$
   Version:   $Revision$
   Thanks:    to Jeremy D. Gill of The J. P. Robarts Research Institute
 
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
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS''
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

// .NAME vtkOOGLExporter - export a scene into Geomview OOGL format.
// .SECTION Description
// vtkOOGLExporter is a concrete subclass of vtkExporter that writes
// Geomview OOGL files.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkOOGLExporter_h
#define __vtkOOGLExporter_h

#include <stdio.h>
#include "vtkExporter.h"

class VTK_EXPORT vtkOOGLExporter : public vtkExporter
{
public:
  static vtkOOGLExporter *New();
  vtkTypeMacro(vtkOOGLExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the Geomview file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkOOGLExporter();
  ~vtkOOGLExporter();

  void WriteData();
  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp, int count);
  void WritePointData(vtkPoints *points, vtkNormals *normals, 
                      vtkTCoords *tcoords, vtkScalars *colors, FILE *fp);
  char *FileName;

private:
  vtkOOGLExporter(const vtkOOGLExporter&);
  void operator=(const vtkOOGLExporter&);
};

#endif

