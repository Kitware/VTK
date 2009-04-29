/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutView.cxx

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

#include "vtkGraphLayoutView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkDirectedGraph.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkGraphLayoutView, "1.61");
vtkStandardNewMacro(vtkGraphLayoutView);
//----------------------------------------------------------------------------
vtkGraphLayoutView::vtkGraphLayoutView()
{
  this->SetInteractionModeTo2D();
  this->SetLabelPlacementModeToDynamic2D();
  this->ReuseSingleRepresentationOn();
}

//----------------------------------------------------------------------------
vtkGraphLayoutView::~vtkGraphLayoutView()
{
}

//----------------------------------------------------------------------------
vtkRenderedGraphRepresentation* vtkGraphLayoutView::GetGraphRepresentation()
{
  vtkRenderedGraphRepresentation* graphRep = 0;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* rep = this->GetRepresentation(i);
    graphRep = vtkRenderedGraphRepresentation::SafeDownCast(rep);
    if (graphRep)
      {
      break;
      }
    }
  if (!graphRep)
    {
    vtkSmartPointer<vtkDirectedGraph> g = vtkSmartPointer<vtkDirectedGraph>::New();
    graphRep = vtkRenderedGraphRepresentation::SafeDownCast(
      this->AddRepresentationFromInput(g));
    }
  return graphRep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkGraphLayoutView::CreateDefaultRepresentation(
  vtkAlgorithmOutput* port)
{
  vtkRenderedGraphRepresentation* rep = vtkRenderedGraphRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexLabelArrayName()
{
  return this->GetGraphRepresentation()->GetVertexLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeLabelArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetVertexLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetVertexLabelVisibility()
{
  return this->GetGraphRepresentation()->GetVertexLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeLabelVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexColorArrayName()
{
  return this->GetGraphRepresentation()->GetVertexColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorVertices(bool vis)
{
  this->GetGraphRepresentation()->SetColorVerticesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorVertices()
{
  return this->GetGraphRepresentation()->GetColorVerticesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetVertexScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetVertexScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetVertexScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeColorArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorEdges(bool vis)
{
  this->GetGraphRepresentation()->SetColorEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorEdges()
{
  return this->GetGraphRepresentation()->GetColorEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnabledEdgesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledEdgesArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEnabledEdgesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledEdgesArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnableEdgesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableEdgesByArray(vis);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEnableEdgesByArray()
{
  return this->GetGraphRepresentation()->GetEnableEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnabledVerticesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledVerticesArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEnabledVerticesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledVerticesArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnableVerticesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableVerticesByArray(vis);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEnableVerticesByArray()
{
  return this->GetGraphRepresentation()->GetEnableVerticesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetScaledGlyphs(bool arg)
{
  this->GetGraphRepresentation()->SetScaling(arg);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetScaledGlyphs()
{
  return this->GetGraphRepresentation()->GetScaling();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetScalingArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetScalingArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetScalingArrayName()
{
  return this->GetGraphRepresentation()->GetScalingArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexIconArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetIconArrayName()
{
  return this->GetGraphRepresentation()->GetVertexIconArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::AddIconType(char *type, int index)
{
  this->GetGraphRepresentation()->AddVertexIconType(type, index);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ClearIconTypes()
{
  this->GetGraphRepresentation()->ClearVertexIconTypes();
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::IsLayoutComplete()
{
  return this->GetGraphRepresentation()->IsLayoutComplete();
}
  
//----------------------------------------------------------------------------
void vtkGraphLayoutView::UpdateLayout()
{
  this->GetGraphRepresentation()->UpdateLayout();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetLayoutStrategy(vtkGraphLayoutStrategy* s)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(s);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(name);
}

//----------------------------------------------------------------------------
vtkGraphLayoutStrategy* vtkGraphLayoutView::GetLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetLayoutStrategy();
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetLayoutStrategyName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy *s)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(s);
}

//----------------------------------------------------------------------------
vtkEdgeLayoutStrategy* vtkGraphLayoutView::GetEdgeLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategy();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategyName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconAlignment(int alignment)
{
  this->GetGraphRepresentation()->SetVertexIconAlignment(alignment);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconVisibility(bool b)
{
  this->GetGraphRepresentation()->SetVertexIconVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetIconVisibility()
{
  return this->GetGraphRepresentation()->GetVertexIconVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  this->GetGraphRepresentation()->SetupRenderWindow(win);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetVertexLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetVertexLabelFontSize()
{
  return this->GetGraphRepresentation()->GetVertexLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEdgeLabelFontSize()
{
  return this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ZoomToSelection()
{
#if 0
  // Bring the graph up to date
  this->GraphLayout->Update();

  // Convert to an index selection
  vtkSmartPointer<vtkConvertSelection> cs = vtkSmartPointer<vtkConvertSelection>::New();
  cs->SetInputConnection(0, this->GetRepresentation()->GetSelectionConnection());
  cs->SetInputConnection(1, this->GraphLayout->GetOutputPort());
  cs->SetOutputType(vtkSelectionNode::INDICES);
  cs->Update();
  vtkGraph* data = vtkGraph::SafeDownCast(this->GraphLayout->GetOutput());
  vtkSelection* converted = cs->GetOutput();

  // Iterate over the selection's nodes, constructing a list of selected vertices.
  // In the case of an edge selection, we add the edges' vertices to vertex list.

  vtkSmartPointer<vtkIdTypeArray> edgeList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasEdges = false;
  vtkSmartPointer<vtkIdTypeArray> vertexList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasVertices = false;
  for( unsigned int m = 0; m < static_cast<vtkIdType>(converted->GetNumberOfNodes()); ++m)
    {
    vtkSelectionNode* node = converted->GetNode(m);
    vtkIdTypeArray* list = 0;
    if (node->GetFieldType() == vtkSelectionNode::VERTEX)
      {
      list = vertexList;
      hasVertices = true;
      }
    else if (node->GetFieldType() == vtkSelectionNode::EDGE)
      {
      list = edgeList;
      hasEdges = true;
      }

    if (list)
      {
      // Append the selection list to the selection
      vtkIdTypeArray* curList = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
      if (curList)
        {
        int inverse = node->GetProperties()->Get(vtkSelectionNode::INVERSE());
        if (inverse)
          {
          vtkIdType num =
            (node->GetFieldType() == vtkSelectionNode::VERTEX) ?
            data->GetNumberOfVertices() : data->GetNumberOfEdges();
          for (vtkIdType j = 0; j < num; ++j)
            {
            if (curList->LookupValue(j) < 0 && list->LookupValue(j) < 0)
              {
              list->InsertNextValue(j);
              }
            }
          }
        else
          {
          vtkIdType numTuples = curList->GetNumberOfTuples();
          for (vtkIdType j = 0; j < numTuples; ++j)
            {
            vtkIdType curValue = curList->GetValue(j);
            if (list->LookupValue(curValue) < 0)
              {
              list->InsertNextValue(curValue);
              }
            }
          }
        } // end if (curList)
      } // end if (list)
    } // end for each child

  vtkIdType i;
  if(hasEdges)
    {
    vtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
    for( i = 0; i < numSelectedEdges; ++i)
      {
      vtkIdType eid = edgeList->GetValue(i);
      vertexList->InsertNextValue(data->GetSourceVertex(eid));
      vertexList->InsertNextValue(data->GetTargetVertex(eid));
      }
    }
  
  // If there is no selection list, return 
  if (vertexList->GetNumberOfTuples() == 0)
    {
    return;
    }

  // Now we use our list of vertices to get the point coordinates
  // of the selection and use that to initialize the bounds that
  // we'll use to reset the camera.

  double bounds[6];
  double position[3];
  data->GetPoint(vertexList->GetValue(0), position);
  bounds[0] = bounds[1] = position[0];
  bounds[2] = bounds[3] = position[1];
  bounds[4] = -0.1;
  bounds[5] = 0.1;
  for (i = 1; i < vertexList->GetNumberOfTuples(); ++i)
    {
    data->GetPoint(vertexList->GetValue(i), position);

    if (position[0] < bounds[0])
      {
      bounds[0] = position[0]; 
      }
    if (position[0] > bounds[1])
      {
      bounds[1] = position[0]; 
      }
    if (position[1] < bounds[2])
      {
      bounds[2] = position[1]; 
      }
    if (position[1] > bounds[3])
      {
      bounds[3] = position[1]; 
      }
    }

  this->Renderer->ResetCamera(bounds);
#endif
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

