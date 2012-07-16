/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphItem.h"

#include "vtkContext2D.h"
#include "vtkGraph.h"
#include "vtkImageData.h"
#include "vtkMarkerUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

#include <vector>

vtkStandardNewMacro(vtkGraphItem);
vtkCxxSetObjectMacro(vtkGraphItem, Graph, vtkGraph);

struct vtkGraphItem::Internals {
  std::vector<float> VertexSizes;
  std::vector<vtkVector2f> VertexPositions;
  std::vector<vtkColor4ub> VertexColors;
  std::vector<int> VertexMarkers;

  std::vector<std::vector<vtkVector2f> > EdgePositions;
  std::vector<std::vector<vtkColor4ub> > EdgeColors;
  std::vector<float> EdgeWidths;
};

vtkGraphItem::vtkGraphItem()
{
  this->Graph = 0;
  this->GraphBuildTime = 0;
  this->Internal = new Internals();
}

vtkGraphItem::~vtkGraphItem()
{
  delete this->Internal;
  if (this->Graph)
    {
    this->Graph->Delete();
    }
}

vtkColor4ub vtkGraphItem::VertexColor(vtkIdType item)
{
  return vtkColor4ub(128, 128, 128, 255);
}

vtkVector2f vtkGraphItem::VertexPosition(vtkIdType item)
{
  double *p = this->Graph->GetPoint(item);
  return vtkVector2f(static_cast<float>(p[0]), static_cast<float>(p[1]));
}

float vtkGraphItem::VertexSize(vtkIdType item)
{
  return 10.0f;
}

int vtkGraphItem::VertexMarker(vtkIdType item)
{
  return vtkMarkerUtilities::CIRCLE;
}

vtkColor4ub vtkGraphItem::EdgeColor(vtkIdType edgeIdx, vtkIdType point)
{
  return vtkColor4ub(0, 0, 0, 255);
}

vtkVector2f vtkGraphItem::EdgePosition(vtkIdType edgeIdx, vtkIdType point)
{
  double *p;
  if (point == 0)
    {
    p = this->Graph->GetPoint(this->Graph->GetSourceVertex(edgeIdx));
    }
  else if (point == this->NumberOfEdgePoints(edgeIdx) - 1)
    {
    p = this->Graph->GetPoint(this->Graph->GetTargetVertex(edgeIdx));
    }
  else
    {
    p = this->Graph->GetEdgePoint(edgeIdx, point - 1);
    }
  return vtkVector2f(static_cast<float>(p[0]), static_cast<float>(p[1]));
}

float vtkGraphItem::EdgeWidth(vtkIdType line, vtkIdType point)
{
  return 0.0f;
}

void vtkGraphItem::RebuildBuffers()
{
  vtkIdType numEdges = this->NumberOfEdges();
  this->Internal->EdgePositions = std::vector<std::vector<vtkVector2f> >(numEdges);
  this->Internal->EdgeColors = std::vector<std::vector<vtkColor4ub> >(numEdges);
  this->Internal->EdgeWidths = std::vector<float>(numEdges);
  for (vtkIdType edgeIdx = 0; edgeIdx < numEdges; ++edgeIdx)
    {
    vtkIdType numPoints = this->NumberOfEdgePoints(edgeIdx);
    this->Internal->EdgePositions[edgeIdx] = std::vector<vtkVector2f>(numPoints);
    this->Internal->EdgeColors[edgeIdx] = std::vector<vtkColor4ub>(numPoints);
    this->Internal->EdgeWidths[edgeIdx] = this->EdgeWidth(edgeIdx, 0);
    for (vtkIdType pointIdx = 0; pointIdx < numPoints; ++pointIdx)
      {
      this->Internal->EdgePositions[edgeIdx][pointIdx] = this->EdgePosition(edgeIdx, pointIdx);
      this->Internal->EdgeColors[edgeIdx][pointIdx] = this->EdgeColor(edgeIdx, pointIdx);
      }
    }

  vtkIdType numVertices = this->NumberOfVertices();
  this->Internal->VertexPositions = std::vector<vtkVector2f>(numVertices);
  this->Internal->VertexColors = std::vector<vtkColor4ub>(numVertices);
  this->Internal->VertexSizes = std::vector<float>(numVertices);
  this->Internal->VertexMarkers = std::vector<int>(numVertices);
  vtkMarkerUtilities::GenerateMarker(this->Sprite.GetPointer(), this->VertexMarker(0), this->VertexSize(0));
  for (vtkIdType vertexIdx = 0; vertexIdx < numVertices; ++vertexIdx)
    {
    this->Internal->VertexPositions[vertexIdx] = this->VertexPosition(vertexIdx);
    this->Internal->VertexColors[vertexIdx] = this->VertexColor(vertexIdx);
    this->Internal->VertexSizes[vertexIdx] = this->VertexSize(vertexIdx);
    this->Internal->VertexMarkers[vertexIdx] = this->VertexMarker(vertexIdx);
    }
}

void vtkGraphItem::PaintBuffers(vtkContext2D *painter)
{
  if (this->Internal->EdgePositions.empty())
    {
    return;
    }
  vtkIdType numEdges = this->Internal->EdgePositions.size();
  for (vtkIdType edgeIdx = 0; edgeIdx < numEdges; ++edgeIdx)
    {
    if (this->Internal->EdgePositions[edgeIdx].empty())
      {
      continue;
      }
    painter->GetPen()->SetWidth(this->Internal->EdgeWidths[edgeIdx]);
    painter->DrawPoly(this->Internal->EdgePositions[edgeIdx][0].GetData(),
                      this->Internal->EdgePositions[edgeIdx].size(),
                      this->Internal->EdgeColors[edgeIdx][0].GetData(), 4);
    }

  if (this->Internal->VertexPositions.empty())
    {
    return;
    }
  painter->GetPen()->SetWidth(this->Internal->VertexSizes[0]);
  painter->DrawPointSprites(this->Sprite.GetPointer(),
                            this->Internal->VertexPositions[0].GetData(),
                            this->Internal->VertexPositions.size(),
                            this->Internal->VertexColors[0].GetData(), 4);
}

vtkIdType vtkGraphItem::NumberOfVertices()
{
  if (!this->Graph)
    {
    return 0;
    }
  return this->Graph->GetNumberOfVertices();
}

vtkIdType vtkGraphItem::NumberOfEdges()
{
  if (!this->Graph)
    {
    return 0;
    }
  return this->Graph->GetNumberOfEdges();
}

vtkIdType vtkGraphItem::NumberOfEdgePoints(vtkIdType edgeIdx)
{
  if (!this->Graph)
    {
    return 0;
    }
  return this->Graph->GetNumberOfEdgePoints(edgeIdx) + 2;
}

bool vtkGraphItem::IsDirty()
{
  if (!this->Graph)
    {
    return false;
    }
  if (this->Graph->GetMTime() > this->GraphBuildTime)
    {
    this->GraphBuildTime = this->Graph->GetMTime();
    return true;
    }
  return false;
}

bool vtkGraphItem::Paint(vtkContext2D *painter)
{
  if (this->IsDirty())
    {
    this->RebuildBuffers();
    }
  this->PaintBuffers(painter);
  return true;
}

void vtkGraphItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Graph: " << (this->Graph ? "" : "(null)") << std::endl;
  if (this->Graph)
    {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
    }
  os << "GraphBuildTime: " << this->GraphBuildTime << std::endl;
}
