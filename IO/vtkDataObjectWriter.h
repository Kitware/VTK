/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

class VTK_IO_EXPORT vtkDataObjectWriter : public vtkWriter
{
public:
  static vtkDataObjectWriter *New();
  vtkTypeRevisionMacro(vtkDataObjectWriter,vtkWriter);
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

  void WriteData();
  vtkDataWriter *Writer;
  
private:
  vtkDataObjectWriter(const vtkDataObjectWriter&);  // Not implemented.
  void operator=(const vtkDataObjectWriter&);  // Not implemented.
};

#endif


