/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectWriter.h
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
// .NAME vtkDataObjectWriter - write vtk field data
// .SECTION Description
// vtkDataObjectWriter is a source object that writes ASCII or binary 
// field data files in vtk format. Field data is a general form of data in
// matrix form.

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

// .SECTION See Also
// vtkFieldData vtkFieldDataReader

#ifndef __vtkDataObjectWriter_h
#define __vtkDataObjectWriter_h

#include "vtkDataWriter.h"
#include "vtkFieldData.h"

class VTK_EXPORT vtkDataObjectWriter : public vtkWriter
{
public:
  static vtkDataObjectWriter *New();
  vtkTypeMacro(vtkDataObjectWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkDataObject *input);
  vtkDataObject *GetInput();
              
  // Description:
  // Methods delegated to vtkDataWriter, see vtkDataWriter.
  void SetFileName(const char *filename) {this->Writer->SetFileName(filename);};
  char *GetFileName() {return this->Writer->GetFileName();};
  void SetHeader(char *header) {this->Writer->SetHeader(header);};
  char *GetHeader() {return this->Writer->GetHeader();};
  void SetFileType(int type) {this->Writer->SetFileType(type);};
  int GetFileType() {return this->Writer->GetFileType();};
  void SetFileTypeToASCII() {this->Writer->SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->Writer->SetFileType(VTK_BINARY);};
  void SetFieldDataName(char *fieldname) {this->Writer->SetFieldDataName(fieldname);};
  char *GetFieldDataName() {return this->Writer->GetFieldDataName();};

  // Description:
  // For legacy compatibility. Do not use.
  void SetInput(vtkDataObject &input) 
    {this->SetInput(&input);}

protected:
  vtkDataObjectWriter();
  ~vtkDataObjectWriter();
  vtkDataObjectWriter(const vtkDataObjectWriter&);
  void operator=(const vtkDataObjectWriter&);

  void WriteData();
  vtkDataWriter *Writer;
  
};

#endif


