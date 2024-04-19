// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkDataWriter.h"     // Needs data because it calls methods on it
#include "vtkIOLegacyModule.h" // For export macro
#include "vtkStdString.h"      // For string used in api
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOLEGACY_EXPORT vtkDataObjectWriter : public vtkWriter
{
public:
  static vtkDataObjectWriter* New();
  vtkTypeMacro(vtkDataObjectWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods delegated to vtkDataWriter, see vtkDataWriter.
   */
  void SetFileName(VTK_FILEPATH const char* filename) { this->Writer->SetFileName(filename); }
  VTK_FILEPATH VTK_FUTURE_CONST char* GetFileName() VTK_FUTURE_CONST
  {
    return this->Writer->GetFileName();
  }
  void SetHeader(const char* header) { this->Writer->SetHeader(header); }
  char* GetHeader() { return this->Writer->GetHeader(); }
  void SetFileType(int type) { this->Writer->SetFileType(type); }
  int GetFileType() { return this->Writer->GetFileType(); }
  void SetFileTypeToASCII() { this->Writer->SetFileType(VTK_ASCII); }
  void SetFileTypeToBinary() { this->Writer->SetFileType(VTK_BINARY); }
  void SetWriteToOutputString(int b) { this->Writer->SetWriteToOutputString(b); }
  void WriteToOutputStringOn() { this->Writer->WriteToOutputStringOn(); }
  void WriteToOutputStringOff() { this->Writer->WriteToOutputStringOff(); }
  int GetWriteToOutputString() { return this->Writer->GetWriteToOutputString(); }
  char* GetOutputString() { return this->Writer->GetOutputString(); }
  vtkStdString GetOutputStdString() { return this->Writer->GetOutputStdString(); }
  vtkIdType GetOutputStringLength() { return this->Writer->GetOutputStringLength(); }
  unsigned char* GetBinaryOutputString() { return this->Writer->GetBinaryOutputString(); }
  void SetFieldDataName(const char* fieldname) { this->Writer->SetFieldDataName(fieldname); }
  char* GetFieldDataName() { return this->Writer->GetFieldDataName(); }
  ///@}

protected:
  vtkDataObjectWriter();
  ~vtkDataObjectWriter() override;

  void WriteData() override;
  vtkDataWriter* Writer;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDataObjectWriter(const vtkDataObjectWriter&) = delete;
  void operator=(const vtkDataObjectWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
