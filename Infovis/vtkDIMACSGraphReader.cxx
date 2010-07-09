/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDIMACSGraphReader.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkDIMACSGraphReader.h"

#include "vtkExecutive.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkMutableDirectedGraph.h"
#include "vtksys/SystemTools.hxx"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkIntArray.h"

#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>
#include <vtksys/ios/iostream>
using vtksys_ios::ifstream;
using vtksys_ios::istringstream;

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

typedef enum problemTypes { GENERIC, COLORING, MAXFLOW } problemTypes;

vtkStandardNewMacro(vtkDIMACSGraphReader);

vtkDIMACSGraphReader::vtkDIMACSGraphReader()
{
  // Default values for the origin vertex
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->VertexAttributeArrayName = NULL;
  this->EdgeAttributeArrayName   = NULL;
  this->Directed = false;
  this->fileOk   = false;
  this->numVerts = 0;
  this->numEdges = 0;
}


vtkDIMACSGraphReader::~vtkDIMACSGraphReader()
{
  this->SetFileName(0);
  this->SetVertexAttributeArrayName(0);
  this->SetEdgeAttributeArrayName(0);
}


void vtkDIMACSGraphReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Vertex Attribute Array Name: "
     << (this->VertexAttributeArrayName ? this->VertexAttributeArrayName : "color")
     << endl;
  os << indent << "Edge Attribute Array Name  : "
     << (this->EdgeAttributeArrayName ? this->EdgeAttributeArrayName : "color")
     << endl;
}



// ============================================================================
// Generic DIMACS file format, which covers many 'DIMACS' style input files.
// This is the default reader if we don't have a special case file.
// * Graphs are undirected.
// * node lines (optional) have a weight value and are formatted as:
//         n id wt
//   Though, technically, some DIMACS formats (i.e., shortest paths) don't
//   specify node-attributes, we'll include them for the generic reader
//   for maximum compatibility.
// * edges, u->v, are formatted as:
//         a u v wt
//   alternatively, edges can also be:
//         e u v wt
int vtkDIMACSGraphReader::buildGenericGraph(vtkGraph     * output,
                                            vtkStdString & defaultVertexAttrArrayName,
                                            vtkStdString & defaultEdgeAttrArrayName)
{
  vtkStdString S;
  int iEdgeU, iEdgeV, iVertexID;
  int currentEdgeId = 0;

  VTK_CREATE(vtkMutableUndirectedGraph, builder);
  VTK_CREATE(vtkIntArray, ArrayVertexAttributes);
  VTK_CREATE(vtkIntArray, ArrayEdgeAttributes);

  VTK_CREATE(vtkIntArray, vertexPedigreeIds);
  VTK_CREATE(vtkIntArray, edgePedigreeIds);

  // Set up vertex attribute array for vertex-weights.
  if(this->VertexAttributeArrayName)
    {
    ArrayVertexAttributes->SetName(this->VertexAttributeArrayName);
    }
  else
    {
    ArrayVertexAttributes->SetName(defaultVertexAttrArrayName);
    }
  ArrayVertexAttributes->SetNumberOfTuples(this->numVerts);

  // Set up Edge attribute array for edge-weights.
  if(this->EdgeAttributeArrayName)
    {
    ArrayEdgeAttributes->SetName(this->EdgeAttributeArrayName);
    }
  else
    {
    ArrayEdgeAttributes->SetName(defaultEdgeAttrArrayName);
    }
  ArrayEdgeAttributes->SetNumberOfTuples(this->numEdges);

  // Set up Pedigree-IDs arrays.
  vertexPedigreeIds->SetName("vertex id");
  vertexPedigreeIds->SetNumberOfTuples(this->numVerts);
  edgePedigreeIds->SetName("edge id");
  edgePedigreeIds->SetNumberOfTuples(this->numEdges);

  // Allocate Vertices in the graph builder
  for(int i=0; i<this->numVerts; i++)
    {
    builder->AddVertex();
    vertexPedigreeIds->SetValue(i, i+1);
    }

  // set starting edge id number.
  int baseEdgeId = 1;

  ifstream IFP(this->FileName);
  if(IFP.is_open())
    {
    while( vtksys::SystemTools::GetLineFromStream(IFP,S) )
      {
      int value;
      istringstream iss(S);
      char lineType;
      iss >> lineType;
      switch(lineType)
        {
        case 'n':  /* vertex (node) definition */
            iss >> iVertexID >> value;
            ArrayVertexAttributes->SetValue(iVertexID-1, value);
            vertexPedigreeIds->SetValue(iVertexID-1, iVertexID);
            break;
        case 'a':  /* edge arc */
        case 'e':
            {
            iss >> iEdgeU >> iEdgeV >> value;
            vtkEdgeType edgeObj = builder->AddEdge(iEdgeU-1, iEdgeV-1);
            ArrayEdgeAttributes->SetValue(edgeObj.Id, value);
            edgePedigreeIds->SetValue(currentEdgeId, currentEdgeId+baseEdgeId);
            currentEdgeId++;
            }
            break;
        case 'c': /* Comment line, ignore it! */
            break;
        default:
            break;
        };
      }
    }

  // Add the pedigree ids to the graph
  builder->GetVertexData()->SetPedigreeIds( vertexPedigreeIds );
  builder->GetEdgeData()->SetPedigreeIds( edgePedigreeIds );

  // Add the attribute arrays to the graph
  builder->GetVertexData()->AddArray(ArrayVertexAttributes);
  builder->GetEdgeData()->AddArray(ArrayEdgeAttributes);

  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid graph structure");
    return 0;
    }
  return 1;
}



