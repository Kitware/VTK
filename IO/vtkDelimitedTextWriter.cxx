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

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

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
}

//-----------------------------------------------------------------------------
vtkDelimitedTextWriter::~vtkDelimitedTextWriter()
{
  this->SetStringDelimiter(0);
  this->SetFieldDelimiter(0);
  this->SetFileName(0);
  delete this->Stream;
}

//-----------------------------------------------------------------------------
int vtkDelimitedTextWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkDelimitedTextWriter::OpenFile()
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
  return true;
}

//-----------------------------------------------------------------------------
template <class iterT>
void vtkDelimitedTextWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ofstream* stream, vtkDelimitedTextWriter* writer,
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
VTK_TEMPLATE_SPECIALIZE
void vtkDelimitedTextWriterGetDataString(
  vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex, 
  ofstream* stream, vtkDelimitedTextWriter* writer, bool* first)
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
  if (!this->OpenFile())
    {
    return;
    }

  vtkstd::vector<vtkSmartPointer<vtkArrayIterator> > columnsIters;

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

      vtksys_ios::ostringstream array_name;
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
    vtkstd::vector<vtkSmartPointer<vtkArrayIterator> >::iterator iter;
    for (iter = columnsIters.begin(); iter != columnsIters.end(); ++iter)
      {
      switch ((*iter)->GetDataType())
        {
        vtkArrayIteratorTemplateMacro(
          vtkDelimitedTextWriterGetDataString(static_cast<VTK_TT*>(iter->GetPointer()),
            index, this->Stream, this, &first));
        }
      }
    (*this->Stream) << "\n";
    }

  this->Stream->close();
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
}
