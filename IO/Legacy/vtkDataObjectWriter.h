/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataObjectWriter
 * @brief   write vtk field data
 *
 * vtkDataObjectWriter is a source object that writes ASCII or binary
 * field data files in vtk format. Field data is a general form of data in
 * matrix form.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 *
 * @sa
 * vtkFieldData vtkFieldDataReader
*/

#ifndef vtkDataObjectWriter_h
#define vtkDataObjectWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkWriter.h"
#include "vtkDataWriter.h" // Needs data because it calls methods on it
#include "vtkStdString.h" // For string used in api

class VTKIOLEGACY_EXPORT vtkDataObjectWriter : public vtkWriter
{
public:
  static vtkDataObjectWriter *New();
  vtkTypeMacro(vtkDataObjectWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Methods delegated to vtkDataWriter, see vtkDataWriter.
   */
  void SetFileName(const char *filename) {this->Writer->SetFileName(filename);};
  char *GetFileName() {return this->Writer->GetFileName();};
  void SetHeader(const char *header) {this->Writer->SetHeader(header);};
  char *GetHeader() {return this->Writer->GetHeader();};
  void SetFileType(int type) {this->Writer->SetFileType(type);};
  int GetFileType() {return this->Writer->GetFileType();};
  void SetFileTypeToASCII() {this->Writer->SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->Writer->SetFileType(VTK_BINARY);};
  void SetWriteToOutputString(int b) {this->Writer->SetWriteToOutputString(b);};
  void WriteToOutputStringOn() {this->Writer->WriteToOutputStringOn();};
  void WriteToOutputStringOff() {this->Writer->WriteToOutputStringOff();};
  int GetWriteToOutputString() {return this->Writer->GetWriteToOutputString();};
  char* GetOutputString() {return this->Writer->GetOutputString();};
  vtkStdString GetOutputStdString() {return this->Writer->GetOutputStdString();};
  int GetOutputStringLength() {return this->Writer->GetOutputStringLength();};
  unsigned char* GetBinaryOutputString() {return this->Writer->GetBinaryOutputString();};
  void SetFieldDataName(const char *fieldname) {this->Writer->SetFieldDataName(fieldname);};
  char *GetFieldDataName() {return this->Writer->GetFieldDataName();};
  //@}

protected:
  vtkDataObjectWriter();
  ~vtkDataObjectWriter();

  void WriteData();
  vtkDataWriter *Writer;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkDataObjectWriter(const vtkDataObjectWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataObjectWriter&) VTK_DELETE_FUNCTION;
};

#endif
