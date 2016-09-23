/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkConeLayoutStrategy.cxx

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

#include "vtkConeLayoutStrategy.h"

#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkOutEdgeIterator.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkGraphEdge.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkConeLayoutStrategy);

vtkConeLayoutStrategy::vtkConeLayoutStrategy()
{
  this->Compression = 0;
  this->Compactness = 0.75;
  this->Spacing = 1.0;
}

vtkConeLayoutStrategy::~vtkConeLayoutStrategy()
{
}

double vtkConeLayoutStrategy::LocalPlacement(vtkIdType node, vtkPoints *points)
{
  vtkIdType child, prevChild, nrChildren;
  VTK_CREATE( vtkOutEdgeIterator, children );

  double circum = 0;      // circum of cone below this node
  double radius;          // radius of cone
  double dAlpha;
  double alpha = 0;
  double largest = 0.0;    // size of largest cone of any child

  double *radii;
  double cx = 0, cy = 0, cr = 0;
  double vx, vy, norm, i1x, i1y, i3x, i3y;

  // Initially position this node at the origin ...
  points->SetPoint(node, 0.0, 0.0, 0.0);

  nrChildren = this->Graph->GetOutDegree(node);

  // If there are no children, we are done.
  if (nrChildren == 0)
  {
    return 1.0;
  }

  this->Graph->GetOutEdges(node, children);
  if (nrChildren == 1)
  {
    // For one child, simply position that child; radius required
    // for the tree will then depend on radius required for that
    // child.
    radius = this->LocalPlacement(
      children->NextGraphEdge()->GetTarget(),
      points);
    return radius;
  }

  // If there is more than one child nodes ...
  radii = new double[nrChildren];
  for (vtkIdType i = 0; i < nrChildren; i++)
  {
    // Layout the next child and record its raidus.  If
    // necessary update the size of largest child cone.
    // Accumulate the (approximate) arc-length
    // required by child nodes as "circum".

    child = children->NextGraphEdge()->GetTarget();
    radii[i] =  this->LocalPlacement(child, points);
    circum += radii[i]*2.0;
    if (radii[i] > largest)
    {
      largest = radii[i];
    }
  }
  radius = circum / (2.0*vtkMath::Pi());

  // Iterate over the children, assigning the node of each
  // a position around a circle of the required radius,
  // based on the radii of that child and its predecessor,

  prevChild = nrChildren-1;
  this->Graph->GetOutEdges(node, children);

  for (vtkIdType j = 0; j < nrChildren; j++)
  {
    child = children->NextGraphEdge()->GetTarget();
    dAlpha = (radii[j] + radii[prevChild]) / radius;
    alpha  += dAlpha;
    points->SetPoint(child, radius*cos(alpha), radius*sin(alpha), 0.0);
    prevChild = j;
    if (j == 0)
    {
      cx = radius*cos(alpha);
      cy = radius*sin(alpha);
      cr = radius;
    }
    else
    {
      vx = cx - radius*cos(alpha);
      vy = cy = radius*sin(alpha);
      norm = sqrt(vx*vx + vy*vy);
      if (norm == 0.0)
      {
        continue;
      }
      vx /= norm;
      vy /= norm;
      i1x = cx - vx*cr;
      i1y = cy - vy*cr;
      i3x = radius*cos(alpha) - radius*vx;
      i3y = radius*sin(alpha) - radius*vy;
      norm = sqrt((i1x-i3x)*(i1x-i3x) + (i1y-i3y)*(i1y-i3y));
      if (radius > norm)
      {
        cx = radius*cos(alpha);
        cy = radius*sin(alpha);
        cr = radius;
      }
      else
      {
        if (norm > cr)
        {
          cx = (i1x + i3x)/2.0;
          cy = (i1y + i3y)/2.0;
          cr = norm/2.0;
        }
      }
    }
  }

  delete [] radii;

  // Update statistics, used when height of cones is calculated.
  if (radius < this->MinRadius)
  {
    this->MinRadius = radius;
  }
  if (radius > this->MaxRadius)
  {
    this->MaxRadius = radius;
  }
  this->SumOfRadii += radius;
  this->NrCones++;

  // For compact placement, space is allocated simply for the
  // radius of this cone plus "a little" extra.  For non-compact
  // placement, allow also for the radius allocated to the
  // largest child.
  return radius + (this->Compression ? 1 : cr);
}

