/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpanTreeLayoutStrategy.cxx

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

// File: vtkSpanTreeLayoutStrategy.cxx
// Graph visualization library for VTK
// (c) 2003 D.J. Duke

#include "vtkSpanTreeLayoutStrategy.h"
#include "vtkConeLayoutStrategy.h"
#include "vtkObjectFactory.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkTree.h"
#include "vtkEdgeListIterator.h"
#include "vtkInEdgeIterator.h"
#include "vtkOutEdgeIterator.h"
#include "vtkGraphLayout.h"
#include "vtkGraph.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkSmartPointer.h"

//--------------------------------------------------------------------------

vtkStandardNewMacro(vtkSpanTreeLayoutStrategy);

vtkSpanTreeLayoutStrategy::vtkSpanTreeLayoutStrategy()
{
  this->TreeLayout             = vtkConeLayoutStrategy::New();
  this->DepthFirstSpanningTree = false;
}

vtkSpanTreeLayoutStrategy::~vtkSpanTreeLayoutStrategy()
{
  if (this->TreeLayout)
    {
    this->TreeLayout->Delete();
    this->TreeLayout = NULL;
    }
}

// Edges that cross levels more than one level of the layout
// will have edge-points inserted to match the structure of
// the rest of the graph.  However, in order to compute the
// position of these points, we first need to lay out a
// graph in which these edge points are represented by real
// vertices.  This struct is used to keep trach of the
// relationship between the proxy nodes in the graph used
// to compute the layout, and edges in the original graph.

struct _vtkBridge_s
{
  vtkEdgeType edge;
  vtkIdType delta;
  vtkIdType anchor[2];
};