// ============================================================================
// Build a graph from a max-flow problem.
// * These are directed.
// * These should have TWO node descriptor lines of the format:
//         n  ID  <char>
//   where <char> is either an 's' or a 't', for the source and sink,
//   respectively.
// * Format of edge lines is:
//         a u v cap
//   to create an edge u->v, and cap gives the edge capacity.
int vtkDIMACSGraphReader::buildMaxflowGraph(vtkGraph * output)
{
  vtkStdString S;
  int iEdgeU, iEdgeV, iVertexID;
  int currentEdgeId = 0;
  int numSrcs = 0;
  int numSinks = 0;

  vtkStdString sAttribute;
  VTK_CREATE(vtkMutableDirectedGraph, builder);

  VTK_CREATE(vtkIntArray, vertexSourceArray);
  VTK_CREATE(vtkIntArray, vertexSinkArray);
  VTK_CREATE(vtkIntArray, edgeCapacityArray);

  VTK_CREATE(vtkIntArray, vertexPedigreeIds);
  VTK_CREATE(vtkIntArray, edgePedigreeIds);

  vertexSourceArray->SetName("sources");
  vertexSinkArray->SetName("sinks");
  edgeCapacityArray->SetName("capacity");

  vertexSourceArray->SetNumberOfTuples(this->numVerts);
  vertexSinkArray->SetNumberOfTuples(this->numVerts);
  edgeCapacityArray->SetNumberOfTuples(this->numEdges);

  for(int i=0; i<this->numVerts; i++)
    {
    vertexSourceArray->SetValue(i,0);
    vertexSinkArray->SetValue(i,0);
    }

  for(int i=0; i<this->numEdges; i++)
    {
    edgeCapacityArray->SetValue(i,0);
    }

  // Set up Pedigree-IDs arrays.
  vertexPedigreeIds->SetName("vertex id");
  vertexPedigreeIds->SetNumberOfTuples(this->numVerts);
  edgePedigreeIds->SetName("edge id");
  edgePedigreeIds->SetNumberOfTuples(this->numEdges);

  // Allocate Vertices in the graph builder
  for(int i=0; i<this->numVerts; i++)
    {
    builder->AddVertex();
    vertexPedigreeIds->SetValue(i, i+1);
    }

  // set starting edge id number.
  int baseEdgeId = 1;

  ifstream IFP(this->FileName);
  if(IFP.is_open())
    {
    while( vtksys::SystemTools::GetLineFromStream(IFP,S) )
      {
      istringstream iss(S);
      char lineType;
      iss >> lineType;
      switch(lineType)
        {
        case 'n':  /* vertex (node) definition */
            iss >> iVertexID >> sAttribute;
            vertexPedigreeIds->SetValue(iVertexID-1, iVertexID);

            if(sAttribute == "s" && numSrcs==0)
              {
              numSrcs++;
              vertexSourceArray->SetValue(iVertexID-1, 1);
              }
            else if(sAttribute == "t" && numSinks==0)
              {
              numSinks++;
              vertexSinkArray->SetValue(iVertexID-1, 1);
              }
            else
              {
              vtkWarningMacro(<< "In DIMACS Max-Flow file: "
                              << this->FileName
                              << "  multiple sources or sinks specified!"
                              << endl
                              << "  Ignoring all but first source/sink found.");
              }
            break;
        case 'a':  /* edge arc */
            {
            int edgeCapacity;
            iss >> iEdgeU >> iEdgeV >> edgeCapacity;
            vtkEdgeType edgeObj = builder->AddEdge(iEdgeU-1, iEdgeV-1);
            edgeCapacityArray->SetValue(edgeObj.Id, edgeCapacity);
            edgePedigreeIds->SetValue(currentEdgeId, currentEdgeId+baseEdgeId);
            currentEdgeId++;
            }
            break;
        case 'c': /* Comment line, ignore it! */
            break;
        default:
            break;
        };
      }
    }

  // Add the pedigree ids to the graph
  builder->GetVertexData()->SetPedigreeIds( vertexPedigreeIds );
  builder->GetEdgeData()->SetPedigreeIds( edgePedigreeIds );

  // Add the attribute arrays to the graph
  builder->GetVertexData()->AddArray(vertexSourceArray);
  builder->GetVertexData()->AddArray(vertexSinkArray);
  builder->GetEdgeData()->AddArray(edgeCapacityArray);


  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid graph structure");
    return 0;
    }
  return 1;
}



