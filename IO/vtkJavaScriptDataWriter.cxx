/*=========================================================================

  Program:   ParaView
  Module:    vtkJavaScriptDataWriter.cxx

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

#include "vtkJavaScriptDataWriter.h"

#include "vtkAlgorithm.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkJavaScriptDataWriter);
vtkCxxRevisionMacro(vtkJavaScriptDataWriter, "1.2");
//-----------------------------------------------------------------------------
vtkJavaScriptDataWriter::vtkJavaScriptDataWriter()
{
  this->Stream = 0;
  this->FileName = 0;
}

//-----------------------------------------------------------------------------
vtkJavaScriptDataWriter::~vtkJavaScriptDataWriter()
{
  this->SetFileName(0);
  delete this->Stream;
}

//-----------------------------------------------------------------------------
int vtkJavaScriptDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkJavaScriptDataWriter::OpenFile()
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
void vtkJavaScriptDataWriter::WriteData()
{
  vtkTable* rg = vtkTable::SafeDownCast(this->GetInput());
  if (rg)
    {
    this->WriteTable(rg);
    }
  else
    {
    vtkErrorMacro(<< "vtkJavaScriptDataWriter can only write vtkTable.");
    }
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::WriteTable(vtkTable* table)
{
  vtkIdType numRows = table->GetNumberOfRows();
  vtkIdType numCols = table->GetNumberOfColumns();
  vtkDataSetAttributes* dsa = table->GetRowData();
  if (!this->OpenFile())
    {
    return;
    }

  // Header stuff
  (*this->Stream) << "var data = [\n";

  // For each row
  for ( vtkIdType r = 0; r < numRows; ++ r )
    {
    // row header
    (*this->Stream) << "{ ";

    // Now for each column put out in the form 
    // colname1: data1, colname2: data2, etc
    for ( int c = 0; c < numCols; ++ c )
      {
      (*this->Stream) << dsa->GetAbstractArray(c)->GetName() << ":";

      // If the array is a string array put "" around it
      if (vtkStringArray::SafeDownCast(dsa->GetAbstractArray(c)))
        {
        (*this->Stream) << "\"" << table->GetValue( r, c ).ToString() << "\",";
        }
      else
        {
        (*this->Stream) << table->GetValue( r, c ).ToString() << ",";
        }
      }

    // row footer
    (*this->Stream) << " },\n";
    }

  // Footer
  (*this->Stream) << "];\n";

  this->Stream->close();
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName? this->FileName : "none") 
    << endl;
}