// Calculate the final layout for a node, given its level in
// the tree, and the (final) position of its parent.


void vtkConeLayoutStrategy::GlobalPlacement(
  vtkIdType root,
  vtkPoints *points,
  double refX,   // parent's X coord
  double refY,   // parent's Y coord
  double level ) // level of this node in the tree.
{
  vtkIdType child;
  VTK_CREATE( vtkOutEdgeIterator, children );

  double final[3];

  points->GetPoint(root, final);

  final[0] += refX;
  final[1] += refY;
  if (this->Compression)
  {
    final[2] = level*this->Spacing;
  }
  else
  {
    final[2] = level*this->Spacing*(this->MaxRadius*this->Compactness);
  }
  points->SetPoint(root, final);

  // Having fixed the position of "root", now iterate over its
  // children and fix their positions, remembering that these
  // are one level further down the tree.

  this->Graph->GetOutEdges(root, children);
  while (children->HasNext())
  {
    child = children->NextGraphEdge()->GetTarget();
    this->GlobalPlacement(child, points, final[0], final[1], level+1);
  }
}

void vtkConeLayoutStrategy::Layout()
{
  VTK_CREATE( vtkMutableDirectedGraph, superGraph );
  VTK_CREATE( vtkPoints, points );
  vtkIdType numVerts = this->Graph->GetNumberOfVertices();

  vtkGraph *temp;
  vtkIdType node, root, nrRoots;

  VTK_CREATE( vtkPoints, tmpPoints );

  tmpPoints->SetNumberOfPoints(numVerts+1);  // Allow for artificial root.
  points->SetNumberOfPoints(numVerts);  // Allow for artificial root.

  // Assume that graph is either a tree or a forest.
  // Force it to be a tree by installing a new root and linking this to all existing
  // nodes with in-degree 0.
  superGraph->DeepCopy(this->Graph);
  temp = this->Graph;
  this->Graph = superGraph;

  nrRoots = 0;
  root = superGraph->AddVertex();
  for (node = 0; node < numVerts; node++)
  {
    if (superGraph->GetInDegree(node) == 0)
    {
      superGraph->AddEdge(root,node);
      nrRoots++;
    }
  }
  if (nrRoots == 0)
  {
    vtkErrorMacro(<<"No roots found in input dataset - output may be ill-defined.");
  }

  this->MinRadius  = 1.0E10F;
  this->MaxRadius  = 0.0;
  this->SumOfRadii = 0.0;
  this->NrCones    = 0;

  // Layout is performed in two passes.  First, find a provisional location
  // for each node, via a bottom-up traversal.  Then calculate a final
  // position for each node, by performing a top-down traversal, placing
  // the root at the origin, and then positioning each child using the
  // provisional location of the child and final location of the parent.

  this->LocalPlacement(root, tmpPoints);

  // Second pass: fix absolute node position.
  this->GlobalPlacement(root, tmpPoints, 0.0, 0.0, 0.0);

  vtkIdType c;
  double p[3];
  for (c = 0; c < numVerts; c++)
  {
    tmpPoints->GetPoint(c, p);
    points->SetPoint(c,p);
  }

  this->Graph = temp;
  this->Graph->SetPoints(points);
}

void vtkConeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Spacing: " << this->Spacing << endl;
  os << indent << "Compactness: " << this->Compactness << endl;
  os << indent << "Compression: " << this->Compression << endl;
}

