/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeLayoutStrategy.cxx

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

#include "vtkTreeLayoutStrategy.h"

#include "vtkAbstractArray.h"
#include "vtkAdjacentVertexIterator.h"
#ifdef VTK_USE_BOOST
#include "vtkBoostBreadthFirstSearchTree.h"
#endif
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"

vtkStandardNewMacro(vtkTreeLayoutStrategy);

vtkTreeLayoutStrategy::vtkTreeLayoutStrategy()
{
  this->Angle = 90;
  this->Radial = false;
  this->LogSpacingValue = 1.0;
  this->LeafSpacing = 0.9;
  this->DistanceArrayName = NULL;
  this->Rotation = 0.0;
  this->ReverseEdges = false;
}

vtkTreeLayoutStrategy::~vtkTreeLayoutStrategy()
{
  this->SetDistanceArrayName(NULL);
}

// Tree layout method
void vtkTreeLayoutStrategy::Layout()
{
  // Do I have a graph to lay out?  Does it have any vertices?
  if (this->Graph == NULL || this->Graph->GetNumberOfVertices() <= 0)
  {
    return;
  }

  vtkTree* tree = vtkTree::SafeDownCast(this->Graph);
  if (tree == NULL)
    {
#ifdef VTK_USE_BOOST
    // Use the BFS search tree to perform the layout
    vtkBoostBreadthFirstSearchTree* bfs = vtkBoostBreadthFirstSearchTree::New();
    bfs->CreateGraphVertexIdArrayOn();
    bfs->SetReverseEdges(this->ReverseEdges);
    bfs->SetInputData(this->Graph);
    bfs->Update();
    tree = vtkTree::New();
    tree->ShallowCopy(bfs->GetOutput());
    bfs->Delete();
    if (tree->GetNumberOfVertices() != this->Graph->GetNumberOfVertices())
      {
      vtkErrorMacro("Tree layout only works on connected graphs");
      tree->Delete();
      return;
      }
#else
    vtkErrorMacro("Layout only works on vtkTree unless VTK_USE_BOOST is on.");
    return;
#endif
    }

  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints(tree->GetNumberOfVertices());

  vtkDoubleArray *anglesArray = vtkDoubleArray::New();
  if( this->Radial )
  {
    anglesArray->SetName( "subtended_angles" );
    anglesArray->SetNumberOfComponents(2);
    anglesArray->SetNumberOfTuples(tree->GetNumberOfVertices());
    vtkDataSetAttributes *data = tree->GetVertexData();
    data->AddArray(anglesArray);
  }

  // Check if the distance array is defined.
  vtkDataArray* distanceArr = NULL;
  if (this->DistanceArrayName != NULL)
    {
    vtkAbstractArray* aa = tree->GetVertexData()->
      GetAbstractArray(this->DistanceArrayName);
    if (!aa)
      {
      vtkErrorMacro("Distance array not found.");
      return;
      }
    distanceArr = vtkDataArray::SafeDownCast(aa);
    if (!distanceArr)
      {
      vtkErrorMacro("Distance array must be a data array.");
      return;
      }
    }
  double maxDistance = 1.0;
  if (distanceArr)
    {
    maxDistance = distanceArr->GetMaxNorm();
    }

  // Count the number of leaves in the tree
  // and get the maximum depth
  vtkIdType leafCount = 0;
  vtkIdType maxLevel = 0;
  vtkIdType lastLeafLevel = 0;
  vtkTreeDFSIterator* iter = vtkTreeDFSIterator::New();
  iter->SetTree(tree);
  while (iter->HasNext())
    {
    vtkIdType vertex = iter->Next();
    if (tree->IsLeaf(vertex))
      {
      leafCount++;
      lastLeafLevel = tree->GetLevel(vertex);
      }
    if (tree->GetLevel(vertex) > maxLevel)
      {
      maxLevel = tree->GetLevel(vertex);
      }
    }

  // Divide the "extra spacing" between tree branches among all internal nodes.
  // When the angle is 360, we want to divide by
  // internalCount - 1 (taking out just the root),
  // so that there is extra space where the tree meets itself.
  // When the angle is lower (here we say 270 or lower),
  // we should to divide by internalCount - lastLeafLevel,
  // so that the tree ends exactly at the sweep angle end points.
  // To do this, we interpolate between these values.
  vtkIdType internalCount = tree->GetNumberOfVertices() - leafCount;
  double alpha = (this->Angle - 270) / 90;
  if (alpha < 0.0)
    {
    alpha = 0.0;
    }
  double internalCountInterp = alpha*(internalCount - 1) + (1.0 - alpha)*(internalCount - lastLeafLevel);
  double internalSpacing = 0.0;
  if (internalCountInterp != 0.0)
    {
    internalSpacing = (1.0 - this->LeafSpacing) / internalCountInterp;
    }

  // Divide the spacing between tree leaves among all leaf nodes.
  // This is similar to the interpolation for internal spacing.
  // When the angle is close to 360, we want space between the first and last leaf nodes.
  // When the angle is lower (less than 270), we fill the full sweep angle so divide
  // by leafCount - 1 to take out this extra space.
  double leafCountInterp = alpha*leafCount + (1.0 - alpha)*(leafCount - 1);
  double leafSpacing = this->LeafSpacing / leafCountInterp;

  double spacing = this->LogSpacingValue;

  // The distance between level L-1 and L is s^L.
  // Thus, if s < 1 then the distance between levels gets smaller in higher levels,
  //       if s = 1 the distance remains the same, and
  //       if s > 1 the distance get larger.
  // The height (distance from the root) of level L, then, is
  // s + s^2 + s^3 + ... + s^L, where s is the log spacing value.
  // The max height (used for normalization) is
  // s + s^2 + s^3 + ... + s^maxLevel.
  // The quick formula for computing this is
  // sum_{i=1}^{n} s^i = (s^(n+1) - 1)/(s - 1) - 1        if s != 1
  //                   = n                                if s == 1
  double maxHeight = maxLevel;
  double eps = 1e-8;
  double diff = spacing - 1.0 > 0 ? spacing - 1.0 : 1.0 - spacing;
  if (diff > eps)
    {
    maxHeight = (pow(spacing, maxLevel+1.0) - 1.0)/(spacing - 1.0) - 1.0;
    }

  vtkSmartPointer<vtkAdjacentVertexIterator> it =
    vtkSmartPointer<vtkAdjacentVertexIterator>::New();
  double curPlace = 0;
  iter->SetMode(vtkTreeDFSIterator::FINISH);
  while (iter->HasNext())
    {
    vtkIdType vertex = iter->Next();

    double height;
    if (distanceArr != NULL)
      {
      height = spacing * distanceArr->GetTuple1(vertex) / maxDistance;
      }
    else
      {
      if (diff <= eps)
        {
        height = tree->GetLevel(vertex)/maxHeight;
        }
      else
        {
        height = ((pow(spacing, tree->GetLevel(vertex)+1.0) - 1.0)/(spacing - 1.0) - 1.0)/maxHeight;
        }
      }

    double x, y;
    if (this->Radial)
      {
      double ang;
      if (tree->IsLeaf(vertex))
        {

        // 1) Compute the position in the arc
        // 2) Spin around so that the tree leaves are at
        //    the bottom and centered
        // 3) Convert to radians
        double angleInDegrees = curPlace * this->Angle;

        angleInDegrees -= (90+this->Angle/2);

        // Convert to radians
        ang = angleInDegrees * vtkMath::Pi() / 180.0;

        curPlace += leafSpacing;

          //add the subtended angles to an array for possible use later...
        double subtended_angle[2];
        double total_arc = (curPlace*this->Angle) - (90.+this->Angle/2.) - angleInDegrees;
        double angle1 = angleInDegrees - (total_arc/2.) + 360.;
        double angle2 = angleInDegrees + (total_arc/2.) + 360.;
        subtended_angle[0] = angle1;
        subtended_angle[1] = angle2;
        anglesArray->SetTuple( vertex, subtended_angle );
        }
      else
        {
        curPlace += internalSpacing;
        tree->GetChildren(vertex, it);
        double minAng = 2*vtkMath::Pi();
        double maxAng = 0.0;
        double angSinSum = 0.0;
        double angCosSum = 0.0;
        bool first = true;
        while (it->HasNext())
          {
          vtkIdType child = it->Next();
          double pt[3];
          newPoints->GetPoint(child, pt);
          double leafAngle = atan2(pt[1], pt[0]);
          if (leafAngle < 0)
            {
            leafAngle += 2*vtkMath::Pi();
            }
          if (first)
            {
            minAng = leafAngle;
            first = false;
            }
          if (!it->HasNext())
            {
            maxAng = leafAngle;
            }
          angSinSum += sin(leafAngle);
          angCosSum += cos(leafAngle);
          }
        // This is how to take the average of the two angles minAng, maxAng
        ang = atan2(sin(minAng) + sin(maxAng), cos(minAng) + cos(maxAng));

        // Make sure the angle is on the same "side" as the average angle.
        // If not, add pi to the angle. This handles some border cases.
        double avgAng = atan2(angSinSum, angCosSum);
        if (sin(ang)*sin(avgAng) + cos(ang)*cos(avgAng) < 0)
          {
          ang += vtkMath::Pi();
          }

          //add the subtended angles to an array for possible use later...
        double subtended_angle[2];
        double angle1 = vtkMath::DegreesFromRadians( minAng );
        double angle2 = vtkMath::DegreesFromRadians( maxAng );

        subtended_angle[0] = angle1;
        subtended_angle[1] = angle2;
        anglesArray->SetTuple( vertex, subtended_angle );
        }
      x = height * cos(ang);
      y = height * sin(ang);
      }
    else
      {
      double width = 2.0 * tan(vtkMath::Pi()*this->Angle / 180.0 / 2.0);
      y = -height;
      if (tree->IsLeaf(vertex))
        {
        x = width * curPlace;
        curPlace += leafSpacing;
        }
      else
        {
        curPlace += internalSpacing;
        tree->GetChildren(vertex, it);
        double minX = VTK_DOUBLE_MAX;
        double maxX = VTK_DOUBLE_MIN;
        while (it->HasNext())
          {
          vtkIdType child = it->Next();
          double pt[3];
          newPoints->GetPoint(child, pt);
          if (pt[0] < minX)
            {
            minX = pt[0];
            }
          if (pt[0] > maxX)
            {
            maxX = pt[0];
            }
          }
        x = (minX + maxX) / 2.0;
        }
      }
    newPoints->SetPoint(vertex, x, y, 0.0);
    }

  // Rotate coordinates
  if (this->Rotation != 0.0)
    {
    vtkSmartPointer<vtkTransform> t = vtkSmartPointer<vtkTransform>::New();
    t->RotateZ(this->Rotation);
    double x[3];
    double y[3];
    for (vtkIdType p = 0; p < newPoints->GetNumberOfPoints(); ++p)
      {
      newPoints->GetPoint(p, x);
      t->TransformPoint(x, y);
      newPoints->SetPoint(p, y);
      }
    }

  // Copy coordinates back into the original graph
  if (vtkTree::SafeDownCast(this->Graph))
    {
    this->Graph->SetPoints(newPoints);
    }
#ifdef VTK_USE_BOOST
  else
    {
    // Reorder the points based on the mapping back to graph vertex ids
    vtkPoints* reordered = vtkPoints::New();
    reordered->SetNumberOfPoints(newPoints->GetNumberOfPoints());
    for (vtkIdType i = 0; i < reordered->GetNumberOfPoints(); i++)
      {
      reordered->SetPoint(i, 0, 0, 0);
      }
    vtkIdTypeArray* graphVertexIdArr = vtkIdTypeArray::SafeDownCast(
      tree->GetVertexData()->GetAbstractArray("GraphVertexId"));
    for (vtkIdType i = 0; i < graphVertexIdArr->GetNumberOfTuples(); i++)
      {
      reordered->SetPoint(graphVertexIdArr->GetValue(i), newPoints->GetPoint(i));
      }
    this->Graph->SetPoints(reordered);
    tree->Delete();
    reordered->Delete();
    }
#endif

  // Clean up.
  iter->Delete();
  newPoints->Delete();
  anglesArray->Delete();
}

void vtkTreeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Angle: " << this->Angle << endl;
  os << indent << "Radial: " << (this->Radial ? "true" : "false") << endl;
  os << indent << "LogSpacingValue: " << this->LogSpacingValue << endl;
  os << indent << "LeafSpacing: " << this->LeafSpacing << endl;
  os << indent << "Rotation: " << this->Rotation << endl;
  os << indent << "DistanceArrayName: "
     << (this->DistanceArrayName ? this->DistanceArrayName : "(null)") << endl;
  os << indent << "ReverseEdges: " << this->ReverseEdges << endl;
}


