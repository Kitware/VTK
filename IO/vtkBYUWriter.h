/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUWriter.h
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
// .NAME vtkBYUWriter - write MOVIE.BYU files
// .SECTION Description
// vtkBYUWriter writes MOVIE.BYU polygonal files. These files consist 
// of a geometry file (.g), a scalar file (.s), a displacement or 
// vector file (.d), and a 2D texture coordinate file (.t). These files 
// must be specified to the object, the appropriate boolean 
// variables must be true, and data must be available from the input
// for the files to be written.
// WARNING: this writer does not currently write triangle strips. Use
// vtkTriangleFilter to convert strips to triangles.

#ifndef __vtkBYUWriter_h
#define __vtkBYUWriter_h

#include <stdio.h>
#include "vtkPolyDataWriter.h"
#include "vtkPolyData.h"

class VTK_IO_EXPORT vtkBYUWriter : public vtkPolyDataWriter
{
public:
  static vtkBYUWriter *New();

  vtkTypeMacro(vtkBYUWriter,vtkPolyDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the geometry file to write.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);

  // Description:
  // Specify the name of the displacement file to write.
  vtkSetStringMacro(DisplacementFileName);
  vtkGetStringMacro(DisplacementFileName);

  // Description:
  // Specify the name of the scalar file to write.
  vtkSetStringMacro(ScalarFileName);
  vtkGetStringMacro(ScalarFileName);

  // Description:
  // Specify the name of the texture file to write.
  vtkSetStringMacro(TextureFileName);
  vtkGetStringMacro(TextureFileName);

  // Description:
  // Turn on/off writing the displacement file.
  vtkSetMacro(WriteDisplacement,int);
  vtkGetMacro(WriteDisplacement,int);
  vtkBooleanMacro(WriteDisplacement,int);
  
  // Description:
  // Turn on/off writing the scalar file.
  vtkSetMacro(WriteScalar,int);
  vtkGetMacro(WriteScalar,int);
  vtkBooleanMacro(WriteScalar,int);
  
  // Description:
  // Turn on/off writing the texture file.
  vtkSetMacro(WriteTexture,int);
  vtkGetMacro(WriteTexture,int);
  vtkBooleanMacro(WriteTexture,int);

protected:
  vtkBYUWriter();
  ~vtkBYUWriter();
  vtkBYUWriter(const vtkBYUWriter&);
  void operator=(const vtkBYUWriter&);

  void WriteData();

  char *GeometryFileName;
  char *DisplacementFileName;
  char *ScalarFileName;
  char *TextureFileName;
  int WriteDisplacement;
  int WriteScalar;
  int WriteTexture;

  void WriteGeometryFile(FILE *fp, int numPts);
  void WriteDisplacementFile(int numPts);
  void WriteScalarFile(int numPts);
  void WriteTextureFile(int numPts);
};

#endif

