/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPolyDataReader - read vtk polygonal data file
// .SECTION Description
// vtkPolyDataReader is a source object that reads ASCII or binary 
// polygonal data files in vtk format. See text for format details.

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkPolyDataReader_h
#define __vtkPolyDataReader_h

#include "vtkPolyDataSource.h"
#include "vtkDataReader.h"

class VTK_EXPORT vtkPolyDataReader : public vtkPolyDataSource
{
public:
  static vtkPolyDataReader *New();
  vtkTypeMacro(vtkPolyDataReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime also considering the vtkDataReader ivar
  unsigned long GetMTime();

  // Description:
  // Set / get the file name of vtk polygonal data file to read.
  void SetFileName(const char *name);
  char *GetFileName();

  // Description:
  // Get the header from the vtk data file.
  char *GetHeader() {return this->Reader->GetHeader();};

  // Description:
  // Specify the InputString for use when reading from a character array.
  void SetInputString(char *in) {this->Reader->SetInputString(in);}
  void SetInputString(char *in,int len) {this->Reader->SetInputString(in,len);}
  char *GetInputString() { return this->Reader->GetInputString();}
  void SetBinaryInputString(unsigned char *in, int len) {
      this->Reader->SetBinaryInputString(in,len);};

  // Description:
  // Set/Get reading from an InputString instead of the default, a file.
  void SetReadFromInputString(int i) {this->Reader->SetReadFromInputString(i);}
  int GetReadFromInputString() {return this->Reader->GetReadFromInputString();}
  vtkBooleanMacro(ReadFromInputString,int);

  // Description:
  // Get the type of file (ASCII or BINARY)
  int GetFileType();

  // Description:
  // Set / get the name of the scalar data to extract. If not specified, first 
  // scalar data encountered is extracted.
  void SetScalarsName(char *name);
  char *GetScalarsName();

  // Description:
  // Set / get the name of the vector data to extract. If not specified, first 
  // vector data encountered is extracted.
  void SetVectorsName(char *name);
  char *GetVectorsName();

  // Description:
  // Set / get the name of the tensor data to extract. If not specified, first 
  // tensor data encountered is extracted.
  void SetTensorsName(char *name);
  char *GetTensorsName();

  // Description:
  // Set / get the name of the normal data to extract. If not specified, first 
  // normal data encountered is extracted.
  void SetNormalsName(char *name);
  char *GetNormalsName();

  // Description:
  // Set / get the name of the texture coordinate data to extract. If not
  // specified, first texture coordinate data encountered is extracted.
  void SetTCoordsName(char *name);
  char *GetTCoordsName();

  // Description:
  // Set / get the name of the lookup table data to extract. If not
  // specified, uses lookup table named by scalar. Otherwise, this
  // specification supersedes.
  void SetLookupTableName(char *name);
  char *GetLookupTableName();

  // Description:
  // Set / get the name of the field data to extract. If not specified, uses 
  // first field data encountered in file.
  void SetFieldDataName(char *name);
  char *GetFieldDataName();

protected:
  vtkPolyDataReader();
  ~vtkPolyDataReader();
  vtkPolyDataReader(const vtkPolyDataReader&) {};
  void operator=(const vtkPolyDataReader&) {};

  void Execute();
  vtkDataReader *Reader;
};

#endif


