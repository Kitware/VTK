/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedGraphRepresentation.cxx

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

#include "vtkRenderedGraphRepresentation.h"

#include "vtkAbstractTransform.h"
#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAnnotationLink.h"
#include "vtkApplyColors.h"
#include "vtkApplyIcons.h"
#include "vtkArcParallelEdgeStrategy.h"
#include "vtkAssignCoordinatesLayoutStrategy.h"
#include "vtkCellData.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkClustering2DLayoutStrategy.h"
#include "vtkCommunity2DLayoutStrategy.h"
#include "vtkConeLayoutStrategy.h"
#include "vtkConvertSelection.h"
#include "vtkCosmicTreeLayoutStrategy.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeLayout.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGeoEdgeStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToGlyphs.h"
#include "vtkGraphToPoints.h"
#include "vtkGraphToPolyData.h"
#include "vtkIconGlyphFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPassThroughEdgeStrategy.h"
#include "vtkPassThroughLayoutStrategy.h"
#include "vtkPerturbCoincidentVertices.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty.h"
#include "vtkRandomLayoutStrategy.h"
#include "vtkRemoveHiddenData.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkSpanTreeLayoutStrategy.h"
#include "vtkSphereSource.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkTexturedActor2D.h"
#include "vtkTransformCoordinateSystems.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"

#include <ctype.h> // So borland 5.6 can find tolower
#include <vtksys/ios/sstream>

/* Fix for BORLAND 5.6 bug where it wrongly chooses remove(const char *) in stdio 
   instead of the remove stl algorithm. */
#if defined (__BORLANDC__) && (__BORLANDC__ == 0x0560)
# define remove borland_remove
#endif
/* Include algorithm last so "remove" macro Borland hack does not
   affect other headers.  */
#include <vtksys/stl/algorithm>




vtkStandardNewMacro(vtkRenderedGraphRepresentation);

