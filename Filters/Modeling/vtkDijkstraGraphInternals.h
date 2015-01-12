/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraGraphInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDijkstraGraphInternals - Helper class due to PIMPL excess
// .SECTION Description
// .SECTION See Also
//  vtkDijkstraGraphGeodesicPath
// .SECTION Warning
//  Do not include this file in a header file, it will break PIMPL convention

#ifndef vtkDijkstraGraphInternals_h
#define vtkDijkstraGraphInternals_h

#include <vector>
#include <map>

//-----------------------------------------------------------------------------
class vtkDijkstraGraphInternals
{
public:

  vtkDijkstraGraphInternals()
    {
      this->HeapSize = 0;
    }

  ~vtkDijkstraGraphInternals()
    {
    }

  // CumulativeWeights(v) current summed weight for path to vertex v.
  std::vector<double> CumulativeWeights;

  // Predecessors(v) predecessor of v.
  std::vector<int> Predecessors;

  // OpenVertices is the set of vertices which has not a shortest path yet but has a path.
  // OpenVertices(v) == 1 means that vertex v is in OpenVertices.
  // OpenVertices is a boolean (1/0) array.
  std::vector<unsigned char> OpenVertices;

  // ClosedVertices is the set of vertices with already determined shortest path
  // ClosedVertices(v) == 1 means that vertex v is in ClosedVertices.
  // ClosedVertices is a boolean (1/0) array.
  std::vector<unsigned char> ClosedVertices;

  // Adjacency representation.
  std::vector< std::map< int,double > > Adjacency;

  // Path repelling by assigning high costs to flagged vertices.
  std::vector<unsigned char> BlockedVertices;


  void Heapify(const int& i)
  {
    // left node
    unsigned int l = i * 2;
    // right node
    unsigned int r = i * 2 + 1;
    int smallest = -1;

    // The value of element v is CumulativeWeights(v)
    // the heap stores the vertex numbers
    if ( l <= this->HeapSize &&
        ( this->CumulativeWeights[ this->Heap[l] ] <
          this->CumulativeWeights[ this->Heap[i] ] ) )
      {
      smallest = l;
      }
    else
      {
      smallest = i;
      }

    if ( r <= this->HeapSize &&
        ( this->CumulativeWeights[ this->Heap[ r ] ] <
          this->CumulativeWeights[ this->Heap[ smallest ] ] ) )
      {
      smallest = r;
      }

    if ( smallest != i )
      {
      int t = this->Heap[i];

      this->Heap[ i ] = this->Heap[ smallest ];

      // where is Heap(i)
      this->HeapIndices[ this->Heap[i] ] = i;

      // Heap and HeapIndices are kinda inverses
      this->Heap[ smallest ] = t;
      this->HeapIndices[ t ] = smallest;

      this->Heapify( smallest );
      }
  }

  void HeapInsert(const int& v)
  {
    if ( this->HeapSize >= (this->Heap.size() - 1) )
      {
      return;
      }

    this->HeapSize++;
    int i = this->HeapSize;

    while ( i > 1 &&
            this->CumulativeWeights[ this->Heap[i/2] ] >
            this->CumulativeWeights[v] )
      {
      this->Heap[ i ] = this->Heap[i/2];
      this->HeapIndices[ this->Heap[i] ] = i;
      i /= 2;
      }
     // Heap and HeapIndices are kinda inverses
    this->Heap[ i ] = v;
    this->HeapIndices[ v ] = i;
  }

  int HeapExtractMin()
  {
    if ( this->HeapSize == 0 )
      {
      return -1;
      }

    int minv = this->Heap[ 1 ];
    this->HeapIndices[ minv ] = -1;

    this->Heap[ 1 ] = this->Heap[ this->HeapSize ];
    this->HeapIndices[ this->Heap[1] ]= 1;

    this->HeapSize--;
    this->Heapify( 1 );

    return minv;
  }

  void HeapDecreaseKey(const int& v)
  {
    // where in Heap is vertex v
    int i = this->HeapIndices[ v ];
    if ( i < 1 || i > static_cast<int>(this->HeapSize) )
      {
      return;
      }

    while ( i > 1 &&
            this->CumulativeWeights[ this->Heap[ i/2 ] ] >
            this->CumulativeWeights[ v ] )
      {
      this->Heap[ i ] = this->Heap[i/2];
      this->HeapIndices[ this->Heap[i] ] = i;
      i /= 2;
      }

    // Heap and HeapIndices are kinda inverses
    this->Heap[ i ] = v;
    this->HeapIndices[ v ] = i;
  }

  void ResetHeap()
  {
    this->HeapSize = 0;
  }

  void InitializeHeap(const int& size)
  {
    this->Heap.resize( size + 1 );
    this->HeapIndices.resize( size );
  }

private:
  unsigned int HeapSize;

  // The priority que (a binary heap) with vertex indices.
  std::vector<int> Heap;

  // HeapIndices(v) the position of v in Heap (HeapIndices and Heap are kind of inverses).
  std::vector<int> HeapIndices;

};

#endif
// VTK-HeaderTest-Exclude: vtkDijkstraGraphInternals.h
