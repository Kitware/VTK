/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBYUReader.h
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
// .NAME vtkBYUReader - read MOVIE.BYU polygon files
// .SECTION Description
// vtkBYUReader is a source object that reads MOVIE.BYU polygon files.
// These files consist of a geometry file (.g), a scalar file (.s), a 
// displacement or vector file (.d), and a 2D texture coordinate file
// (.t).

#ifndef __vtkBYUReader_h
#define __vtkBYUReader_h

#include <stdio.h>
#include "vtkPolyDataSource.h"

class VTK_IO_EXPORT vtkBYUReader : public vtkPolyDataSource 
{
public:
  static vtkBYUReader *New();

  vtkTypeMacro(vtkBYUReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify name of geometry FileName.
  vtkSetStringMacro(GeometryFileName);
  vtkGetStringMacro(GeometryFileName);

  // Description:
  // Specify name of displacement FileName.
  vtkSetStringMacro(DisplacementFileName);
  vtkGetStringMacro(DisplacementFileName);

  // Description:
  // Specify name of scalar FileName.
  vtkSetStringMacro(ScalarFileName);
  vtkGetStringMacro(ScalarFileName);

  // Description:
  // Specify name of texture coordinates FileName.
  vtkSetStringMacro(TextureFileName);
  vtkGetStringMacro(TextureFileName);

  // Description:
  // Turn on/off the reading of the displacement file.
  vtkSetMacro(ReadDisplacement,int);
  vtkGetMacro(ReadDisplacement,int);
  vtkBooleanMacro(ReadDisplacement,int);
  
  // Description:
  // Turn on/off the reading of the scalar file.
  vtkSetMacro(ReadScalar,int);
  vtkGetMacro(ReadScalar,int);
  vtkBooleanMacro(ReadScalar,int);
  
  // Description:
  // Turn on/off the reading of the texture coordinate file.
  // Specify name of geometry FileName.
  vtkSetMacro(ReadTexture,int);
  vtkGetMacro(ReadTexture,int);
  vtkBooleanMacro(ReadTexture,int);

  // Description:
  // Set/Get the part number to be read.
  vtkSetClampMacro(PartNumber,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(PartNumber,int);

protected:
  vtkBYUReader();
  ~vtkBYUReader();

  void Execute();
  // This source does not know how to generate pieces yet.
  int ComputeDivisionExtents(vtkDataObject *output, 
			     int idx, int numDivisions);

  char *GeometryFileName;
  char *DisplacementFileName;
  char *ScalarFileName;
  char *TextureFileName;
  int ReadDisplacement;
  int ReadScalar;
  int ReadTexture;
  int PartNumber;

  void ReadGeometryFile(FILE *fp, int &numPts);
  void ReadDisplacementFile(int numPts);
  void ReadScalarFile(int numPts);
  void ReadTextureFile(int numPts);
private:
  vtkBYUReader(const vtkBYUReader&);  // Not implemented.
  void operator=(const vtkBYUReader&);  // Not implemented.
};

#endif


