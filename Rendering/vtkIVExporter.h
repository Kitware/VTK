/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIVExporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Jon A. Webb of Visual Interface Inc.

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
// .NAME vtkIVExporter - export a scene into OpenInventor 2.0 format.
// .SECTION Description
// vtkIVExporter is a concrete subclass of vtkExporter that writes
// OpenInventor 2.0 files.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkIVExporter_h
#define __vtkIVExporter_h

#include <stdio.h>
#include "vtkExporter.h"

class VTK_RENDERING_EXPORT vtkIVExporter : public vtkExporter
{
public:
  static vtkIVExporter *New();
  vtkTypeMacro(vtkIVExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the OpenInventor file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkIVExporter();
  ~vtkIVExporter();

  void WriteData();
  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp);
  void WritePointData(vtkPoints *points, vtkDataArray *normals, 
		      vtkDataArray *tcoords, vtkUnsignedCharArray *colors, 
                      FILE *fp);
  char *FileName;
private:
  vtkIVExporter(const vtkIVExporter&);  // Not implemented.
  void operator=(const vtkIVExporter&);  // Not implemented.
};

#endif

