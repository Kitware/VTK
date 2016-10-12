/*=========================================================================

  Program:   ParaView
  Module:    vtkDelimitedTextWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkDelimitedTextWriter.h"

#include "vtkAlgorithm.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"

#include <vector>
#include <sstream>

vtkStandardNewMacro(vtkDelimitedTextWriter);
//-----------------------------------------------------------------------------
vtkDelimitedTextWriter::vtkDelimitedTextWriter()
{
  this->StringDelimiter = 0;
  this->FieldDelimiter = 0;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->Stream = 0;
  this->FileName = 0;
  this->WriteToOutputString = false;
  this->OutputString = 0;
}

//-----------------------------------------------------------------------------
vtkDelimitedTextWriter::~vtkDelimitedTextWriter()
{
  this->SetStringDelimiter(0);
  this->SetFieldDelimiter(0);
  this->SetFileName(0);
  delete this->Stream;
  delete[] this->OutputString;
}

//-----------------------------------------------------------------------------
int vtkDelimitedTextWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkDelimitedTextWriter::OpenStream()
{
  if (this->WriteToOutputString)
  {
    this->Stream = new std::ostringstream;
  }
  else
  {
    if ( !this->FileName )
    {
      vtkErrorMacro(<< "No FileName specified! Can't write!");
      this->SetErrorCode(vtkErrorCode::NoFileNameError);
      return false;
    }

    vtkDebugMacro(<<"Opening file for writing...");

    ofstream *fptr = new ofstream(this->FileName, ios::out);

    if (fptr->fail())
    {
      vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
      this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
      delete fptr;
      return false;
    }

    this->Stream = fptr;
  }

  return true;
}

//-----------------------------------------------------------------------------
template <class iterT>
void vtkDelimitedTextWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ostream* stream, vtkDelimitedTextWriter* writer,
  bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
  {
    if ((index+cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << iter->GetValue(index+cc);
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template<>
void vtkDelimitedTextWriterGetDataString(
  vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex,
  ostream* stream, vtkDelimitedTextWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
  {
    if ((index+cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << writer->GetString(iter->GetValue(index+cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkDelimitedTextWriter::GetString(vtkStdString string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
  {
    vtkStdString temp = this->StringDelimiter;
    temp += string + this->StringDelimiter;
    return temp;
  }
  return string;
}

//-----------------------------------------------------------------------------
void vtkDelimitedTextWriter::WriteData()
{
  vtkTable* rg = vtkTable::SafeDownCast(this->GetInput());
  if (rg)
  {
    this->WriteTable(rg);
  }
  else
  {
    vtkErrorMacro(<< "CSVWriter can only write vtkTable.");
  }
}

//-----------------------------------------------------------------------------
void vtkDelimitedTextWriter::WriteTable(vtkTable* table)
{
  vtkIdType numRows = table->GetNumberOfRows();
  vtkDataSetAttributes* dsa = table->GetRowData();
  if (!this->OpenStream())
  {
    return;
  }

  std::vector<vtkSmartPointer<vtkArrayIterator> > columnsIters;

  int cc;
  int numArrays = dsa->GetNumberOfArrays();
  bool first = true;
  // Write headers:
  for (cc=0; cc < numArrays; cc++)
  {
    vtkAbstractArray* array = dsa->GetAbstractArray(cc);
    for (int comp=0; comp < array->GetNumberOfComponents(); comp++)
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
    vtkArrayIterator* iter = array->NewIterator();
    columnsIters.push_back(iter);
    iter->Delete();
  }
  (*this->Stream) << "\n";

  for (vtkIdType index=0; index < numRows; index++)
  {
    first = true;
    std::vector<vtkSmartPointer<vtkArrayIterator> >::iterator iter;
    for (iter = columnsIters.begin(); iter != columnsIters.end(); ++iter)
    {
      switch ((*iter)->GetDataType())
      {
        vtkArrayIteratorTemplateMacro(
          vtkDelimitedTextWriterGetDataString(static_cast<VTK_TT*>(iter->GetPointer()),
            index, this->Stream, this, &first));
        case VTK_VARIANT:
        {
          vtkDelimitedTextWriterGetDataString(static_cast<vtkArrayIteratorTemplate<vtkVariant>*>(iter->GetPointer()),
            index, this->Stream, this, &first);
          break;
        }
      }
    }
    (*this->Stream) << "\n";
  }

  if (this->WriteToOutputString)
  {
    std::ostringstream *ostr =
      static_cast<std::ostringstream*>(this->Stream);

    delete [] this->OutputString;
    size_t strLen = ostr->str().size();
    this->OutputString = new char[strLen+1];
    memcpy(this->OutputString, ostr->str().c_str(), strLen+1);
  }
  delete this->Stream;
  this->Stream = 0;
}

//-----------------------------------------------------------------------------
char *vtkDelimitedTextWriter::RegisterAndGetOutputString()
{
  char *tmp = this->OutputString;
  this->OutputString = NULL;

  return tmp;
}

//-----------------------------------------------------------------------------
void vtkDelimitedTextWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ?
    this->FieldDelimiter : "(none)") << endl;
  os << indent << "StringDelimiter: " << (this->StringDelimiter ?
    this->StringDelimiter : "(none)") << endl;
  os << indent << "UseStringDelimiter: " << this->UseStringDelimiter << endl;
  os << indent << "FileName: " << (this->FileName? this->FileName : "none")
    << endl;
  os << indent << "WriteToOutputString: " << this->WriteToOutputString << endl;
}
