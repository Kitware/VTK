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

#include "vtkTreeLayoutStrategy.h"

#include "vtkAbstractArray.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTree.h"
#include "vtkTreeDFSIterator.h"

vtkCxxRevisionMacro(vtkTreeLayoutStrategy, "1.1");
vtkStandardNewMacro(vtkTreeLayoutStrategy);

vtkTreeLayoutStrategy::vtkTreeLayoutStrategy()
{
  this->Angle = 90;
  this->Radial = false;
  this->LogSpacingValue = 1.0;
  this->LeafSpacing = 0.9;
  this->DistanceArrayName = NULL;
}

vtkTreeLayoutStrategy::~vtkTreeLayoutStrategy()
{
  this->SetDistanceArrayName(NULL);
}

// Tree layout method
void vtkTreeLayoutStrategy::Layout()
{
  vtkTree* tree = vtkTree::SafeDownCast(this->Graph);
  if (tree != NULL)
    {
    vtkPoints* newPoints = vtkPoints::New();
    newPoints->SetNumberOfPoints(tree->GetNumberOfVertices());

    // Check if the distance array is defined.
    vtkDataArray* distanceArr = NULL;
    if (this->DistanceArrayName != NULL)
      {
      vtkAbstractArray* aa = tree->GetVertexData()->
        GetArray(this->DistanceArrayName);
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
    vtkTreeDFSIterator* iter = vtkTreeDFSIterator::New();
    iter->SetTree(tree);
    while (iter->HasNext())
      {
      vtkIdType vertex = iter->Next();
      if (tree->IsLeaf(vertex))
        {
        leafCount++;
        }
      if (tree->GetLevel(vertex) > maxLevel)
        {
        maxLevel = tree->GetLevel(vertex);
        }
      }
    // Don't count the root in the list of internal nodes.
    vtkIdType internalCount = tree->GetNumberOfVertices() - leafCount - 1;
    double leafSpacing = this->LeafSpacing / static_cast<double>(leafCount);
    double internalSpacing = (1.0 - this->LeafSpacing) / static_cast<double>(internalCount);

    double angleRad = this->Angle * vtkMath::Pi() / 180.0;
    double spacing;
    if (this->LogSpacingValue == 1.0)
      {
      if (this->Radial)
        {
        spacing = 1.0 / maxLevel;
        }
      else
        {
        spacing = 0.5 / tan(angleRad / 2);
        }
      }
    else
      {
      spacing = this->LogSpacingValue;
      }

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
        if (this->LogSpacingValue == 1.0)
          {
          height = spacing * tree->GetLevel(vertex) / static_cast<double>(maxLevel);
          }
        else
          {
          height = (1 - pow(spacing, tree->GetLevel(vertex) + 1.0)) / (1 - spacing) - 1.0;
          }
        }

      double x, y;
      if (this->Radial)
        {
        double ang;
        if (tree->IsLeaf(vertex))
          {
          ang = 2.0 * vtkMath::Pi() * curPlace;
          ang *= this->Angle / 360.0;
          ang -= vtkMath::Pi() / 2.0 + vtkMath::Pi()*this->Angle / 180.0;
          curPlace += leafSpacing;
          }
        else
          {
          curPlace += internalSpacing;
          vtkIdType nchildren;
          const vtkIdType* children;
          tree->GetChildren(vertex, nchildren, children);
          double minAng = 2*vtkMath::Pi();
          double maxAng = 0.0;
          double angSinSum = 0.0;
          double angCosSum = 0.0;
          for (vtkIdType c = 0; c < nchildren; c++)
            {
            double pt[3];
            newPoints->GetPoint(children[c], pt);
            double leafAngle = atan2(pt[1], pt[0]);
            if (leafAngle < 0)
              {
              leafAngle += 2*vtkMath::Pi();
              }
            if (leafAngle < minAng)
              {
              minAng = leafAngle;
              }
            if (leafAngle > maxAng)
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
          vtkIdType nchildren;
          const vtkIdType* children;
          tree->GetChildren(vertex, nchildren, children);
          double minX = VTK_DOUBLE_MAX;
          double maxX = VTK_DOUBLE_MIN;
          for (vtkIdType c = 0; c < nchildren; c++)
            {
            double pt[3];
            newPoints->GetPoint(children[c], pt);
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
    tree->SetPoints(newPoints);

    // Clean up.
    iter->Delete();
    newPoints->Delete();
    }
  else
    {
    vtkErrorMacro(<<"tree layout currently only works on trees.");
    }
} 

void vtkTreeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Angle: " << this->Angle << endl;
  os << indent << "Radial: " << (this->Radial ? "true" : "false") << endl;
  os << indent << "LogSpacingValue: " << this->LogSpacingValue << endl;
  os << indent << "LeafSpacing: " << this->LeafSpacing << endl;
  os << indent << "DistanceArrayName: " 
     << (this->DistanceArrayName ? this->DistanceArrayName : "(null)") << endl;
}

#if 0
// Code storage

#include "vtkGraphToBoostAdapter.h"
#include "vtkTreeToBoostAdapter.h"
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/property_map.hpp>
#include <boost/vector_property_map.hpp>
#include <boost/pending/queue.hpp>

using namespace boost;

// Redefine the bfs visitor, the only visitor we
// are using is the tree_edge visitor.
template <typename PlacementMap>
  class placement_visitor : public default_dfs_visitor
  {
  public:
    placement_visitor() { }
    placement_visitor(PlacementMap dist, typename property_traits<PlacementMap>::value_type ii = 1) 
      : d(dist), cur_place(0), internal_inc(ii) { }

    template <typename Vertex, typename Graph>
    void finish_vertex(Vertex v, const Graph& g)
    {
      put(d, v, cur_place);
      if (g->IsLeaf(v))
        {
        cur_place += 1;
        }
      else
        {
        cur_place += internal_inc;
        }
    }

    typename property_traits<PlacementMap>::value_type max_place() { return cur_place; }

  private:
    PlacementMap d;
    typename property_traits<PlacementMap>::value_type cur_place;
    typename property_traits<PlacementMap>::value_type internal_inc;
  };

    // Create a color map (used for marking visited nodes)
    //vector_property_map<default_color_type> color;
    //vtkDoubleArray* placement = vtkDoubleArray::New();
    //depth_first_search(tree, 
    //  placement_visitor<vtkDoubleArray*>(placement, 1.0), 
    //  color, tree->GetRoot());
#endif



