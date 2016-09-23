/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKCoreDecomposition.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkKCoreDecomposition.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkEdgeListIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkOutEdgeIterator.h"
#include "vtkUndirectedGraph.h"
#include "vtkDirectedGraph.h"
#include "vtkType.h"
#include <vtksys/hash_map.hxx>

vtkStandardNewMacro(vtkKCoreDecomposition);

namespace
{

// The Neighbors class defines a graph edge iterator
// that allows us to iterate over just the in edges,
// just the out edges, or both the in and out edges.
class Neighbors
{
public:
  Neighbors(bool _UseInDegreeNeighbors,
            bool _UseOutDegreeNeighbors)
  {
    this->iti = vtkInEdgeIterator::New();
    this->ito = vtkOutEdgeIterator::New();
    this->UseInDegreeNeighbors = _UseInDegreeNeighbors;
    this->UseOutDegreeNeighbors = _UseOutDegreeNeighbors;
  }

  ~Neighbors()
  {
    this->iti->Delete();
    this->ito->Delete();
  }

  void Initialize(vtkGraph* g,
                  int v)
  {
    if(vtkUndirectedGraph::SafeDownCast(g))
    {
      this->Undirected = true;
    }
    else
    {
      this->Undirected = false;
    }

    this->iti->Initialize(g, vtkIdType(v - 1));

    if(!this->Undirected)
    {
      this->ito->Initialize(g, vtkIdType(v - 1));
    }
  }

  bool HasNext()
  {
    if(this->Undirected)
    {
      return(this->iti->HasNext());
    }

    if(this->UseInDegreeNeighbors &&
      !this->UseOutDegreeNeighbors)
    {
      return(this->iti->HasNext());
    }

    if(!this->UseInDegreeNeighbors &&
        this->UseOutDegreeNeighbors)
    {
      return(this->ito->HasNext());
    }

    return(this->iti->HasNext() || this->ito->HasNext());
  }

  int Next()
  {
    if(this->Undirected)
    {
      return(int(this->iti->Next().Source) + 1);
    }

    if(this->UseInDegreeNeighbors &&
      !this->UseOutDegreeNeighbors)
    {
      return(int(this->iti->Next().Source) + 1);
    }

    if(!this->UseInDegreeNeighbors &&
       this->UseOutDegreeNeighbors)
    {
      return(int(this->ito->Next().Target) + 1);
    }

    if(this->iti->HasNext())
    {
      return(int(this->iti->Next().Source) + 1);
    }

    return(int(this->ito->Next().Target) + 1);
  }

private:
  vtkInEdgeIterator* iti;
  vtkOutEdgeIterator* ito;
  bool UseInDegreeNeighbors;
  bool UseOutDegreeNeighbors;
  bool Undirected;
};

// Array that is indexed starting from 1.
class tableVert {
public:
  tableVert(int n)
  {
    this->_array = vtkIntArray::New();
    this->_array->SetNumberOfTuples(n);
  }

  tableVert(vtkIntArray* a)
  {
    this->_array = a;
    this->_array->Register(this->_array);
  }

  ~tableVert()
  {
    if(this->_array)
    {
      this->_array->Delete();
      this->_array = 0;
    }
  }

  int& operator[]( int idx )
  {
    if(idx < 1 || idx > this->_array->GetNumberOfTuples())
    {
      cerr << "Write Number of tuples = " << this->_array->GetNumberOfTuples() << endl;
      cerr << "Array index out out bounds in tableVert operator [], index: " << idx << endl;
      return(static_cast<int*>(this->_array->GetVoidPointer(0))[0]);
    }

    return(static_cast<int*>(this->_array->GetVoidPointer(0))[idx - 1]);
  }

private:
  vtkIntArray* _array;
};

// Array that is indexed starting from 0.
class tableDeg {
public:
  tableDeg()
  {
    this->_array = 0;
  }

  ~tableDeg()
  {
    if(this->_array)
    {
      this->_array->Delete();
      this->_array = 0;
    }
  }

  int getArraySize()
  {
    if(this->_array)
    {
      return(this->_array->GetNumberOfTuples());
    }
    return(0);
  }

  void setNewArray(int n)
  {
    if(this->_array)
    {
      this->_array->Delete();
    }
    this->_array = vtkIntArray::New();
    this->_array->SetNumberOfTuples(n);
  }

  int& operator[]( int idx )
  {
    if(idx < 0 || idx >= this->_array->GetNumberOfTuples())
    {
      cerr << "Read Number of tuples = " << this->_array->GetNumberOfTuples() << endl;
      cerr << "Array index out out bounds in tableDeg operator [], index: " << idx << endl;
      return(static_cast<int*>(this->_array->GetVoidPointer(0))[0]);
    }

    return(static_cast<int*>(this->_array->GetVoidPointer(0))[idx]);
  }

private:
  vtkIntArray* _array;
};
};

vtkKCoreDecomposition::vtkKCoreDecomposition()
{
  this->OutputArrayName = 0;
  this->UseInDegreeNeighbors = true;
  this->UseOutDegreeNeighbors = true;
  this->CheckInputGraph = true;
}

vtkKCoreDecomposition::~vtkKCoreDecomposition()
{
  this->SetOutputArrayName(0);
}

