/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkChacoGraphReader.cxx

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

#include "vtkChacoGraphReader.h"

#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"

#include <vtksys/ios/fstream>
#include <vtksys/ios/sstream>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/vtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(vtksys_ios::istream& stream, vtkStdString &output, char delim='\n');

vtkCxxRevisionMacro(vtkChacoGraphReader, "1.2");
vtkStandardNewMacro(vtkChacoGraphReader);

vtkChacoGraphReader::vtkChacoGraphReader()
{
  // Default values for the origin vertex
  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
}

vtkChacoGraphReader::~vtkChacoGraphReader()
{
  this->SetFileName(0);
}

void vtkChacoGraphReader::PrintSelf(vtksys_ios::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FileName: " 
     << (this->FileName ? this->FileName : "(none)") << endl;
}

int vtkChacoGraphReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (this->FileName == NULL)
    {
    vtkErrorMacro("File name undefined");
    return 0;
    }

  vtksys_ios::ifstream fin(this->FileName);
  if(!fin.is_open())
    {
    vtkErrorMacro("Could not open file " << this->FileName << ".");
    return 0;
    }

  // Create a mutable graph builder
  VTK_CREATE(vtkMutableUndirectedGraph, builder);

  // Get the header line
  vtkStdString line;
  my_getline(fin, line);
  vtksys_ios::stringstream firstLine;
  firstLine << line;
  vtkIdType numVerts;
  vtkIdType numEdges;
  firstLine >> numVerts >> numEdges;
  vtkIdType type = 0;
  if (firstLine.good())
    {
    firstLine >> type;
    }

  // Create the weight arrays
  int vertWeights = type % 10;
  int edgeWeights = (type / 10) % 10;
  //cerr << "type=" << type << ",vertWeights=" << vertWeights << ",edgeWeights=" << edgeWeights << endl;
  vtkIntArray** vertArr = new vtkIntArray*[vertWeights];
  for (int vw = 0; vw < vertWeights; vw++)
    {
    vtksys_ios::ostringstream oss;
    oss << "weight " << (vw+1);
    vertArr[vw] = vtkIntArray::New();
    vertArr[vw]->SetName(oss.str().c_str());
    builder->GetVertexData()->AddArray(vertArr[vw]);
    vertArr[vw]->Delete();
    }
  vtkIntArray** edgeArr = new vtkIntArray*[edgeWeights];
  for (int ew = 0; ew < edgeWeights; ew++)
    {
    vtksys_ios::ostringstream oss;
    oss << "weight " << (ew+1);
    edgeArr[ew] = vtkIntArray::New();
    edgeArr[ew]->SetName(oss.str().c_str());
    builder->GetEdgeData()->AddArray(edgeArr[ew]);
    edgeArr[ew]->Delete();
    }

  // Add the vertices
  for (vtkIdType v = 0; v < numVerts; v++)
    {
    builder->AddVertex();
    }

  // Add the edges
  for (vtkIdType u = 0; u < numVerts; u++)
    {
    my_getline(fin, line);
    vtksys_ios::stringstream stream;
    stream << line;
    //cerr << "read line " << stream.str() << endl;
    int weight;
    for (int vw = 0; vw < vertWeights; vw++)
      {
      stream >> weight;
      vertArr[vw]->InsertNextValue(weight);
      }
    vtkIdType v;
    while (stream.good())
      {
      stream >> v;
      //cerr << "read adjacent vertex " << v << endl;

      // vtkGraph ids are 1 less than Chaco graph ids
      v--;
      // Only add the edge if v less than u.
      // This avoids adding the same edge twice.
      if (v < u)
        {
        builder->AddEdge(u, v);
        for (int ew = 0; ew < edgeWeights; ew++)
          {
          stream >> weight;
          edgeArr[ew]->InsertNextValue(weight);
          }
        }
      }
    }

  // Clean up
  fin.close();

  // Get the output graph
  vtkGraph* output = vtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorMacro(<<"Invalid graph structure");
    return 0;
    }

  return 1;
}

static int
my_getline(vtksys_ios::istream& in, vtkStdString &out, char delimiter)
{
  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;
  
  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
    {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
      {
      out += downcast;
      }
    else
      {
      return numCharactersRead;
      }
    }

  return numCharactersRead;
}


