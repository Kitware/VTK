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
#include "vtkMolecule.h"
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
  // It could be vtkDirectedGraph, vtkUndirectedGraph, or vtkMolecule.
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

  GraphType graphType;
  if (!this->ReadGraphType(graphType))
  {
    this->CloseVTKFile();
    return 1;
  }

  vtkSmartPointer<vtkMutableDirectedGraph> dir_builder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undir_builder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  vtkGraph *builder = 0;
  switch (graphType)
  {
    case vtkGraphReader::DirectedGraph:
      builder = dir_builder;
      break;

    case vtkGraphReader::UndirectedGraph:
    case vtkGraphReader::Molecule: // Extends undirected graph.
      builder = undir_builder;
      break;

    default:
      vtkErrorMacro("ReadGraphType gave invalid result.");
      this->CloseVTKFile();
      return 1;
  }

  // Lattice information for molecules:
  bool hasLattice = false;
  vtkVector3d lattice_a;
  vtkVector3d lattice_b;
  vtkVector3d lattice_c;
  vtkVector3d lattice_origin;

  while(true)
  {
    if(!this->ReadString(line))
    {
      break;
    }

    if(!strncmp(this->LowerCase(line), "field", 5))
    {
      vtkFieldData* const field_data = this->ReadFieldData();
      switch (graphType)
      {
        case vtkGraphReader::DirectedGraph:
          dir_builder->SetFieldData(field_data);
          break;

        case vtkGraphReader::UndirectedGraph:
        case vtkGraphReader::Molecule:
          undir_builder->SetFieldData(field_data);
          break;

        default: // Can't happen, would return earlier.
          break;
      }

      field_data->Delete();
      continue;
    }

    if(!strncmp(this->LowerCase(line), "points", 6))
    {
      vtkIdType point_count = 0;
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
      vtkIdType vertex_count = 0;
      if(!this->Read(&vertex_count))
      {
        vtkErrorMacro(<<"Cannot read number of vertices!");
        this->CloseVTKFile ();
        return 1;
      }
      for (vtkIdType v = 0; v < vertex_count; ++v)
      {
        switch (graphType)
        {
          case vtkGraphReader::DirectedGraph:
            dir_builder->AddVertex();
            break;

          case vtkGraphReader::UndirectedGraph:
          case vtkGraphReader::Molecule:
            undir_builder->AddVertex();
            break;

          default: // Can't happen, would return earlier.
            break;
        }
      }
      continue;
    }

    if(!strncmp(this->LowerCase(line), "edges", 5))
    {
      vtkIdType edge_count = 0;
      if(!this->Read(&edge_count))
      {
        vtkErrorMacro(<<"Cannot read number of edges!");
        this->CloseVTKFile ();
        return 1;
      }
      vtkIdType source = 0;
      vtkIdType target = 0;
      for(vtkIdType edge = 0; edge != edge_count; ++edge)
      {
        if(!(this->Read(&source) && this->Read(&target)))
        {
          vtkErrorMacro(<<"Cannot read edge!");
          this->CloseVTKFile();
          return 1;
        }

        switch (graphType)
        {
          case vtkGraphReader::DirectedGraph:
            dir_builder->AddEdge(source, target);
            break;

          case vtkGraphReader::UndirectedGraph:
          case vtkGraphReader::Molecule:
            undir_builder->AddEdge(source, target);
            break;

          default: // Can't happen, would return earlier.
            break;
        }
      }
      continue;
    }

    if(!strncmp(this->LowerCase(line), "vertex_data", 10))
    {
      vtkIdType vertex_count = 0;
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
      vtkIdType edge_count = 0;
      if(!this->Read(&edge_count))
      {
        vtkErrorMacro(<<"Cannot read number of edges!");
        this->CloseVTKFile();
        return 1;
      }

      this->ReadEdgeData(builder, edge_count);
      continue;
    }

    if (!strncmp(this->LowerCase(line), "lattice_", 8))
    {
      switch (line[8]) // lattice_<line[8]> -- which vector: a, b, c, or origin?
      {
        case 'a':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_a[i]))
            {
              vtkErrorMacro("Error while parsing lattice information.");
              this->CloseVTKFile();
              return 1;
            }
          }
          continue;

        case 'b':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_b[i]))
            {
              vtkErrorMacro("Error while parsing lattice information.");
              this->CloseVTKFile();
              return 1;
            }
          }
          continue;

        case 'c':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_c[i]))
            {
              vtkErrorMacro("Error while parsing lattice information.");
              this->CloseVTKFile();
              return 1;
            }
          }
          continue;

        case 'o':
          hasLattice = true;
          for (int i = 0; i < 3; ++i)
          {
            if (!this->Read(&lattice_origin[i]))
            {
              vtkErrorMacro("Error while parsing lattice information.");
              this->CloseVTKFile();
              return 1;
            }
          }
          continue;

        default:
          break;
      }

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

  bool valid = output->CheckedShallowCopy(builder);

  vtkMolecule *mol = vtkMolecule::SafeDownCast(output);
  if (valid && hasLattice && mol)
  {
    mol->SetLattice(lattice_a, lattice_b, lattice_c);
    mol->SetLatticeOrigin(lattice_origin);
  }

  if (!valid)
  {
    vtkErrorMacro(<<"Invalid graph structure, returning empty graph.");
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGraphReader::ReadGraphType(GraphType &type)
{
  type = UnknownGraph;

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
    type = DirectedGraph;
  }
  else if(!strncmp(this->LowerCase(line), "undirected_graph", 16))
  {
    type = UndirectedGraph;
  }
  else if (!strncmp(this->LowerCase(line), "molecule", 8))
  {
    type = Molecule;
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
  GraphType graphType;
  if (!this->ReadGraphType(graphType))
  {
    this->CloseVTKFile();
    return 1;
  }
  this->CloseVTKFile();

  vtkGraph *output = 0;
  switch (graphType)
  {
    case vtkGraphReader::DirectedGraph:
      output = vtkDirectedGraph::New();
      break;

    case vtkGraphReader::UndirectedGraph:
      output = vtkUndirectedGraph::New();
      break;

    case vtkGraphReader::Molecule:
      output = vtkMolecule::New();
      break;

    default:
      vtkErrorMacro("ReadGraphType returned invalid result.");
      return 1;
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