void vtkSpanTreeLayoutStrategy::Layout()
{
  vtkSmartPointer<vtkPoints> points
    = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkMutableDirectedGraph> spanningDAG
    = vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  vtkSmartPointer<vtkGraphLayout> layoutWorker
    = vtkSmartPointer<vtkGraphLayout>::New();
  vtkSmartPointer<vtkOutEdgeIterator> outEdges
    = vtkSmartPointer<vtkOutEdgeIterator>::New();
  vtkSmartPointer<vtkInEdgeIterator> inEdges
    = vtkSmartPointer<vtkInEdgeIterator>::New();


  // Auxiliary structures used for building a spanning tree
  int *level;
  int *marks;
  vtkIdType *queue;
  vtkIdType front, back;

  // Handle for the layout computed for the spanning tree
  vtkPoints *layout;

  // Auxiliary structures for placing bends into edges.
  _vtkBridge_s *editlist;
  _vtkBridge_s link;
  link.delta = 0;
  link.anchor[1] = 0;
  vtkIdType editsize;
  vtkEdgeType edge;
  vtkIdType i, nrNodes, nrEdges;
  double pointS[3], pointT[3], pointA[3];
  double edgePoints[6];

  // ----------------------------------------------------------

  vtkDebugMacro(<<"vtkSpanTreeLayoutStrategy executing.");

  // Ensure that all required inputs are available.
  nrNodes = this->Graph->GetNumberOfVertices();
  nrEdges = this->Graph->GetNumberOfEdges();

  if (nrNodes == 0 || nrEdges == 0 || !this->TreeLayout)
    {
    if (nrNodes == 0)
      {
      vtkErrorMacro(<< "Cannot execute - no nodes in input." );
      }
    if (nrEdges == 0)
      {
      vtkErrorMacro(<< "Cannot execute - no edges in input." );
      }
    if (!this->TreeLayout)
      {
      vtkErrorMacro(<< "Cannot execute - no tree layout strategy." );
      }
    return;
    }

  // Compute a spanning tree from the graph.  This is done inline here
  // rather than via a Boost class so we can offer a choice of spanning
  // tree.  Graph traversal is supported by a queue, and during
  // traversal the (tree)level is calculated for each vertex.
  level = new int [nrNodes];
  marks = new int [nrNodes];
  queue = new vtkIdType [nrNodes];

  // Initialize spanning tree with all vertices of the graph.
  for (vtkIdType v = 0; v < nrNodes; v++)
    {
    spanningDAG->AddVertex();
    marks[v] = 0;
    }

  // Strategy: iterate over the vertices of the graph.
  // As each unvisited vertex is found, we perform a traversal starting
  // from that vertex.  The result is technically a spanning forest.
  for (vtkIdType v = 0; v < nrNodes; v++)
    {
    if (!marks[v])           // not visited
      {
      front = back = 0;
      queue[back++] = v;     // push node v
      level[v] = 0;
      marks[v] = 1;          // mark as visited
      while (back != front)
        {
        vtkIdType src;
        if (this->DepthFirstSpanningTree)
          {
          src = queue[--back];    // stack discipline = depth-first traversal
          }
        else
          {
          src = queue[front++];   // queue discipline = breadth-first traversal
          }
        // Look at outgoing edges from this node,
        // adding any unseen targets to the queue,
        // and edges to the spanning tree.
        this->Graph->GetOutEdges(src, outEdges);
        while (outEdges->HasNext())
          {
          vtkIdType dst = outEdges->Next().Target;
          if (marks[dst] == 0) // not seen or done
            {
            level[dst] = level[src]+1;
            queue[back++] = dst;
            spanningDAG->AddGraphEdge(src,dst);
            marks[dst] = 1;  //seen
            }
          }
        // Look at incoming edges: as per outgoing edges.
        this->Graph->GetInEdges(src, inEdges);
        while (inEdges->HasNext())
          {
          vtkIdType origin = inEdges->Next().Source;
          if (marks[origin] == 0) // not seen or done
            {
            level[origin] = level[src]+1;
            queue[back++] = origin;
            spanningDAG->AddGraphEdge(src,origin);
            marks[origin] = 1;  //seen
            }
          }
        } // while back != front
      } // if !marks[v]
    } // for each vertex


  // Check each edge to see if it spans more than one level of
  // the tree.  If it does, the edge will be drawn using edge-points,
  // and before we lay out the tree, we need to insert proxy nodes
  // to compute the position for those points.

  editsize = 0;
  editlist = new _vtkBridge_s[nrEdges];
  this->Graph->GetEdges(edges);
  while (edges->HasNext())
    {
    link.edge = edges->Next();
    // Loop ...
    if (link.edge.Source == link.edge.Target)
      {
      link.anchor[0] = spanningDAG->AddVertex();
      spanningDAG->AddEdge(link.edge.Source,link.anchor[0]);
      editlist[editsize++] = link;
      continue;
      }
    // If the difference in level between the start and end nodes
    // is greater than one, this edge, by definition, is not
    // present in the layout tree.
    link.delta = level[link.edge.Target] - level[link.edge.Source];
    if (abs(static_cast<int>(link.delta)) > 1)
      {
      link.anchor[0] = spanningDAG->AddVertex();
      spanningDAG->AddEdge(link.delta > 0 ? link.edge.Source : link.edge.Target, link.anchor[0]);
      if (abs(static_cast<int>(link.delta)) > 2)
        {
        link.anchor[1] = spanningDAG->AddVertex();
        spanningDAG->AddEdge(link.anchor[0], link.anchor[1]);
        }
      editlist[editsize++] = link;
      }
    }

  //  Layout the tree using the layout filter provided.
  layoutWorker->SetLayoutStrategy(this->TreeLayout);
  layoutWorker->SetInputData(spanningDAG);
  layoutWorker->Update();
  layout = layoutWorker->GetOutput()->GetPoints();

  // Copy the node positions for nodes in the original
  // graph from the layout tree to the output positions.
  points->SetNumberOfPoints(nrNodes);
  for (i = 0; i < nrNodes; i++)
    {
    points->SetPoint(i, layout->GetPoint(i));
    }

  // Now run through the edit list, computing the position for
  // each of the edge points
  for (i = 0; i < editsize; i++)
    {
    link = editlist[i];
    if (link.delta == 0)
      {
      // Loop: Each loop is drawn as an edge with 2 edge points.  The x & y
      // coordinates have been fixed by the layout.  The z coordinates are
      // scaled to that the edge points are 1/3 of the distance between
      // levels, above and below the node.
      layout->GetPoint(link.edge.Source, pointS);
      layout->GetPoint(link.anchor[0],   pointA);
      edgePoints[0] = edgePoints[3] = pointA[0];
      edgePoints[1] = edgePoints[4] = pointA[1];
      edgePoints[2] = pointS[2] + (pointA[2]-pointS[2])/3.0;
      edgePoints[5] = pointS[2] - (pointA[2]-pointS[2])/3.0;
      this->Graph->SetEdgePoints(link.edge.Id, 2, edgePoints);
      }
    else if (link.delta > 1)
      {
      layout->GetPoint(link.edge.Source, pointS);
      layout->GetPoint(link.edge.Target, pointT);
      layout->GetPoint(link.anchor[0],   pointA);
      edgePoints[0] = pointA[0];
      edgePoints[1] = pointA[1];
      edgePoints[2] = pointS[2] + (pointT[2] - pointS[2])/link.delta;
      if (link.delta > 2)
        {
        layout->GetPoint(link.anchor[1], pointA);
        edgePoints[3] = edgePoints[0];
        edgePoints[4] = edgePoints[1];
        edgePoints[5] = pointS[2] + (link.delta-1)*(pointT[2] - pointS[2])/link.delta;
        this->Graph->SetEdgePoints(link.edge.Id, 2, edgePoints);
        }
      else
        {
        this->Graph->SetEdgePoints(link.edge.Id, 1, edgePoints);
        }
      }
    else if (link.delta < -1)
      {
      int delta = -link.delta;
      layout->GetPoint(link.edge.Source, pointS);
      layout->GetPoint(link.edge.Target, pointT);
      layout->GetPoint(link.anchor[0],   pointA);
      edgePoints[0] = pointA[0];
      edgePoints[1] = pointA[1];
      edgePoints[2] = pointS[2] + (pointT[2] - pointS[2])/delta;
      if (link.delta < -2)
        {
        layout->GetPoint(link.anchor[1], pointA);
        edgePoints[3] = edgePoints[0];
        edgePoints[4] = edgePoints[1];
        edgePoints[5] = pointS[2] + (delta-1)*(pointT[2] - pointS[2])/delta;
        this->Graph->SetEdgePoints(link.edge.Id, 2, edgePoints);
        }
      else
        {
        this->Graph->SetEdgePoints(link.edge.Id, 1, edgePoints);
        }
      }
    }

  // Clean up temporary storage.
  delete [] editlist;
  delete [] level;
  delete [] marks;
  delete [] queue;

  this->Graph->SetPoints(points);
  vtkDebugMacro(<<"SpanTreeLayoutStrategy complete.");
}

void vtkSpanTreeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGraphLayoutStrategy::PrintSelf(os,indent);
  os << indent << "TreeLayout: " << (this->TreeLayout ? "" : "(none)") << endl;
  if (this->TreeLayout)
    {
    this->TreeLayout->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "DepthFirstSpanningTree: " << (this->DepthFirstSpanningTree ? "On" : "Off") << endl;
}


