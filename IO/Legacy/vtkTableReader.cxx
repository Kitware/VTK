/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTableReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkTableReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
vtkTableReader::vtkTableReader()
{
  vtkTable *output = vtkTable::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkTableReader::~vtkTableReader()
{
}

//----------------------------------------------------------------------------
vtkTable* vtkTableReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkTable* vtkTableReader::GetOutput(int idx)
{
  return vtkTable::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkTableReader::SetOutput(vtkTable *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
// I do not think this should be here, but I do not want to remove it now.
int vtkTableReader::RequestUpdateExtent(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int piece, numPieces;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTableReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Return all data in the first piece ...
  if(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  vtkDebugMacro(<<"Reading vtk table...");

  if(!this->OpenVTKFile() || !this->ReadHeader())
  {
    return 1;
  }

  // Read table-specific stuff
  char line[256];
  if(!this->ReadString(line))
  {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
  }

  if(strncmp(this->LowerCase(line),"dataset", (unsigned long)7))
  {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseVTKFile();
    return 1;
  }

  if(!this->ReadString(line))
  {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 1;
  }

  if(strncmp(this->LowerCase(line),"table", 5))
  {
    vtkErrorMacro(<< "Cannot read dataset type: " << line);
    this->CloseVTKFile();
    return 1;
  }

  vtkTable* const output = vtkTable::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  while(true)
  {
    if(!this->ReadString(line))
    {
      break;
    }

    if(!strncmp(this->LowerCase(line), "field", 5))
    {
      vtkFieldData* const field_data = this->ReadFieldData();
      output->SetFieldData(field_data);
      field_data->Delete();
      continue;
    }

    if(!strncmp(this->LowerCase(line), "row_data", 8))
    {
      int row_count = 0;
      if(!this->Read(&row_count))
      {
        vtkErrorMacro(<<"Cannot read number of rows!");
        this->CloseVTKFile();
        return 1;
      }


      this->ReadRowData(output, row_count);
      continue;
    }

    vtkErrorMacro(<< "Unrecognized keyword: " << line);
  }

  vtkDebugMacro(<< "Read " << output->GetNumberOfRows() <<" rows in "
                << output->GetNumberOfColumns() <<" columns.\n");

  this->CloseVTKFile ();

  return 1;
}

//----------------------------------------------------------------------------
int vtkTableReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
void vtkTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
