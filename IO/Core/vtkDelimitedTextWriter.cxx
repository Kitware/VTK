// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkDelimitedTextWriter.h"

#include "vtkAlgorithm.h"
#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"
#include "vtksys/FStream.hxx"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDelimitedTextWriter);
//------------------------------------------------------------------------------
vtkDelimitedTextWriter::vtkDelimitedTextWriter()
{
  this->StringDelimiter = nullptr;
  this->FieldDelimiter = nullptr;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->Stream = nullptr;
  this->FileName = nullptr;
  this->WriteToOutputString = false;
  this->OutputString = nullptr;
}

//------------------------------------------------------------------------------
vtkDelimitedTextWriter::~vtkDelimitedTextWriter()
{
  this->SetStringDelimiter(nullptr);
  this->SetFieldDelimiter(nullptr);
  this->SetFileName(nullptr);
  delete this->Stream;
  delete[] this->OutputString;
}

//------------------------------------------------------------------------------
int vtkDelimitedTextWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
bool vtkDelimitedTextWriter::OpenStream()
{
  if (this->WriteToOutputString)
  {
    this->Stream = new std::ostringstream;
  }
  else
  {
    if (!this->FileName)
    {
      vtkErrorMacro(<< "No FileName specified! Can't write!");
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      return false;
    }

    vtkDebugMacro(<< "Opening file for writing...");

    vtksys::ofstream* fptr = new vtksys::ofstream(this->FileName, ios::out);

    if (fptr->fail())
    {
      vtkErrorMacro(<< "Unable to open file: " << this->FileName);
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      delete fptr;
      return false;
    }

    this->Stream = fptr;
  }

  if (this->Notation == SCIENTIFIC_NOTATION)
  {
    this->Stream->setf(ios::scientific, ios::floatfield);
  }
  else if (this->Notation == FIXED_NOTATION)
  {
    this->Stream->setf(ios::fixed, ios::floatfield);
  }
  else
  {
    // Standard notation
    this->Stream->unsetf(ios::floatfield);
  }

  if (this->Precision > 0)
  {
    this->Stream->precision(this->Precision);
  }

  return true;
}

//------------------------------------------------------------------------------
struct vtkDelimitedTextWriterGetDataString
{
  template <class TArray, class T = vtk::GetAPIType<TArray>>
  void operator()(TArray* array, vtkIdType tupleIndex, ostream* stream,
    vtkDelimitedTextWriter* writer, bool* first)
  {
    auto values = vtk::DataArrayValueRange<vtk::detail::DynamicTupleSize, T>(array);
    int numComps = array->GetNumberOfComponents();
    vtkIdType index = tupleIndex * numComps;
    for (int cc = 0; cc < numComps; cc++)
    {
      if ((index + cc) < array->GetNumberOfValues())
      {
        if (!*first)
        {
          (*stream) << writer->GetFieldDelimiter();
        }
        *first = false;
        if constexpr (std::is_same_v<TArray, vtkStringArray>)
        {
          (*stream) << writer->GetString(values[numComps * index + cc]);
        }
        else
        {
          (*stream) << values[numComps * index + cc];
        }
      }
      else
      {
        if (!*first)
        {
          (*stream) << writer->GetFieldDelimiter();
        }
        *first = false;
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkStdString vtkDelimitedTextWriter::GetString(vtkStdString string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
  {
    std::string temp = this->StringDelimiter;
    temp += string + this->StringDelimiter;
    return temp;
  }
  return string;
}

//------------------------------------------------------------------------------
bool vtkDelimitedTextWriter::WriteDataAndReturn()
{
  bool ret = false;
  ;
  vtkTable* rg = vtkTable::SafeDownCast(this->GetInput());
  if (rg)
  {
    ret = this->WriteTable(rg);
  }
  else
  {
    vtkErrorMacro(<< "CSVWriter can only write vtkTable.");
  }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkDelimitedTextWriter::WriteTable(vtkTable* table)
{
  vtkIdType numRows = table->GetNumberOfRows();
  vtkDataSetAttributes* dsa = table->GetRowData();
  if (!this->OpenStream())
  {
    return false;
  }

  int cc;
  int numArrays = dsa->GetNumberOfArrays();
  bool first = true;
  // Write headers:
  for (cc = 0; cc < numArrays; cc++)
  {
    vtkAbstractArray* array = dsa->GetAbstractArray(cc);
    for (int comp = 0; comp < array->GetNumberOfComponents(); comp++)
    {
      if (!first)
      {
        (*this->Stream) << this->FieldDelimiter;
      }
      first = false;

      std::ostringstream array_name;
      array_name << array->GetName();
      if (array->GetNumberOfComponents() > 1)
      {
        array_name << ":" << comp;
      }
      (*this->Stream) << this->GetString(array_name.str());
    }
  }
  (*this->Stream) << "\n";

  using Arrays =
    vtkTypeList::Append<vtkArrayDispatch::AllArrays, vtkStringArray, vtkVariantArray>::Result;
  vtkDelimitedTextWriterGetDataString getter;
  for (vtkIdType index = 0; index < numRows; index++)
  {
    first = true;
    for (cc = 0; cc < numArrays; cc++)
    {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      if (!vtkArrayDispatch::DispatchByArray<Arrays>::Execute(
            array, getter, index, this->Stream, this, &first))
      {
        if (auto da = vtkDataArray::SafeDownCast(array))
        {
          switch (da->GetDataType())
          {
            vtkTemplateMacro((getter.template operator()<vtkDataArray, VTK_TT>(
              da, index, this->Stream, this, &first)));
          }
        }
      }
    }
    (*this->Stream) << "\n";
  }

  if (this->WriteToOutputString)
  {
    std::ostringstream* ostr = static_cast<std::ostringstream*>(this->Stream);

    delete[] this->OutputString;
    size_t strLen = ostr->str().size();
    this->OutputString = new char[strLen + 1];
    memcpy(this->OutputString, ostr->str().c_str(), strLen + 1);
  }
  delete this->Stream;
  this->Stream = nullptr;
  return true;
}

//------------------------------------------------------------------------------
char* vtkDelimitedTextWriter::RegisterAndGetOutputString()
{
  char* tmp = this->OutputString;
  this->OutputString = nullptr;

  return tmp;
}

//------------------------------------------------------------------------------
void vtkDelimitedTextWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ? this->FieldDelimiter : "(none)")
     << endl;
  os << indent << "StringDelimiter: " << (this->StringDelimiter ? this->StringDelimiter : "(none)")
     << endl;
  os << indent << "UseStringDelimiter: " << this->UseStringDelimiter << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "none") << endl;
  os << indent << "WriteToOutputString: " << this->WriteToOutputString << endl;
}
VTK_ABI_NAMESPACE_END
