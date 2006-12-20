/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphReader.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkGraphReader, "1.1");
vtkStandardNewMacro(vtkGraphReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
vtkGraphReader::vtkGraphReader()
{
  vtkGraph *output = vtkGraph::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkGraphReader::~vtkGraphReader()
{
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraphReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkGraph* vtkGraphReader::GetOutput(int idx)
{
  return vtkGraph::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void vtkGraphReader::SetOutput(vtkGraph *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
// I do not think this should be here, but I do not want to remove it now.
int vtkGraphReader::RequestUpdateExtent(
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
int vtkGraphReader::RequestData(
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

  vtkDebugMacro(<<"Reading vtk graph ...");

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

  if(strncmp(this->LowerCase(line),"graph", 5))
    {
    vtkErrorMacro(<< "Cannot read dataset type: " << line);
    this->CloseVTKFile();
    return 1;
    }

  vtkGraph* const output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if(!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile();
    return 1;
    }

  if(!strncmp(this->LowerCase(line),"directed", 8))
    {
    output->SetDirected(true);
    }
  else if(!strncmp(this->LowerCase(line), "undirected", 10))
    {
    output->SetDirected(false);
    }
  else
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseVTKFile();
    return 1;
    }

  int done = 0;
  while(!done)
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
      
    if(!strncmp(this->LowerCase(line), "points", 6))
      {
      int point_count = 0;
      if(!this->Read(&point_count))
        {
        vtkErrorMacro(<<"Cannot read number of points!");
        this->CloseVTKFile ();
        return 1;
        }

      this->ReadPoints(output, point_count);
      continue;
      }
      
    if(!strncmp(this->LowerCase(line), "arcs", 4))
      {
      int arc_count = 0;
      if(!this->Read(&arc_count))
        {
        vtkErrorMacro(<<"Cannot read number of arcs!");
        this->CloseVTKFile ();
        return 1;
        }
      int source = 0;
      int target = 0;
      for(int arc = 0; arc != arc_count; ++arc)
        {
        if(!(this->Read(&source) && this->Read(&target)))
          {
          vtkErrorMacro(<<"Cannot read arc!");
          this->CloseVTKFile();
          return 1;
          }
        
        output->AddArc(source, target);
        }
      continue;
      }

    if(!strncmp(this->LowerCase(line), "point_data", 10))
      {
      int point_count = 0;
      if(!this->Read(&point_count))
        {
        vtkErrorMacro(<<"Cannot read number of points!");
        this->CloseVTKFile ();
        return 1;
        }

      this->ReadPointData(output, point_count);
      continue;
      }
      
    if(!strncmp(this->LowerCase(line), "cell_data", 9))
      {
      int cell_count = 0;
      if(!this->Read(&cell_count))
        {
        vtkErrorMacro(<<"Cannot read number of points!");
        this->CloseVTKFile ();
        return 1;
        }

      this->ReadCellData(output, cell_count);
      continue;
      }
      
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }

  vtkDebugMacro(<< "Read " << output->GetNumberOfNodes() <<" nodes and "
                << output->GetNumberOfArcs() <<" arcs.\n");

  this->CloseVTKFile ();

  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
void vtkGraphReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
