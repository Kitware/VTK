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

#include <vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkJavaScriptDataWriter);
//-----------------------------------------------------------------------------
vtkJavaScriptDataWriter::vtkJavaScriptDataWriter()
{
  this->VariableName = 0;
  this->FileName = 0;
  this->IncludeFieldNames = true; // Default is to include field names
  this->OutputStream = 0;
  this->SetVariableName( "data" ); // prepare the default.
}

//-----------------------------------------------------------------------------
vtkJavaScriptDataWriter::~vtkJavaScriptDataWriter()
{
  this->SetFileName( 0 );
  this->SetVariableName( 0 );
}

//-----------------------------------------------------------------------------
int vtkJavaScriptDataWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::SetOutputStream(ostream *output_stream)
{
  this->OutputStream = output_stream;
}

//-----------------------------------------------------------------------------
ostream* vtkJavaScriptDataWriter::GetOutputStream()
{
  return this->OutputStream;
}

//-----------------------------------------------------------------------------
ofstream* vtkJavaScriptDataWriter::OpenFile()
{
  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return 0;
    }

  vtkDebugMacro(<<"Opening file for writing...");

  ofstream *fptr = new ofstream(this->FileName, ios::out);

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return 0;
    }

  return fptr;
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::WriteData()
{
  vtkTable* input_table = vtkTable::SafeDownCast(this->GetInput());

  // Check for valid input
  if (!input_table) 
    {
    vtkErrorMacro(<< "vtkJavaScriptDataWriter can only write vtkTable.");
    return;
    }

  // Check for filename
  if (this->FileName)
    {
    ofstream *file_stream = this->OpenFile();
    if (file_stream)
      {
      this->WriteTable(input_table,file_stream);
      }
    file_stream->close();
    }

  else if (this->OutputStream)
    {
    this->WriteTable(input_table,this->OutputStream);
    }
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::WriteTable(vtkTable* table, ostream *stream_ptr)
{
  vtkIdType numRows = table->GetNumberOfRows();
  vtkIdType numCols = table->GetNumberOfColumns();
  vtkDataSetAttributes* dsa = table->GetRowData();
  if (this->FileName && !this->OpenFile())
    {
    return;
    }

  vtkStdString rowHeader = "[";
  vtkStdString rowFooter = "],";
  if (this->IncludeFieldNames)
    {
    rowHeader = "{";
    rowFooter = "},";
    }

  // Header stuff
  if ( this->VariableName )
    {
    (*stream_ptr) << "var " << this->VariableName << " = [\n";
    }
  else
    {
    (*stream_ptr) << "[";
    }

  // For each row
  for ( vtkIdType r = 0; r < numRows; ++ r )
    {
    // row header
    (*stream_ptr) << rowHeader;

    // Now for each column put out in the form 
    // colname1: data1, colname2: data2, etc
    for ( int c = 0; c < numCols; ++ c )
      {
      if (this->IncludeFieldNames)
        {
        (*stream_ptr) << dsa->GetAbstractArray(c)->GetName() << ":";
        }

      // If the array is a string array put "" around it
      if (vtkStringArray::SafeDownCast(dsa->GetAbstractArray(c)))
        {
        (*stream_ptr) << "\"" << table->GetValue( r, c ).ToString() << "\",";
        }
      else
        {
        (*stream_ptr) << table->GetValue( r, c ).ToString() << ",";
        }
      }

    // row footer
    (*stream_ptr) << rowFooter;
    }

  // Footer
  (*stream_ptr) << ( this->VariableName ? "];\n" : "]" );
}

//-----------------------------------------------------------------------------
void vtkJavaScriptDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VariableName: " << this->VariableName << endl;
  os << indent << "FileName: " << (this->FileName? this->FileName : "none") 
    << endl;
  os << indent << "IncludeFieldNames: " << 
    (this->IncludeFieldNames ? "true" : "false") << endl;
}