vtkRenderedGraphRepresentation::vtkRenderedGraphRepresentation()
{
  this->ApplyColors         = vtkSmartPointer<vtkApplyColors>::New();
  this->VertexDegree        = vtkSmartPointer<vtkVertexDegree>::New();
  this->EmptyPolyData       = vtkSmartPointer<vtkPolyData>::New();
  this->EdgeCenters         = vtkSmartPointer<vtkEdgeCenters>::New();
  this->GraphToPoints       = vtkSmartPointer<vtkGraphToPoints>::New();
  this->VertexLabelHierarchy = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->EdgeLabelHierarchy  = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->Layout              = vtkSmartPointer<vtkGraphLayout>::New();
  this->Coincident          = vtkSmartPointer<vtkPerturbCoincidentVertices>::New();
  this->EdgeLayout          = vtkSmartPointer<vtkEdgeLayout>::New();
  this->GraphToPoly         = vtkSmartPointer<vtkGraphToPolyData>::New();
  this->EdgeMapper          = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->EdgeActor           = vtkSmartPointer<vtkActor>::New();
  this->VertexGlyph         = vtkSmartPointer<vtkGraphToGlyphs>::New();
  this->VertexMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->VertexActor         = vtkSmartPointer<vtkActor>::New();
  this->OutlineGlyph        = vtkSmartPointer<vtkGraphToGlyphs>::New();
  this->OutlineMapper       = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->OutlineActor        = vtkSmartPointer<vtkActor>::New();
  this->VertexScalarBar     = vtkSmartPointer<vtkScalarBarWidget>::New();
  this->EdgeScalarBar       = vtkSmartPointer<vtkScalarBarWidget>::New();
  this->RemoveHiddenGraph   = vtkSmartPointer<vtkRemoveHiddenData>::New();
  this->ApplyVertexIcons    = vtkSmartPointer<vtkApplyIcons>::New();
  this->VertexIconPoints    = vtkSmartPointer<vtkGraphToPoints>::New();
  this->VertexIconTransform = vtkSmartPointer<vtkTransformCoordinateSystems>::New();
  this->VertexIconGlyph     = vtkSmartPointer<vtkIconGlyphFilter>::New();
  this->VertexIconMapper    = vtkSmartPointer<vtkPolyDataMapper2D>::New();
  this->VertexIconActor     = vtkSmartPointer<vtkTexturedActor2D>::New();

  this->VertexHoverArrayName = 0;
  this->EdgeHoverArrayName = 0;
  this->VertexColorArrayNameInternal = 0;
  this->EdgeColorArrayNameInternal = 0;
  this->ScalingArrayNameInternal = 0;
  this->LayoutStrategyName = 0;
  this->EdgeLayoutStrategyName = 0;

  this->HideVertexLabelsOnInteraction = false;
  this->HideEdgeLabelsOnInteraction = false;

  /*
   <graphviz>
   digraph {
     Layout -> Coincident -> EdgeLayout -> VertexDegree -> ApplyColors
     ApplyColors -> VertexGlyph -> VertexMapper -> VertexActor
     ApplyColors -> GraphToPoly -> EdgeMapper -> EdgeActor
     ApplyColors -> ApplyVertexIcons
     Coincident -> OutlineGlyph -> OutlineMapper -> OutlineActor
     
     VertexDegree -> GraphToPoints
     GraphToPoints -> VertexLabelHierarchy -> "vtkRenderView Labels"
     GraphToPoints -> VertexIcons -> VertexIconPriority -> "vtkRenderView Icons"
     ApplyVertexIcons -> VertexIconPoints -> VertexIconTransform -> VertexIconGlyphFilter -> VertexIconMapper -> VertexIconActor
     VertexDegree -> EdgeCenters
     EdgeCenters -> EdgeLabelHierarchy -> "vtkRenderView Labels"
     EdgeCenters -> EdgeIcons -> EdgeIconPriority -> "vtkRenderView Icons"
   }
   </graphviz>
  */

  this->Coincident->SetInputConnection(this->Layout->GetOutputPort());
  this->RemoveHiddenGraph->SetInputConnection(this->Coincident->GetOutputPort());
  this->EdgeLayout->SetInputConnection(this->RemoveHiddenGraph->GetOutputPort());
  this->VertexDegree->SetInputConnection(this->EdgeLayout->GetOutputPort());
  this->ApplyColors->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->ApplyVertexIcons->SetInputConnection(this->ApplyColors->GetOutputPort());

  // Vertex actor
  this->VertexGlyph->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);

  // Outline actor
  this->OutlineGlyph->SetInputConnection(this->RemoveHiddenGraph->GetOutputPort());
  this->OutlineMapper->SetInputConnection(this->OutlineGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);

  // Edge actor
  this->GraphToPoly->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->EdgeMapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);

  // Experimental icons
  this->VertexIconPoints->SetInputConnection(this->ApplyVertexIcons->GetOutputPort());
  this->VertexIconTransform->SetInputConnection(this->VertexIconPoints->GetOutputPort());
  this->VertexIconGlyph->SetInputConnection(this->VertexIconTransform->GetOutputPort());
  this->VertexIconMapper->SetInputConnection(this->VertexIconGlyph->GetOutputPort());
  this->VertexIconActor->SetMapper(this->VertexIconMapper);
  this->VertexIconTransform->SetInputCoordinateSystemToWorld();
  this->VertexIconTransform->SetOutputCoordinateSystemToDisplay();
  this->VertexIconGlyph->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "vtkApplyIcons icon");
  this->ApplyVertexIcons->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "icon");
  this->VertexIconActor->VisibilityOff();

  this->GraphToPoints->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeCenters->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeLabelHierarchy->SetInput(this->EmptyPolyData);
  this->VertexLabelHierarchy->SetInput(this->EmptyPolyData);

  // Set default parameters
  vtkSmartPointer<vtkDirectedGraph> g =
    vtkSmartPointer<vtkDirectedGraph>::New();
  this->Layout->SetInput(g);
  vtkSmartPointer<vtkFast2DLayoutStrategy> strategy =
    vtkSmartPointer<vtkFast2DLayoutStrategy>::New();
  this->Layout->SetLayoutStrategy(strategy);
  //this->Layout->SetZRange(0.001);
  this->Layout->SetZRange(0.0);
  vtkSmartPointer<vtkArcParallelEdgeStrategy> edgeStrategy =
    vtkSmartPointer<vtkArcParallelEdgeStrategy>::New();
  this->Layout->UseTransformOn();
  this->SetVertexColorArrayName("VertexDegree");
  this->SetVertexLabelArrayName("VertexDegree");
  this->SetVertexLabelPriorityArrayName("VertexDegree");
  this->SetVertexIconArrayName("IconIndex");
  this->SetVertexIconPriorityArrayName("VertexDegree");
  this->EdgeLayout->SetLayoutStrategy(edgeStrategy);

  this->VertexGlyph->FilledOn();
  this->VertexGlyph->SetGlyphType(vtkGraphToGlyphs::VERTEX);
  this->VertexMapper->SetScalarModeToUseCellFieldData();
  this->VertexMapper->SelectColorArray("vtkApplyColors color");
  this->VertexMapper->SetScalarVisibility(true);

  this->OutlineGlyph->FilledOff();
  this->OutlineGlyph->SetGlyphType(vtkGraphToGlyphs::VERTEX);
  this->OutlineMapper->SetScalarVisibility(false);
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->FrontfaceCullingOn();

  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray("vtkApplyColors color");
  this->EdgeMapper->SetScalarVisibility(true);
  this->EdgeActor->SetPosition(0, 0, -0.003);

  this->VertexScalarBar->GetScalarBarActor()->VisibilityOff();
  this->EdgeScalarBar->GetScalarBarActor()->VisibilityOff();

  vtkSmartPointer<vtkViewTheme> theme =
    vtkSmartPointer<vtkViewTheme>::New();
  this->ApplyViewTheme(theme);
}

vtkRenderedGraphRepresentation::~vtkRenderedGraphRepresentation()
{
  this->SetScalingArrayNameInternal(0);
  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
  this->SetLayoutStrategyName(0);
  this->SetEdgeLayoutStrategyName(0);
  this->SetVertexHoverArrayName(0);
  this->SetEdgeHoverArrayName(0);
}