// This is the O(edges) k-cores algorithm implementation
// that looks exactly like the code listing given in the
// reference paper, "An O(m) Algorithm for Cores Decomposition
// of Networks."
void vtkKCoreDecomposition::Cores(vtkGraph* g,
                                  vtkIntArray* KCoreNumbers)
{
  int n, md, start, num;
  int w, pu, pw;
  int v, d, i, u, du;

  tableVert deg(KCoreNumbers);
  tableVert pos(g->GetNumberOfVertices());
  tableVert vert(g->GetNumberOfVertices());
  tableDeg bin;

  if(vtkDirectedGraph::SafeDownCast(g) &&
     this->UseInDegreeNeighbors &&
     this->UseOutDegreeNeighbors)
  {
    bin.setNewArray(2*g->GetNumberOfVertices() - 1);
  }
  else
  {
    bin.setNewArray(g->GetNumberOfVertices());
  }

  n = g->GetNumberOfVertices();
  md = 0;
  Neighbors neighborVertices(this->UseInDegreeNeighbors,
                             this->UseOutDegreeNeighbors);

  for(v = 1; v <= n; v++)
  {
    d = 0;
    neighborVertices.Initialize(g, v);
    while(neighborVertices.HasNext())
    {
      d++;
      neighborVertices.Next();
    }
    deg[v] = d;
    if(d > md)
    {
      md = d;
    }
  }

  if(md > bin.getArraySize())
  {
    vtkErrorMacro("Maximum vertex degree exceeds bin array size: " << md << ". Unable to compute K core.");
    return;
  }

  for(d = 0; d <= md; d++)
  {
    bin[d] = 0;
  }

  for(v = 1; v <= n; v++)
  {
    bin[deg[v]] += 1;
  }

  start = 1;
  for(d = 0; d <= md; d++)
  {
    num = bin[d];
    bin[d] = start;
    start = start + num;
  }

  for(v = 1; v <= n; v++)
  {
    pos[v] = bin[deg[v]];
    vert[pos[v]] = v;
    bin[deg[v]] += 1;
  }

  for(d = md; d >= 1; d--)
  {
    bin[d] = bin[d - 1];
  }

  bin[0] = 1;

  for(i = 1; i <= n; i++)
  {
    v = vert[i];
    neighborVertices.Initialize(g, v);
    while(neighborVertices.HasNext())
    {
      u = neighborVertices.Next();
      if(deg[u] > deg[v])
      {
        du = deg[u];
        pu = pos[u];
        pw = bin[du];
        w = vert[pw];
        if(u != w)
        {
          pos[u] = pw;
          pos[w] = pu;
          vert[pu] = w;
          vert[pw] = u;
        }
        bin[du] += 1;
        deg[u] -= 1;
      }
    }
  }
}

int vtkKCoreDecomposition::RequestData(vtkInformation *vtkNotUsed(request),
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);

  if(this->CheckInputGraph)
  {
    // Check input graph for parallel edges.  The input graph must not contain
    // parallel edges for the K core algorithm to work.  We loop over the edges
    // and use a Cantor pairing function to create a unique integer from each
    // <source, target> pair.  This unique integer is used as a key in a hash map
    // to keep track of all of the unique edges we have seen so far.
    vtkEdgeListIterator* it = vtkEdgeListIterator::New();
    vtksys::hash_map<unsigned long int, bool> hmap;
    input->GetEdges(it);
    bool foundParallelEdges = false;
    bool foundLoops = false;
    while(it->HasNext())
    {
      vtkEdgeType e = it->Next();
      // Cantor pairing function
      unsigned long int id = (unsigned long int) (0.5*(e.Source + e.Target)*(e.Source + e.Target + 1.0) + e.Target);
      if(hmap.find(id) == hmap.end())
      {
        hmap[id] = true;
      }
      else
      {
        vtkErrorMacro("Found parallel edge between at vertex ID: " << e.Source << " and vertex ID: " << e.Target);
        foundParallelEdges = true;
      }

      if(vtkUndirectedGraph::SafeDownCast(input))
      {
        id = (unsigned long int) (0.5*(e.Target + e.Source)*(e.Target + e.Source + 1.0) + e.Source);
        if(hmap.find(id) == hmap.end())
        {
          hmap[id] = true;
        }
        else
        {
          vtkErrorMacro("Found parallel edge between at vertex ID: " << e.Source << " and vertex ID: " << e.Target);
          foundParallelEdges = true;
        }
      }

      // Check input graph for loops, i.e. edges that start and end
      // on the same vertex.  The K core is not defined for these graphs.
      // Just loop over the edges and check for equal target and source.
      if(e.Source == e.Target)
      {
        foundLoops = true;
        vtkErrorMacro("Found loop at vertex ID: " << e.Source);
      }
    }

    it->Delete();
    hmap.clear();

    if(foundLoops || foundParallelEdges)
    {
      if(foundLoops)
      {
        vtkErrorMacro("Found loops in input graph.  Unable to compute K core.");
      }

      if(foundParallelEdges)
      {
        vtkErrorMacro("Found parallel edges in input graph.  Unable to compute K core.");
      }
      return 0;
    }
  }

  // Create the attribute array
  vtkIntArray* KCoreNumbers = vtkIntArray::New();
  if (this->OutputArrayName)
  {
    KCoreNumbers->SetName(this->OutputArrayName);
  }
  else
  {
    KCoreNumbers->SetName("KCoreDecompositionNumbers");
  }

  KCoreNumbers->SetNumberOfTuples(input->GetNumberOfVertices());

  // Call the K core algorithm implementation to find the k core
  // decomposition for the input graph.
  this->Cores(input,
              KCoreNumbers);

  // Add attribute array to the output
  output->GetVertexData()->AddArray(KCoreNumbers);
  KCoreNumbers->Delete();

  return 1;
}

void vtkKCoreDecomposition::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputArrayName: "
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
  os << indent << "UseInDegreeNeighbors: "
     << (this->UseInDegreeNeighbors ? "on" : "off") << endl;
  os << indent << "UseOutDegreeNeighbors: "
     << (this->UseOutDegreeNeighbors ? "on" : "off") << endl;
  os << indent << "CheckInputGraph: "
     << (this->CheckInputGraph ? "on" : "off") << endl;
}