// ============================================================================
// Builder method for creating coloring problem graphs.
// * These are undirected.
// * nodes and edges have no weights associated with them.
// * edges (u->v) are formatted as:
//         e u v
int vtkDIMACSGraphReader::buildColoringGraph(vtkGraph * output)
{
  vtkStdString S;
  int iEdgeU, iEdgeV;
  int currentEdgeId = 0;

  VTK_CREATE(vtkMutableUndirectedGraph, builder);
  VTK_CREATE(vtkIntArray, vertexPedigreeIds);
  VTK_CREATE(vtkIntArray, edgePedigreeIds);

  // Set up Pedigree-IDs arrays.
  vertexPedigreeIds->SetName("vertex id");
  vertexPedigreeIds->SetNumberOfTuples(this->numVerts);
  edgePedigreeIds->SetName("edge id");
  edgePedigreeIds->SetNumberOfTuples(this->numEdges);

  // Allocate Vertices in the graph builder
  for(int i=0; i<this->numVerts; i++)
    {
    builder->AddVertex();
    vertexPedigreeIds->SetValue(i, i+1);
    }

  // set starting edge id number.
  int baseEdgeId = 1;

  ifstream IFP(this->FileName);
  if(IFP.is_open())
    {
    while( vtksys::SystemTools::GetLineFromStream(IFP,S) )
      {
      istringstream iss(S);
      char lineType;
      iss >> lineType;
      switch(lineType)
        {
        case 'e':  /* edge arc */
            {
            iss >> iEdgeU >> iEdgeV;
            vtkEdgeType edgeObj = builder->AddEdge(iEdgeU-1, iEdgeV-1);
            edgePedigreeIds->SetValue(currentEdgeId, currentEdgeId+baseEdgeId);
            currentEdgeId++;
            }
            break;
        default:
            break;
        };
      }
    }

  // Add the pedigree ids to the graph
  builder->GetVertexData()->SetPedigreeIds( vertexPedigreeIds );
  builder->GetEdgeData()->SetPedigreeIds( edgePedigreeIds );

  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid graph structure");
    return 0;
    }

  return 1;
}



