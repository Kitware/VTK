/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesWriter.h
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
// .NAME vtkMCubesWriter - write binary marching cubes file
// .SECTION Description
// vtkMCubesWriter is a polydata writer that writes binary marching cubes
// files. (Marching cubes is an isosurfacing technique that generates many
// triangles.) The binary format is supported by W. Lorensen's marching cubes
// program (and the vtkSliceCubes object). Each triangle is represented by
// three records, with each record consisting of six single precision
// floating point numbers representing the a triangle vertex coordinate and
// vertex normal.

// .SECTION Caveats
// Binary files are written in sun/hp/sgi (i.e., Big Endian) form.

// .SECTION See Also
// vtkMarchingCubes vtkSliceCubes vtkMCubesReader

#ifndef __vtkMCubesWriter_h
#define __vtkMCubesWriter_h

#include <stdio.h>
#include "vtkPolyDataWriter.h"

class VTK_EXPORT vtkMCubesWriter : public vtkPolyDataWriter
{
public:
  static vtkMCubesWriter *New();
  vtkTypeMacro(vtkMCubesWriter,vtkPolyDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get file name of marching cubes limits file.
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);

protected:
  vtkMCubesWriter();
  ~vtkMCubesWriter();
  vtkMCubesWriter(const vtkMCubesWriter&);
  void operator=(const vtkMCubesWriter&);

  void WriteData();
  char *LimitsFileName;
};

#endif


