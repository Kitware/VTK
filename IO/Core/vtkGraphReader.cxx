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
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkGraphReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
vtkGraphReader::vtkGraphReader()
{
  // We don't know the output type yet.
  // It could be vtkDirectedGraph or vtkUndirectedGraph.
  // We will set it in RequestInformation().

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
  char line[256];

  bool directed = true;
  if (!this->ReadGraphDirectedness(directed))
    {
    this->CloseVTKFile();
    return 1;
    }

  vtkSmartPointer<vtkMutableDirectedGraph> dir_builder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undir_builder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
  vtkGraph *builder = 0;
  if (directed)
    {
    builder = dir_builder;
    }
  else
    {
    builder = undir_builder;
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
      if (directed)
        {
        dir_builder->SetFieldData(field_data);
        }
      else
        {
        undir_builder->SetFieldData(field_data);
        }
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

      this->ReadPoints(builder, point_count);
      continue;
      }

    if(!strncmp(this->LowerCase(line), "vertices", 8))
      {
      int vertex_count = 0;
      if(!this->Read(&vertex_count))
        {
        vtkErrorMacro(<<"Cannot read number of vertices!");
        this->CloseVTKFile ();
        return 1;
        }
      for (vtkIdType v = 0; v < vertex_count; ++v)
        {
        if (directed)
          {
          dir_builder->AddVertex();
          }
        else
          {
          undir_builder->AddVertex();
          }
        }
      continue;
      }

    if(!strncmp(this->LowerCase(line), "edges", 5))
      {
      int edge_count = 0;
      if(!this->Read(&edge_count))
        {
        vtkErrorMacro(<<"Cannot read number of edges!");
        this->CloseVTKFile ();
        return 1;
        }
      int source = 0;
      int target = 0;
      for(int edge = 0; edge != edge_count; ++edge)
        {
        if(!(this->Read(&source) && this->Read(&target)))
          {
          vtkErrorMacro(<<"Cannot read edge!");
          this->CloseVTKFile();
          return 1;
          }

        if (directed)
          {
          dir_builder->AddEdge(source, target);
          }
        else
          {
          undir_builder->AddEdge(source, target);
          }
        }
      continue;
      }

    if(!strncmp(this->LowerCase(line), "vertex_data", 10))
      {
      int vertex_count = 0;
      if(!this->Read(&vertex_count))
        {
        vtkErrorMacro(<<"Cannot read number of vertices!");
        this->CloseVTKFile();
        return 1;
        }


      this->ReadVertexData(builder, vertex_count);
      continue;
      }

    if(!strncmp(this->LowerCase(line), "edge_data", 9))
      {
      int edge_count = 0;
      if(!this->Read(&edge_count))
        {
        vtkErrorMacro(<<"Cannot read number of edges!");
        this->CloseVTKFile();
        return 1;
        }

      this->ReadEdgeData(builder, edge_count);
      continue;
      }

    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    }

  vtkDebugMacro(<< "Read "
    << builder->GetNumberOfVertices()
    << " vertices and "
    << builder->GetNumberOfEdges()
    << " edges.\n");

  this->CloseVTKFile ();

  // Copy builder into output.
  vtkGraph* const output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  bool valid = true;
  if (directed)
    {
    valid = output->CheckedShallowCopy(dir_builder);
    }
  else
    {
    valid = output->CheckedShallowCopy(undir_builder);
    }

  if (!valid)
    {
    vtkErrorMacro(<<"Invalid graph structure, returning empty graph.");
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::ReadGraphDirectedness(bool & directed)
  {
  if(!this->OpenVTKFile() || !this->ReadHeader())
    {
    return 0;
    }

  // Read graph-specific stuff
  char line[256];
  if(!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile();
    return 0;
    }

  if(strncmp(this->LowerCase(line),"dataset", (unsigned long)7))
    {
    vtkErrorMacro(<< "Unrecognized keyword: " << line);
    this->CloseVTKFile();
    return 0;
    }

  if(!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    return 0;
    }

  if(!strncmp(this->LowerCase(line),"directed_graph", 14))
    {
    directed = true;
    }
  else if(!strncmp(this->LowerCase(line), "undirected_graph", 16))
    {
    directed = false;
    }
  else
    {
    vtkErrorMacro(<< "Cannot read type: " << line);
    this->CloseVTKFile();
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::RequestDataObject(vtkInformation *,
                                      vtkInformationVector **,
                                      vtkInformationVector *)
{
  bool directed = true;
  if (!this->ReadGraphDirectedness(directed))
    {
    this->CloseVTKFile();
    return 1;
    }
  this->CloseVTKFile();

  vtkGraph *output = 0;
  if (directed)
    {
    output = vtkDirectedGraph::New();
    }
  else
    {
    output = vtkUndirectedGraph::New();
    }
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::ProcessRequest(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkGraphReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