// ============================================================================
// Searches for the problem line in a dimacs graph (starts with 'p')
// and gets the problem type and the number of vertices and edges.
// Sets the directedness of the graph as well based on what the problem
// definition is (i.e, max-flow problems are directed, but coloring is not).
int vtkDIMACSGraphReader::ReadGraphMetaData()
{
  if (this->FileName == NULL)
    {
    vtkErrorMacro("File name undefined");
    return 0;
    }

  ifstream IFP(this->FileName);
  if(!IFP.is_open())
    {
    vtkErrorMacro("Could not open file " << this->FileName << ".");
    return(0);
    }
  vtkStdString S;
  bool foundProblemLine = false;
  bool foundMultipleProblemLines = false;

  // Look at lines until we find the problem line (this should always
  // be one of the first lines in a DIMACS graph)
  while(!foundProblemLine && vtksys::SystemTools::GetLineFromStream(IFP,S))
    {
    istringstream iss(S);
    char lineType;
    iss >> lineType;
    switch(lineType)
      {
      case 'p':
        if( !foundProblemLine )
          {
          foundProblemLine = true;
          iss >> this->dimacsProblemStr
              >> this->numVerts
              >> this->numEdges;
          }
        else
          {
          foundMultipleProblemLines = true;
          }
      };
    }
  IFP.close();

  if(!foundProblemLine)
    {
    vtkErrorMacro(<< "Error in DIMACS file: " << this->FileName
                  << ", could not find a problem description line.");
    return 0;
    }

  if(foundMultipleProblemLines)
    {
    vtkWarningMacro(<< "Found multiple problem lines in DIMACS file: "
                    << this->FileName << "; using the first one found.");
    }

  // Set directed if necessary
  if(this->dimacsProblemStr == "max")
    {
    this->Directed = true;
    }

  this->fileOk = true;

  return 1;
}



// ============================================================================
int vtkDIMACSGraphReader::RequestData(vtkInformation       *  vtkNotUsed(request),
                                      vtkInformationVector ** vtkNotUsed(inputVector),
                                      vtkInformationVector *  outputVector)
{
  int rval=0;
  if( !this->fileOk )
    {
    return 0;
    }

  vtkGraph * output = vtkGraph::GetData(outputVector);

  if(this->dimacsProblemStr == "edge")
    {
    vtkDebugMacro("Loading DIMACS coloring problem graph.");
    rval = buildColoringGraph(output);
    }
  else if(this->dimacsProblemStr == "max")
    {
    vtkDebugMacro("Loading DIMACS max-flow problem graph.");
    rval = buildMaxflowGraph(output);
    }
  else
    {
    vtkDebugMacro("Loading DIMACS default graph.");
    vtkStdString vWeightName = "weight";
    vtkStdString eWeightName = "weight";
    rval = buildGenericGraph(output, vWeightName, eWeightName);
    }
  return rval;
}



//----------------------------------------------------------------------------
int vtkDIMACSGraphReader::RequestDataObject(vtkInformation*,
                                            vtkInformationVector**,
                                            vtkInformationVector* )
{
  ReadGraphMetaData();

  vtkDataObject *current = this->GetExecutive()->GetOutputData(0);
  if (!current || (this->Directed && !vtkDirectedGraph::SafeDownCast(current))
               || (!this->Directed && vtkDirectedGraph::SafeDownCast(current)))
    {
    vtkGraph *output = 0;
    if (this->Directed)
      {
      output = vtkDirectedGraph::New();
      }
    else
      {
      output = vtkUndirectedGraph::New();
      }
    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }
  return 1;
}
