/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJExporter.h
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
// .NAME vtkOBJExporter - export a scene into Wavefront format.
// .SECTION Description
// vtkOBJExporter is a concrete subclass of vtkExporter that writes wavefront
// .OBJ files in ASCII form. It also writes out a mtl file that contains the
// material properties. The filenames are derived by appending the .obj and
// .mtl suffix onto the user specified FilePrefix.
//
// .SECTION See Also
// vtkExporter


#ifndef __vtkOBJExporter_h
#define __vtkOBJExporter_h

#include <stdio.h>
#include "vtkExporter.h"

class VTK_EXPORT vtkOBJExporter : public vtkExporter
{
public:
  static vtkOBJExporter *New();
  vtkTypeMacro(vtkOBJExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the prefix of the files to write out. The resulting filenames
  // will have .obj and .mtl appended to them.
  vtkSetStringMacro(FilePrefix);
  vtkGetStringMacro(FilePrefix);

protected:
  vtkOBJExporter();
  ~vtkOBJExporter();
  vtkOBJExporter(const vtkOBJExporter&);
  void operator=(const vtkOBJExporter&);

  void WriteData();
  void WriteAnActor(vtkActor *anActor, FILE *fpObj, FILE *fpMat, int &id);
  char *FilePrefix;
};

#endif