void vtkRenderedGraphRepresentation::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabelHierarchy->SetLabelArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelHierarchy->SetLabelArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexLabelArrayName()
{
  return this->VertexLabelHierarchy->GetLabelArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeLabelArrayName()
{
  return this->EdgeLabelHierarchy->GetLabelArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexLabelPriorityArrayName(const char* name)
{
  this->VertexLabelHierarchy->SetPriorityArrayName(name);
}

void vtkRenderedGraphRepresentation::SetEdgeLabelPriorityArrayName(const char* name)
{
  this->EdgeLabelHierarchy->SetPriorityArrayName(name);
}

const char* vtkRenderedGraphRepresentation::GetVertexLabelPriorityArrayName()
{
  return this->VertexLabelHierarchy->GetPriorityArrayName();
}

const char* vtkRenderedGraphRepresentation::GetEdgeLabelPriorityArrayName()
{
  return this->EdgeLabelHierarchy->GetPriorityArrayName();
}

void vtkRenderedGraphRepresentation::SetVertexLabelVisibility(bool b)
{
  if (b)
    {
    this->VertexLabelHierarchy->SetInputConnection(this->GraphToPoints->GetOutputPort());
    }
  else
    {
    this->VertexLabelHierarchy->SetInput(this->EmptyPolyData);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeLabelVisibility(bool b)
{
  if (b)
    {
    this->EdgeLabelHierarchy->SetInputConnection(this->EdgeCenters->GetOutputPort());
    }
  else
    {
    this->EdgeLabelHierarchy->SetInput(this->EmptyPolyData);
    }
}

bool vtkRenderedGraphRepresentation::GetVertexLabelVisibility()
{
  return this->VertexLabelHierarchy->GetInputConnection(0, 0) ==
         this->GraphToPoints->GetOutputPort();
}

bool vtkRenderedGraphRepresentation::GetEdgeLabelVisibility()
{
  return this->EdgeLabelHierarchy->GetInputConnection(0, 0) ==
         this->EdgeCenters->GetOutputPort();
}

void vtkRenderedGraphRepresentation::SetEdgeVisibility(bool b)
{
  this->EdgeActor->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetEdgeVisibility()
{
  return this->EdgeActor->GetVisibility() ? true : false;
}

void vtkRenderedGraphRepresentation::SetVertexLabelTextProperty(vtkTextProperty* p)
{
  this->VertexLabelHierarchy->SetTextProperty(p);
}

void vtkRenderedGraphRepresentation::SetEdgeLabelTextProperty(vtkTextProperty* p)
{
  this->EdgeLabelHierarchy->SetTextProperty(p);
}

vtkTextProperty* vtkRenderedGraphRepresentation::GetVertexLabelTextProperty()
{
  return this->VertexLabelHierarchy->GetTextProperty();
}

vtkTextProperty* vtkRenderedGraphRepresentation::GetEdgeLabelTextProperty()
{
  return this->EdgeLabelHierarchy->GetTextProperty();
}

void vtkRenderedGraphRepresentation::SetVertexIconArrayName(const char* name)
{
  this->ApplyVertexIcons->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

void vtkRenderedGraphRepresentation::SetEdgeIconArrayName(const char*)
{
  // TODO: Implement.
}

const char* vtkRenderedGraphRepresentation::GetVertexIconArrayName()
{
  // TODO: Implement.
  return 0;
}

const char* vtkRenderedGraphRepresentation::GetEdgeIconArrayName()
{
  // TODO: Implement.
  return 0;
}

void vtkRenderedGraphRepresentation::SetVertexIconPriorityArrayName(const char*)
{
  // TODO: Implement.
}

void vtkRenderedGraphRepresentation::SetEdgeIconPriorityArrayName(const char*)
{
  // TODO: Implement.
}

const char* vtkRenderedGraphRepresentation::GetVertexIconPriorityArrayName()
{
  // TODO: Implement.
  return 0;
}

const char* vtkRenderedGraphRepresentation::GetEdgeIconPriorityArrayName()
{
  // TODO: Implement.
  return 0;
}

void vtkRenderedGraphRepresentation::SetVertexIconVisibility(bool b)
{
  this->VertexIconActor->SetVisibility(b);
}

void vtkRenderedGraphRepresentation::SetEdgeIconVisibility(bool)
{
  // TODO: Implement.
}

bool vtkRenderedGraphRepresentation::GetVertexIconVisibility()
{
  return (this->VertexIconActor->GetVisibility() ? true : false);
}

bool vtkRenderedGraphRepresentation::GetEdgeIconVisibility()
{
  // TODO: Implement.
  return false;
}

void vtkRenderedGraphRepresentation::AddVertexIconType(const char* name, int type)
{
  this->ApplyVertexIcons->SetIconType(name, type);
  this->ApplyVertexIcons->UseLookupTableOn();
}  

void vtkRenderedGraphRepresentation::AddEdgeIconType(const char*, int)
{
  // TODO: Implement.
}

void vtkRenderedGraphRepresentation::ClearVertexIconTypes()
{
  this->ApplyVertexIcons->ClearAllIconTypes();
  this->ApplyVertexIcons->UseLookupTableOff();
}

void vtkRenderedGraphRepresentation::ClearEdgeIconTypes()
{
  // TODO: Implement.
}

void vtkRenderedGraphRepresentation::SetUseVertexIconTypeMap(bool b)
{
  this->ApplyVertexIcons->SetUseLookupTable(b);
}

void vtkRenderedGraphRepresentation::SetUseEdgeIconTypeMap(bool)
{
  // TODO: Implement.
}

bool vtkRenderedGraphRepresentation::GetUseVertexIconTypeMap()
{
  return this->ApplyVertexIcons->GetUseLookupTable();
}

bool vtkRenderedGraphRepresentation::GetUseEdgeIconTypeMap()
{
  // TODO: Implement.
  return false;
}

// TODO: Icon alignment
void vtkRenderedGraphRepresentation::SetVertexIconAlignment(int align)
{
  (void)align;
}

int vtkRenderedGraphRepresentation::GetVertexIconAlignment()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetEdgeIconAlignment(int align)
{
  (void)align;
}

int vtkRenderedGraphRepresentation::GetEdgeIconAlignment()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetVertexSelectedIcon(int icon)
{
  this->ApplyVertexIcons->SetSelectedIcon(icon);
}

int vtkRenderedGraphRepresentation::GetVertexSelectedIcon()
{
  return this->ApplyVertexIcons->GetSelectedIcon();
}

void vtkRenderedGraphRepresentation::SetVertexIconSelectionMode(int mode)
{
  this->ApplyVertexIcons->SetSelectionMode(mode);
}

int vtkRenderedGraphRepresentation::GetVertexIconSelectionMode()
{
  return this->ApplyVertexIcons->GetSelectionMode();
}

void vtkRenderedGraphRepresentation::SetColorVerticesByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool vtkRenderedGraphRepresentation::GetColorVerticesByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void vtkRenderedGraphRepresentation::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

const char* vtkRenderedGraphRepresentation::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

void vtkRenderedGraphRepresentation::SetColorEdgesByArray(bool b)
{
  this->ApplyColors->SetUseCellLookupTable(b);
}

bool vtkRenderedGraphRepresentation::GetColorEdgesByArray()
{
  return this->ApplyColors->GetUseCellLookupTable();
}

void vtkRenderedGraphRepresentation::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(1, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_EDGES, name);
}

const char* vtkRenderedGraphRepresentation::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

// TODO: Implement enable stuff.
void vtkRenderedGraphRepresentation::SetEnableVerticesByArray(bool b)
{
  (void)b;
}

bool vtkRenderedGraphRepresentation::GetEnableVerticesByArray()
{
  return false;
}

void vtkRenderedGraphRepresentation::SetEnabledVerticesArrayName(const char* name)
{
  (void)name;
}

const char* vtkRenderedGraphRepresentation::GetEnabledVerticesArrayName()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetEnableEdgesByArray(bool b)
{
  (void)b;
}

bool vtkRenderedGraphRepresentation::GetEnableEdgesByArray()
{
  return false;
}

void vtkRenderedGraphRepresentation::SetEnabledEdgesArrayName(const char* name)
{
  (void)name;
}

const char* vtkRenderedGraphRepresentation::GetEnabledEdgesArrayName()
{
  return 0;
}

void vtkRenderedGraphRepresentation::SetGlyphType(int type)
{
  if (type != this->VertexGlyph->GetGlyphType())
    {
    this->VertexGlyph->SetGlyphType(type);
    this->OutlineGlyph->SetGlyphType(type);
    if (type == vtkGraphToGlyphs::SPHERE)
      {
      this->OutlineActor->GetProperty()->FrontfaceCullingOn();
      }
    else
      {
      this->OutlineActor->GetProperty()->FrontfaceCullingOff();
      }
    }
}

int vtkRenderedGraphRepresentation::GetGlyphType()
{
  return this->VertexGlyph->GetGlyphType();
}

void vtkRenderedGraphRepresentation::SetScaling(bool b)
{
  this->VertexGlyph->SetScaling(b);
  this->OutlineGlyph->SetScaling(b);
}

bool vtkRenderedGraphRepresentation::GetScaling()
{
  return this->VertexGlyph->GetScaling();
}

void vtkRenderedGraphRepresentation::SetScalingArrayName(const char* name)
{
  this->VertexGlyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->OutlineGlyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->SetScalingArrayNameInternal(name);
}

const char* vtkRenderedGraphRepresentation::GetScalingArrayName()
{
  return this->GetScalingArrayNameInternal();
}

void vtkRenderedGraphRepresentation::SetVertexScalarBarVisibility(bool b)
{
  this->VertexScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetVertexScalarBarVisibility()
{
  return this->VertexScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

void vtkRenderedGraphRepresentation::SetEdgeScalarBarVisibility(bool b)
{
  this->EdgeScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool vtkRenderedGraphRepresentation::GetEdgeScalarBarVisibility()
{
  return this->EdgeScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

bool vtkRenderedGraphRepresentation::IsLayoutComplete()
{
  return this->Layout->IsLayoutComplete() ? true : false;
}

void vtkRenderedGraphRepresentation::UpdateLayout()
{
  if (!this->IsLayoutComplete())
    {
    this->Layout->Modified();
    // TODO : Should render here??
    }
}

void vtkRenderedGraphRepresentation::SetLayoutStrategy(vtkGraphLayoutStrategy* s)
{
  if (!s)
    {
    vtkErrorMacro("Layout strategy must not be NULL.");
    return;
    }
  if (vtkRandomLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Random");
    }
  else if (vtkForceDirectedLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Force Directed");
    }
  else if (vtkSimple2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Simple 2D");
    }
  else if (vtkClustering2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Clustering 2D");
    }
  else if (vtkCommunity2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Community 2D");
    }
  else if (vtkFast2DLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Fast 2D");
    }
  else if (vtkCircularLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Circular");
    }
  else if (vtkTreeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Tree");
    }
  else if (vtkCosmicTreeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Cosmic Tree");
    }
  else if (vtkPassThroughLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Pass Through");
    }
  else if (vtkConeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Cone");
    }
  else if (vtkSpanTreeLayoutStrategy::SafeDownCast(s))
    {
    this->SetLayoutStrategyName("Span Tree");
    }
  else
    {
    this->SetLayoutStrategyName("Unknown");
    }
  this->Layout->SetLayoutStrategy(s);
}

vtkGraphLayoutStrategy* vtkRenderedGraphRepresentation::GetLayoutStrategy()
{
  return this->Layout->GetLayoutStrategy();
}

void vtkRenderedGraphRepresentation::SetLayoutStrategy(const char* name)
{
  vtkstd::string str = name;
  vtksys_stl::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(vtkstd::remove(str.begin(), str.end(), ' '), str.end());
  vtkSmartPointer<vtkGraphLayoutStrategy> strategy =
    vtkSmartPointer<vtkPassThroughLayoutStrategy>::New();
  if (str == "random")
    {
    strategy = vtkSmartPointer<vtkRandomLayoutStrategy>::New();
    }
  else if (str == "forcedirected")
    {
    strategy = vtkSmartPointer<vtkForceDirectedLayoutStrategy>::New();
    }
  else if (str == "simple2d")
    {
    strategy = vtkSmartPointer<vtkSimple2DLayoutStrategy>::New();
    }
  else if (str == "clustering2d")
    {
    strategy = vtkSmartPointer<vtkClustering2DLayoutStrategy>::New();
    }
  else if (str == "community2d")
    {
    strategy = vtkSmartPointer<vtkCommunity2DLayoutStrategy>::New();
    }
  else if (str == "fast2d")
    {
    strategy = vtkSmartPointer<vtkFast2DLayoutStrategy>::New();
    }
  else if (str == "circular")
    {
    strategy = vtkSmartPointer<vtkCircularLayoutStrategy>::New();
    }
  else if (str == "tree")
    {
    strategy = vtkSmartPointer<vtkTreeLayoutStrategy>::New();
    }
  else if (str == "cosmictree")
    {
    strategy = vtkSmartPointer<vtkCosmicTreeLayoutStrategy>::New();
    }
  else if (str == "cone")
    {
    strategy = vtkSmartPointer<vtkConeLayoutStrategy>::New();
    }
  else if (str == "spantree")
    {
    strategy = vtkSmartPointer<vtkSpanTreeLayoutStrategy>::New();
    }
  else if (str != "passthrough")
    {
    vtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
    }
  vtkstd::string type1 = strategy->GetClassName();
  vtkstd::string type2 = this->GetLayoutStrategy()->GetClassName();
  if (type1 != type2)
    {
    this->SetLayoutStrategy(strategy);
    }
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToAssignCoordinates(
  const char* xarr,
  const char* yarr,
  const char* zarr)
{
  vtkAssignCoordinatesLayoutStrategy* s =
    vtkAssignCoordinatesLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkAssignCoordinatesLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetXCoordArrayName(xarr);
  s->SetYCoordArrayName(yarr);
  s->SetZCoordArrayName(zarr);
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToTree(
  bool radial,
  double angle,
  double leafSpacing,
  double logSpacing)
{
  vtkTreeLayoutStrategy* s =
    vtkTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetRadial(radial);
  s->SetAngle(angle);
  s->SetLeafSpacing(leafSpacing);
  s->SetLogSpacingValue(logSpacing);
}

void vtkRenderedGraphRepresentation::SetLayoutStrategyToCosmicTree(
  const char* nodeSizeArrayName,
  bool sizeLeafNodesOnly,
  int layoutDepth,
  vtkIdType layoutRoot)
{
  vtkCosmicTreeLayoutStrategy* s =
    vtkCosmicTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkCosmicTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
    }
  s->SetNodeSizeArrayName(nodeSizeArrayName);
  s->SetSizeLeafNodesOnly(sizeLeafNodesOnly);
  s->SetLayoutDepth(layoutDepth);
  s->SetLayoutRoot(layoutRoot);
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* s)
{
  if (!s)
    {
    vtkErrorMacro("Layout strategy must not be NULL.");
    return;
    }
  if (vtkArcParallelEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Arc Parallel");
    }
  else if (vtkGeoEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Geo");
    }
  else if (vtkPassThroughEdgeStrategy::SafeDownCast(s))
    {
    this->SetEdgeLayoutStrategyName("Pass Through");
    }
  else
    {
    this->SetEdgeLayoutStrategyName("Unknown");
    }
  this->EdgeLayout->SetLayoutStrategy(s);
}

vtkEdgeLayoutStrategy* vtkRenderedGraphRepresentation::GetEdgeLayoutStrategy()
{
  return this->EdgeLayout->GetLayoutStrategy();
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(const char* name)
{
  vtkstd::string str = name;
  vtkstd::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(vtkstd::remove(str.begin(), str.end(), ' '), str.end());
  vtkSmartPointer<vtkEdgeLayoutStrategy> strategy =
    vtkSmartPointer<vtkPassThroughEdgeStrategy>::New();
  if (str == "arcparallel")
    {
    strategy = vtkSmartPointer<vtkArcParallelEdgeStrategy>::New();
    }
  else if (str == "geo")
    {
    strategy = vtkSmartPointer<vtkGeoEdgeStrategy>::New();
    }
  else if (str != "passthrough")
    {
    vtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
    }
  vtkstd::string type1 = strategy->GetClassName();
  vtkstd::string type2 = this->GetEdgeLayoutStrategy()->GetClassName();
  if (type1 != type2)
    {
    this->SetEdgeLayoutStrategy(strategy);
    }
}

void vtkRenderedGraphRepresentation::SetEdgeLayoutStrategyToGeo(double explodeFactor)
{
  vtkGeoEdgeStrategy* s =
    vtkGeoEdgeStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
    {
    s = vtkGeoEdgeStrategy::New();
    this->SetEdgeLayoutStrategy(s);
    s->Delete();
    }
  s->SetExplodeFactor(explodeFactor);
}

bool vtkRenderedGraphRepresentation::AddToView(vtkView* view)
{
  this->Superclass::AddToView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    this->VertexScalarBar->SetInteractor(rv->GetRenderWindow()->GetInteractor());
    this->EdgeScalarBar->SetInteractor(rv->GetRenderWindow()->GetInteractor());
    this->VertexGlyph->SetRenderer(rv->GetRenderer());
    this->OutlineGlyph->SetRenderer(rv->GetRenderer());
    this->VertexIconTransform->SetViewport(rv->GetRenderer());
    rv->GetRenderer()->AddActor(this->OutlineActor);
    rv->GetRenderer()->AddActor(this->VertexActor);
    rv->GetRenderer()->AddActor(this->EdgeActor);
    rv->GetRenderer()->AddActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->AddActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->GetRenderer()->AddActor(this->VertexIconActor);
    rv->AddLabels(this->VertexLabelHierarchy->GetOutputPort());
    rv->AddLabels(this->EdgeLabelHierarchy->GetOutputPort());
    //rv->AddIcons(this->VertexIconPriority->GetOutputPort());
    //rv->AddIcons(this->EdgeIconPriority->GetOutputPort());
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeCenters);
    rv->RegisterProgress(this->GraphToPoints);
    rv->RegisterProgress(this->VertexLabelHierarchy);
    rv->RegisterProgress(this->EdgeLabelHierarchy);
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeLayout);
    rv->RegisterProgress(this->GraphToPoly);
    rv->RegisterProgress(this->EdgeMapper);
    rv->RegisterProgress(this->VertexGlyph);
    rv->RegisterProgress(this->VertexMapper);
    rv->RegisterProgress(this->OutlineGlyph);
    rv->RegisterProgress(this->OutlineMapper);
    return true;
    }
  return false;
}

bool vtkRenderedGraphRepresentation::RemoveFromView(vtkView* view)
{
  this->Superclass::RemoveFromView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    this->VertexGlyph->SetRenderer(0);
    this->OutlineGlyph->SetRenderer(0);
    rv->GetRenderer()->RemoveActor(this->VertexActor);
    rv->GetRenderer()->RemoveActor(this->OutlineActor);
    rv->GetRenderer()->RemoveActor(this->EdgeActor);
    rv->GetRenderer()->RemoveActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->RemoveActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->GetRenderer()->RemoveActor(this->VertexIconActor);
    rv->RemoveLabels(this->VertexLabelHierarchy->GetOutputPort());
    rv->RemoveLabels(this->EdgeLabelHierarchy->GetOutputPort());
    //rv->RemoveIcons(this->VertexIcons->GetOutputPort());
    //rv->RemoveIcons(this->EdgeIcons->GetOutputPort());
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeCenters);
    rv->UnRegisterProgress(this->GraphToPoints);
    rv->UnRegisterProgress(this->VertexLabelHierarchy);
    rv->UnRegisterProgress(this->EdgeLabelHierarchy);
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeLayout);
    rv->UnRegisterProgress(this->GraphToPoly);
    rv->UnRegisterProgress(this->EdgeMapper);
    rv->UnRegisterProgress(this->VertexGlyph);
    rv->UnRegisterProgress(this->VertexMapper);
    rv->UnRegisterProgress(this->OutlineGlyph);
    rv->UnRegisterProgress(this->OutlineMapper);
    return true;
    }
  return false;
}

void vtkRenderedGraphRepresentation::PrepareForRendering(vtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);

  this->VertexIconActor->SetTexture(view->GetIconTexture());
  if (this->VertexIconActor->GetTexture() &&
      this->VertexIconActor->GetTexture()->GetInput())
    {
    this->VertexIconGlyph->SetIconSize(view->GetIconSize());
    this->VertexIconGlyph->SetUseIconSize(true);
    this->VertexIconActor->GetTexture()->MapColorScalarsThroughLookupTableOff();
    this->VertexIconActor->GetTexture()->GetInput()->Update();
    int* dim = this->VertexIconActor->GetTexture()->GetInput()->GetDimensions();
    this->VertexIconGlyph->SetIconSheetSize(dim);
    }

  // Make sure the transform is synchronized between rep and view
  this->Layout->SetTransform(view->GetTransform());
}

vtkSelection* vtkRenderedGraphRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view), vtkSelection* sel)
{
  // Search for selection nodes relating to the vertex and edges
  // of the graph.
  vtkSmartPointer<vtkSelectionNode> vertexNode =
    vtkSmartPointer<vtkSelectionNode>::New();
  vtkSmartPointer<vtkSelectionNode> edgeNode =
    vtkSmartPointer<vtkSelectionNode>::New();
  bool foundEdgeNode = false;

  if (sel->GetNumberOfNodes() > 0)
    {
    for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
      {
      vtkSelectionNode* node = sel->GetNode(i);
      vtkProp* prop = vtkProp::SafeDownCast(
        node->GetProperties()->Get(vtkSelectionNode::PROP()));
      if (node->GetContentType() == vtkSelectionNode::FRUSTUM)
        {
        // A frustum selection can be used to select vertices and edges.
        vertexNode->ShallowCopy(node);
        edgeNode->ShallowCopy(node);
        foundEdgeNode = true;
        }
      else if (prop == this->VertexActor.GetPointer())
        {
        // The prop on the selection matches the vertex actor, so
        // this must have been a visible cell selection.
        vertexNode->ShallowCopy(node);
        }
      else if (prop == this->EdgeActor.GetPointer())
        {
        // The prop on the selection matches the edge actor, so
        // this must have been a visible cell selection.
        edgeNode->ShallowCopy(node);
        foundEdgeNode = true;
        }
      }
    }

  // Remove the prop to avoid reference loops.
  vertexNode->GetProperties()->Remove(vtkSelectionNode::PROP());
  edgeNode->GetProperties()->Remove(vtkSelectionNode::PROP());

  vtkSelection* converted = vtkSelection::New();
  vtkGraph* input = vtkGraph::SafeDownCast(this->GetInput());
  if (!input)
    {
    return converted;
    }

  bool selectedVerticesFound = false;
  if (vertexNode)
    {
    // Convert a cell selection on the glyphed vertices into a
    // vertex selection on the graph of the appropriate type.

    // First, convert the cell selection on the polydata to
    // a pedigree ID selection (or index selection if there are no
    // pedigree IDs).
    vtkSmartPointer<vtkSelection> vertexSel =
      vtkSmartPointer<vtkSelection>::New();
    vertexSel->AddNode(vertexNode);

    vtkPolyData* poly = vtkPolyData::SafeDownCast(
      this->VertexGlyph->GetOutput());
    vtkSmartPointer<vtkTable> temp =
      vtkSmartPointer<vtkTable>::New();
    temp->SetRowData(vtkPolyData::SafeDownCast(poly)->GetCellData());
    vtkSelection* polyConverted = 0;
    if (poly->GetCellData()->GetPedigreeIds())
      {
      polyConverted = vtkConvertSelection::ToSelectionType(
        vertexSel, poly, vtkSelectionNode::PEDIGREEIDS);
      }
    else
      {
      polyConverted = vtkConvertSelection::ToSelectionType(
        vertexSel, poly, vtkSelectionNode::INDICES);
      }

    // Now that we have a pedigree or index selection, interpret this
    // as a vertex selection on the graph, and convert it to the
    // appropriate selection type for this representation.
    for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
      {
      polyConverted->GetNode(i)->SetFieldType(vtkSelectionNode::VERTEX);
      }
    vtkSelection* vertexConverted = vtkConvertSelection::ToSelectionType(
      polyConverted, input, this->SelectionType, this->SelectionArrayNames);

    // For all output selection nodes, select all the edges among selected vertices.
    for (unsigned int i = 0; i < vertexConverted->GetNumberOfNodes(); ++i)
      {
      if (vertexConverted->GetNode(i)->GetSelectionList()->
          GetNumberOfTuples() > 0)
        {
        // Get the list of selected vertices.
        selectedVerticesFound = true;
        vtkSmartPointer<vtkIdTypeArray> selectedVerts =
          vtkSmartPointer<vtkIdTypeArray>::New();
        vtkConvertSelection::GetSelectedVertices(
          vertexConverted, input, selectedVerts);

        // Get the list of induced edges on these vertices.
        vtkSmartPointer<vtkIdTypeArray> selectedEdges =
          vtkSmartPointer<vtkIdTypeArray>::New();
        input->GetInducedEdges(selectedVerts, selectedEdges);

        // Create an edge index selection containing the induced edges.
        vtkSmartPointer<vtkSelection> edgeSelection =
          vtkSmartPointer<vtkSelection>::New();
        vtkSmartPointer<vtkSelectionNode> edgeSelectionNode =
          vtkSmartPointer<vtkSelectionNode>::New();
        edgeSelectionNode->SetSelectionList(selectedEdges);
        edgeSelectionNode->SetContentType(vtkSelectionNode::INDICES);
        edgeSelectionNode->SetFieldType(vtkSelectionNode::EDGE);
        edgeSelection->AddNode(edgeSelectionNode);

        // Convert the edge selection to the appropriate type for this representation.
        vtkSelection* edgeConverted = vtkConvertSelection::ToSelectionType(
          edgeSelection, input, this->SelectionType, this->SelectionArrayNames);

        // Add the converted induced edge selection to the output selection.
        if (edgeConverted->GetNumberOfNodes() > 0)
          {
          converted->AddNode(edgeConverted->GetNode(0));
          }
        edgeConverted->Delete();
        }

      // Add the vertex selection node to the output selection.
      converted->AddNode(vertexConverted->GetNode(i));
      }
    polyConverted->Delete();
    vertexConverted->Delete();
    }
  if (foundEdgeNode && !selectedVerticesFound)
    {
    // If no vertices were found (hence no induced edges), look for
    // edges that were within the selection box.

    // First, convert the cell selection on the polydata to
    // a pedigree ID selection (or index selection if there are no
    // pedigree IDs).
    vtkSmartPointer<vtkSelection> edgeSel =
      vtkSmartPointer<vtkSelection>::New();
    edgeSel->AddNode(edgeNode);
    vtkPolyData* poly = vtkPolyData::SafeDownCast(
      this->GraphToPoly->GetOutput());
    vtkSelection* polyConverted = 0;
    if (poly->GetCellData()->GetPedigreeIds())
      {
      polyConverted = vtkConvertSelection::ToSelectionType(
        edgeSel, poly, vtkSelectionNode::PEDIGREEIDS);
      }
    else
      {
      polyConverted = vtkConvertSelection::ToSelectionType(
        edgeSel, poly, vtkSelectionNode::INDICES);
      }

    // Now that we have a pedigree or index selection, interpret this
    // as an edge selection on the graph, and convert it to the
    // appropriate selection type for this representation.
    for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
      {
      polyConverted->GetNode(i)->SetFieldType(vtkSelectionNode::EDGE);
      }

    // Convert the edge selection to the appropriate type for this representation.
    vtkSelection* edgeConverted = vtkConvertSelection::ToSelectionType(
      polyConverted, input, this->SelectionType, this->SelectionArrayNames);

    // Add the vertex selection node to the output selection.
    for (unsigned int i = 0; i < edgeConverted->GetNumberOfNodes(); ++i)
      {
      converted->AddNode(edgeConverted->GetNode(i));
      }
    polyConverted->Delete();
    edgeConverted->Delete();
    }
  return converted;
}

int vtkRenderedGraphRepresentation::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector*)
{
  this->Layout->SetInputConnection(this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  this->ApplyVertexIcons->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  this->RemoveHiddenGraph->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  return 1;
}

void vtkRenderedGraphRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->ApplyColors->SetCellLookupTable(theme->GetCellLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  this->ApplyColors->SetScalePointLookupTable(theme->GetScalePointLookupTable());
  this->ApplyColors->SetScaleCellLookupTable(theme->GetScaleCellLookupTable());

  float baseSize = static_cast<float>(theme->GetPointSize());
  float lineWidth = static_cast<float>(theme->GetLineWidth());
  this->VertexGlyph->SetScreenSize(baseSize);
  this->VertexActor->GetProperty()->SetPointSize(baseSize);
  this->OutlineGlyph->SetScreenSize(baseSize + 2);
  this->OutlineActor->GetProperty()->SetPointSize(baseSize + 2);
  this->OutlineActor->GetProperty()->SetLineWidth(1);
  this->EdgeActor->GetProperty()->SetLineWidth(lineWidth);

  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());

  // FIXME: This is a strange hack to get around some weirdness with
  // the gradient background and multiple transparent actors (assuming
  // related to depth peeling or some junk...)
  if (theme->GetPointOpacity() == 0)
    {
    this->OutlineActor->VisibilityOff();
    }

  this->GetVertexLabelTextProperty()->ShallowCopy(theme->GetPointTextProperty());
  this->GetVertexLabelTextProperty()->SetLineOffset(-2*baseSize);
  this->GetEdgeLabelTextProperty()->ShallowCopy(theme->GetCellTextProperty());

  // Moronic hack.. the circles seem to be really small so make them bigger
  if (this->VertexGlyph->GetGlyphType() == vtkGraphToGlyphs::CIRCLE)
      {
      this->VertexGlyph->SetScreenSize(baseSize*2+1);
      this->OutlineGlyph->SetScreenSize(baseSize*2+1);
      }
}

//----------------------------------------------------------------------------
void vtkRenderedGraphRepresentation::ComputeSelectedGraphBounds(double bounds[6] )
{
  // Bring the graph up to date
  this->Layout->Update();

  // Convert to an index selection
  vtkSmartPointer<vtkConvertSelection> cs = vtkSmartPointer<vtkConvertSelection>::New();
  cs->SetInputConnection(0, this->GetInternalSelectionOutputPort());
  cs->SetInputConnection(1, this->Layout->GetOutputPort());
  cs->SetOutputType(vtkSelectionNode::INDICES);
  cs->Update();
  vtkGraph* data = vtkGraph::SafeDownCast(this->Layout->GetOutput());
  vtkSelection* converted = cs->GetOutput();

  // Iterate over the selection's nodes, constructing a list of selected vertices.
  // In the case of an edge selection, we add the edges' vertices to vertex list.

  vtkSmartPointer<vtkIdTypeArray> edgeList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasEdges = false;
  vtkSmartPointer<vtkIdTypeArray> vertexList = vtkSmartPointer<vtkIdTypeArray>::New();
  bool hasVertices = false;
  for( unsigned int m = 0; m < converted->GetNumberOfNodes(); ++m)
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
}

vtkUnicodeString vtkRenderedGraphRepresentation::GetHoverTextInternal(vtkSelection* sel)
{
  vtkGraph* input = vtkGraph::SafeDownCast(this->GetInput());
  vtkSmartPointer<vtkIdTypeArray> selectedItems = vtkSmartPointer<vtkIdTypeArray>::New();
  vtkConvertSelection::GetSelectedVertices(sel, input, selectedItems);
  vtkDataSetAttributes* data = input->GetVertexData();
  const char* hoverArrName = this->GetVertexHoverArrayName();
  if (selectedItems->GetNumberOfTuples() == 0)
    {
    vtkConvertSelection::GetSelectedEdges(sel, input, selectedItems);
    data = input->GetEdgeData();
    hoverArrName = this->GetEdgeHoverArrayName();
    }
  if (selectedItems->GetNumberOfTuples() == 0 || !hoverArrName)
    {
    return vtkUnicodeString();
    }
  vtkAbstractArray* arr = data->GetAbstractArray(hoverArrName);
  if (!arr)
    {
    return vtkUnicodeString();
    }
  vtkIdType item = selectedItems->GetValue(0);
  return arr->GetVariantValue(item).ToUnicodeString();
}

void vtkRenderedGraphRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LayoutStrategyName: "
     << (this->LayoutStrategyName ? this->LayoutStrategyName : "(none)") << endl;
  os << indent << "EdgeLayoutStrategyName: "
     << (this->EdgeLayoutStrategyName ? this->EdgeLayoutStrategyName : "(none)") << endl;
  os << indent << "VertexHoverArrayName: "
     << (this->VertexHoverArrayName ? this->VertexHoverArrayName : "(none)") << endl;
  os << indent << "EdgeHoverArrayName: "
     << (this->EdgeHoverArrayName ? this->EdgeHoverArrayName : "(none)") << endl;
  os << indent << "HideVertexLabelsOnInteraction: "
     << (this->HideVertexLabelsOnInteraction ? "On" : "Off") << endl;
  os << indent << "HideEdgeLabelsOnInteraction: "
     << (this->HideEdgeLabelsOnInteraction ? "On" : "Off") << endl;
}
