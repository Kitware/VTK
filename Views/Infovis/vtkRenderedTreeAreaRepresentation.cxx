/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedTreeAreaRepresentation.cxx

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

#include "vtkRenderedTreeAreaRepresentation.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAppendPolyData.h"
#include "vtkApplyColors.h"
#include "vtkAreaLayout.h"
#include "vtkCellArray.h"
#include "vtkConvertSelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkEdgeCenters.h"
#include "vtkExtractEdges.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkGraphHierarchicalBundleEdges.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
#include "vtkHierarchicalGraphPipeline.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleAreaSelectHover.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkSectorSource.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSplineGraphEdges.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTexturedActor2D.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkTreeRingToPolyData.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"
#include "vtkWorldPointPicker.h"

#ifdef VTK_USE_QT
#include "vtkQtTreeRingLabelMapper.h"
#endif

#include <vector>

class vtkRenderedTreeAreaRepresentation::Internals
{
public:
  std::vector<vtkSmartPointer<vtkHierarchicalGraphPipeline> > Graphs;
};

vtkStandardNewMacro(vtkRenderedTreeAreaRepresentation);

vtkRenderedTreeAreaRepresentation::vtkRenderedTreeAreaRepresentation()
{
  this->Implementation = new Internals;
  this->SetNumberOfInputPorts(2);
  // Processing objects
  this->ApplyColors            = vtkSmartPointer<vtkApplyColors>::New();
  this->VertexDegree           = vtkSmartPointer<vtkVertexDegree>::New();
  this->TreeAggregation        = vtkSmartPointer<vtkTreeFieldAggregator>::New();
  this->TreeLevels             = vtkSmartPointer<vtkTreeLevelsFilter>::New();
  this->Picker                 = vtkSmartPointer<vtkWorldPointPicker>::New();
  this->EdgeScalarBar          = vtkSmartPointer<vtkScalarBarWidget>::New();

  // Area objects
  this->AreaLayout         = vtkSmartPointer<vtkAreaLayout>::New();
  this->AreaToPolyData     = vtkTreeRingToPolyData::New();
  this->AreaMapper         = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->AreaActor          = vtkSmartPointer<vtkActor>::New();
  this->AreaLabelMapper    = vtkDynamic2DLabelMapper::New();
  this->AreaLabelActor     = vtkSmartPointer<vtkActor2D>::New();
  this->HighlightData      = vtkSmartPointer<vtkPolyData>::New();
  this->HighlightMapper    = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->HighlightActor     = vtkSmartPointer<vtkActor>::New();
  this->AreaLabelHierarchy = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->EmptyPolyData      = vtkSmartPointer<vtkPolyData>::New();

  this->AreaSizeArrayNameInternal = 0;
  this->AreaColorArrayNameInternal = 0;
  this->AreaLabelArrayNameInternal = 0;
  this->AreaLabelPriorityArrayNameInternal = 0;
  this->AreaHoverTextInternal = 0;
  this->AreaHoverArrayName = 0;
  this->UseRectangularCoordinates = false;

  this->SetAreaColorArrayName("level");
  this->ColorAreasByArrayOn();
  this->SetAreaSizeArrayName("size");
  this->SetGraphEdgeColorArrayName("fraction");
  this->ColorGraphEdgesByArrayOn();
  vtkDynamic2DLabelMapper* areaMapper = vtkDynamic2DLabelMapper::New();
  this->SetAreaLabelMapper(areaMapper);
  areaMapper->Delete();
  this->AreaLabelActor->PickableOff();

  vtkSmartPointer<vtkStackedTreeLayoutStrategy> strategy =
    vtkSmartPointer<vtkStackedTreeLayoutStrategy>::New();
  strategy->SetReverse(true);
  this->AreaLayout->SetLayoutStrategy(strategy);
  this->AreaLayout->SetAreaArrayName("area");
  this->SetShrinkPercentage(0.1);
  this->AreaToPolyData->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "area");

  // Set default parameters
  this->SetAreaLabelArrayName("id");
  this->AreaLabelVisibilityOff();
  this->EdgeScalarBar->GetScalarBarActor()->VisibilityOff();
  this->EdgeScalarBar->SetRepositionable(true);

  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();

  // Set filter attributes
  this->TreeAggregation->LeafVertexUnitSizeOn();

  // Highlight
  this->HighlightMapper->SetInputData(this->HighlightData);
  this->HighlightActor->SetMapper(this->HighlightMapper);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetLineWidth(4.0);

  /*
  <graphviz>
  digraph {
    "Tree input" -> TreeLevels -> VertexDegree -> TreeAggregation -> AreaLayout
    AreaLayout -> ApplyColors -> AreaToPolyData -> AreaMapper -> AreaActor
    AreaLayout -> AreaLabelMapper -> AreaLabelActor
    AreaLayout -> vtkHierarchicalGraphPipeline
    "Graph input" -> vtkHierarchicalGraphPipeline
  }
  </graphviz>
  */

  this->VertexDegree->SetInputConnection(this->TreeLevels->GetOutputPort());
  this->TreeAggregation->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->AreaLayout->SetInputConnection(this->TreeAggregation->GetOutputPort());

  this->ApplyColors->SetInputConnection(this->AreaLayout->GetOutputPort());
  this->AreaToPolyData->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->AreaMapper->SetInputConnection(this->AreaToPolyData->GetOutputPort());
  this->AreaMapper->SetScalarModeToUseCellFieldData();
  this->AreaMapper->SelectColorArray("vtkApplyColors color");
  this->AreaActor->SetMapper(this->AreaMapper);

  this->AreaLabelHierarchy->SetInputData(this->EmptyPolyData);

  // Set the orientation array to be the text rotation array produced by
  // vtkStackedTreeLayoutStrategy.
  this->AreaLabelHierarchy->SetInputArrayToProcess(4, 0, 0, vtkDataObject::VERTEX, "TextRotation");
  this->AreaLabelHierarchy->SetInputArrayToProcess(5, 0, 0, vtkDataObject::VERTEX, "TextBoundedSize");

  //this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
  //this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
}

vtkRenderedTreeAreaRepresentation::~vtkRenderedTreeAreaRepresentation()
{
  this->SetAreaSizeArrayNameInternal(0);
  this->SetAreaColorArrayNameInternal(0);
  this->SetAreaLabelArrayNameInternal(0);
  this->SetAreaLabelPriorityArrayNameInternal(0);
  this->SetAreaHoverTextInternal(0);
  this->SetAreaHoverArrayName(0);
  delete this->Implementation;
  if (this->AreaLabelMapper)
    {
    this->AreaLabelMapper->Delete();
    }
  if (this->AreaToPolyData)
    {
    this->AreaToPolyData->Delete();
    }
}

const char* vtkRenderedTreeAreaRepresentation::GetAreaSizeArrayName()
{
  return this->GetAreaSizeArrayNameInternal();
}

void vtkRenderedTreeAreaRepresentation::SetAreaSizeArrayName(const char* name)
{
  this->AreaLayout->SetSizeArrayName(name);
  this->SetAreaSizeArrayNameInternal(name);
}

const char* vtkRenderedTreeAreaRepresentation::GetAreaLabelArrayName()
{
  return this->AreaLabelHierarchy->GetLabelArrayName();
}

void vtkRenderedTreeAreaRepresentation::SetAreaLabelArrayName(const char* name)
{
  this->AreaLabelHierarchy->SetInputArrayToProcess(2, 0, 0, vtkDataObject::VERTEX, name);
}

vtkTextProperty* vtkRenderedTreeAreaRepresentation::GetAreaLabelTextProperty()
{
  return this->AreaLabelHierarchy->GetTextProperty();
}

void vtkRenderedTreeAreaRepresentation::SetAreaLabelTextProperty(vtkTextProperty* prop)
{
  this->AreaLabelHierarchy->SetTextProperty(prop);
}

const char* vtkRenderedTreeAreaRepresentation::GetAreaColorArrayName()
{
  return this->GetAreaColorArrayNameInternal();
}

void vtkRenderedTreeAreaRepresentation::SetAreaColorArrayName(const char* name)
{
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->SetAreaColorArrayNameInternal(name);
}

bool vtkRenderedTreeAreaRepresentation::ValidIndex(int idx)
{
  return (idx >= 0 &&
          idx < static_cast<int>(this->Implementation->Graphs.size()));
}

const char* vtkRenderedTreeAreaRepresentation::GetGraphEdgeColorArrayName(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetColorArrayName();
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::SetGraphEdgeColorArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetColorArrayName(name);
    this->EdgeScalarBar->GetScalarBarActor()->SetTitle(name);
    }
}

void vtkRenderedTreeAreaRepresentation::SetGraphEdgeColorToSplineFraction(int idx)
{
  this->SetGraphEdgeColorArrayName("fraction", idx);
}

void vtkRenderedTreeAreaRepresentation::SetAreaLabelPriorityArrayName(const char* name)
{
  this->AreaLabelHierarchy->SetInputArrayToProcess(0, 0, 0, vtkDataObject::VERTEX, name);
}

const char* vtkRenderedTreeAreaRepresentation::GetAreaLabelPriorityArrayName()
{
  return this->AreaLabelHierarchy->GetPriorityArrayName();
}

void vtkRenderedTreeAreaRepresentation::SetGraphBundlingStrength(double strength, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetBundlingStrength(strength);
    }
}

double vtkRenderedTreeAreaRepresentation::GetGraphBundlingStrength(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetBundlingStrength();
    }
  return 0.0;
}

void vtkRenderedTreeAreaRepresentation::SetGraphSplineType(int type, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetSplineType(type);
    }
}

int vtkRenderedTreeAreaRepresentation::GetGraphSplineType(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetSplineType();
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::SetEdgeScalarBarVisibility(bool b)
{
  this->EdgeScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool vtkRenderedTreeAreaRepresentation::GetEdgeScalarBarVisibility()
{
  return this->EdgeScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

void vtkRenderedTreeAreaRepresentation::SetGraphHoverArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetHoverArrayName(name);
    }
}

const char* vtkRenderedTreeAreaRepresentation::GetGraphHoverArrayName(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetHoverArrayName();
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::SetAreaLabelMapper(vtkLabeledDataMapper* mapper)
{
  // AreaLayout -> AreaLabelMapper -> AreaLabelActor
  if (this->AreaLabelMapper != mapper)
    {
    vtkLabeledDataMapper* oldMapper = this->AreaLabelMapper;
    this->AreaLabelMapper = mapper;
    if (this->AreaLabelMapper)
      {
      this->AreaLabelMapper->Register(this);
      this->AreaLabelMapper->SetLabelModeToLabelFieldData();
      if (oldMapper)
        {
        this->AreaLabelMapper->SetFieldDataName(oldMapper->GetFieldDataName());
        this->SetAreaLabelTextProperty( oldMapper->GetLabelTextProperty() );
        }
      this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
      this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
      }
    if (oldMapper)
      {
      oldMapper->Delete();
      }
    }
}

void vtkRenderedTreeAreaRepresentation::SetAreaToPolyData(vtkPolyDataAlgorithm* alg)
{
  // AreaLayout -> ApplyColors -> AreaToPolyData -> AreaMapper -> AreaActor
  if (this->AreaToPolyData != alg)
    {
    vtkPolyDataAlgorithm* oldAlg = this->AreaToPolyData;
    this->AreaToPolyData = alg;
    if (this->AreaToPolyData)
      {
      this->AreaToPolyData->Register(this);
      this->AreaToPolyData->SetInputConnection(this->ApplyColors->GetOutputPort());
      this->AreaMapper->SetInputConnection(this->AreaToPolyData->GetOutputPort());
      }
    if (oldAlg)
      {
      oldAlg->Delete();
      }
    }
}

vtkUnicodeString vtkRenderedTreeAreaRepresentation::GetHoverTextInternal(vtkSelection* sel)
{
  vtkGraph* input = vtkGraph::SafeDownCast(this->GetInput());
  vtkSmartPointer<vtkIdTypeArray> selectedItems = vtkSmartPointer<vtkIdTypeArray>::New();
  vtkConvertSelection::GetSelectedVertices(sel, input, selectedItems);
  vtkDataSetAttributes* data = input->GetVertexData();
  const char* hoverArrName = this->GetAreaHoverArrayName();
  if (selectedItems->GetNumberOfTuples() == 0)
    {
    for (int i = 0; i < this->GetNumberOfInputConnections(i); ++i)
      {
      vtkGraph* g = vtkGraph::SafeDownCast(this->GetInputDataObject(1, i));
      vtkConvertSelection::GetSelectedEdges(sel, g, selectedItems);
      if (selectedItems->GetNumberOfTuples() > 0)
        {
        hoverArrName = this->GetGraphHoverArrayName(i);
        data = g->GetEdgeData();
        break;
        }
      }
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

void vtkRenderedTreeAreaRepresentation::UpdateHoverHighlight(vtkView* view, int x, int y)
{
  // Make sure we have a context.
  vtkRenderer* r = vtkRenderView::SafeDownCast(view)->GetRenderer();
  vtkRenderWindow* win = r->GetRenderWindow();
  if (!win)
    {
    return;
    }
  win->MakeCurrent();
  if (!win->IsCurrent())
    {
    return;
    }

  // Use the hardware picker to find a point in world coordinates.
  this->Picker->Pick(x, y, 0, r);
  double pos[3];
  this->Picker->GetPickPosition(pos);
  float posFloat[3] =
  {
    static_cast<float>(pos[0]),
    static_cast<float>(pos[1]),
    static_cast<float>(pos[2])
  };
  this->AreaLayout->Update();
  vtkIdType id = this->AreaLayout->FindVertex(posFloat);
  if (id >= 0)
    {
    float sinfo[4] = {0.0, 1.0, 0.0, 1.0};
    double z = 0.02;
    this->AreaLayout->GetBoundingArea(id, sinfo);
    if (this->UseRectangularCoordinates)
      {
      vtkSmartPointer<vtkPoints> highlightPoints =
        vtkSmartPointer<vtkPoints>::New();
      highlightPoints->SetNumberOfPoints(5);

      vtkSmartPointer<vtkCellArray> highA =
        vtkSmartPointer<vtkCellArray>::New();
      highA->InsertNextCell(5);
      for( int i = 0; i < 5; ++i)
        {
        highA->InsertCellPoint(i);
        }
      highlightPoints->SetPoint(0, sinfo[0], sinfo[2], z);
      highlightPoints->SetPoint(1, sinfo[1], sinfo[2], z);
      highlightPoints->SetPoint(2, sinfo[1], sinfo[3], z);
      highlightPoints->SetPoint(3, sinfo[0], sinfo[3], z);
      highlightPoints->SetPoint(4, sinfo[0], sinfo[2], z);
      this->HighlightData->SetPoints(highlightPoints);
      this->HighlightData->SetLines(highA);
      }
    else
      {
      if( sinfo[1] - sinfo[0] != 360. )
        {
        vtkSmartPointer<vtkSectorSource> sector =
          vtkSmartPointer<vtkSectorSource>::New();
        sector->SetInnerRadius(sinfo[2]);
        sector->SetOuterRadius(sinfo[3]);
        sector->SetZCoord(z);
        sector->SetStartAngle(sinfo[0]);
        sector->SetEndAngle(sinfo[1]);

        int resolution = (int)((sinfo[1]-sinfo[0])/1);
        if( resolution < 1 )
          resolution = 1;
        sector->SetCircumferentialResolution(resolution);
        sector->Update();

        vtkSmartPointer<vtkExtractEdges> extract =
          vtkSmartPointer<vtkExtractEdges>::New();
        extract->SetInputConnection(sector->GetOutputPort());

        vtkSmartPointer<vtkAppendPolyData> append =
          vtkSmartPointer<vtkAppendPolyData>::New();
        append->AddInputConnection(extract->GetOutputPort());
        append->Update();

        this->HighlightData->ShallowCopy(append->GetOutput());
        }
      else
        {
        vtkSmartPointer<vtkPoints> highlightPoints =
          vtkSmartPointer<vtkPoints>::New();
        highlightPoints->SetNumberOfPoints(240);

        double conversion = vtkMath::Pi()/180.;
        double current_angle = 0.;

        vtkSmartPointer<vtkCellArray> highA =
          vtkSmartPointer<vtkCellArray>::New();
        for( int i = 0; i < 120; ++i)
          {
          highA->InsertNextCell(2);
          double current_x = sinfo[2]*cos(conversion*current_angle);
          double current_y = sinfo[2]*sin(conversion*current_angle);
          highlightPoints->SetPoint( i, current_x, current_y, z );

          current_angle += 3.;

          highA->InsertCellPoint(i);
          highA->InsertCellPoint((i+1)%120);
          }

        current_angle = 0.;
        for( int i = 0; i < 120; ++i)
          {
          highA->InsertNextCell(2);
          double current_x = sinfo[3]*cos(conversion*current_angle);
          double current_y = sinfo[3]*sin(conversion*current_angle);
          highlightPoints->SetPoint( 120+i, current_x, current_y, z );

          current_angle += 3.;

          highA->InsertCellPoint(120+i);
          highA->InsertCellPoint(120+((i+1)%120));
          }
        this->HighlightData->SetPoints(highlightPoints);
        this->HighlightData->SetLines(highA);
        }
      }
    this->HighlightActor->VisibilityOn();
    }
  else
    {
    this->HighlightActor->VisibilityOff();
    }
}

double vtkRenderedTreeAreaRepresentation::GetShrinkPercentage()
{
  return this->AreaLayout->GetLayoutStrategy()->GetShrinkPercentage();
}

void vtkRenderedTreeAreaRepresentation::SetShrinkPercentage(double pcent)
{
  this->AreaLayout->GetLayoutStrategy()->SetShrinkPercentage(pcent);
}

const char* vtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelArrayName(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetLabelArrayName();
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetLabelArrayName(name);
    }
}

vtkTextProperty* vtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelTextProperty(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetLabelTextProperty();
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelTextProperty(vtkTextProperty* prop, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetLabelTextProperty(prop);
    }
}

vtkAreaLayoutStrategy* vtkRenderedTreeAreaRepresentation::GetAreaLayoutStrategy()
{
  return this->AreaLayout->GetLayoutStrategy();
}

void vtkRenderedTreeAreaRepresentation::SetAreaLayoutStrategy(vtkAreaLayoutStrategy* s)
{
  this->AreaLayout->SetLayoutStrategy(s);
}

bool vtkRenderedTreeAreaRepresentation::GetAreaLabelVisibility()
{
  return this->AreaLabelHierarchy->GetInputConnection(0, 0) ==
         this->AreaLayout->GetOutputPort();
}

void vtkRenderedTreeAreaRepresentation::SetAreaLabelVisibility(bool b)
{
  if (b)
    {
    this->AreaLabelHierarchy->SetInputConnection(this->AreaLayout->GetOutputPort());
    }
  else
    {
    this->AreaLabelHierarchy->SetInputData(this->EmptyPolyData);
    }
}

bool vtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelVisibility(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetLabelVisibility();
    }
  return false;
}

void vtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelVisibility(bool b, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetLabelVisibility(b);
    }
}

bool vtkRenderedTreeAreaRepresentation::GetColorGraphEdgesByArray(int idx)
{
  if (this->ValidIndex(idx))
    {
    return this->Implementation->Graphs[idx]->GetColorEdgesByArray();
    }
  return false;
}

void vtkRenderedTreeAreaRepresentation::SetColorGraphEdgesByArray(bool b, int idx)
{
  if (this->ValidIndex(idx))
    {
    this->Implementation->Graphs[idx]->SetColorEdgesByArray(b);
    }
}

bool vtkRenderedTreeAreaRepresentation::GetColorAreasByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void vtkRenderedTreeAreaRepresentation::SetColorAreasByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

void vtkRenderedTreeAreaRepresentation::SetLabelRenderMode(int mode)
{
  if (mode != this->GetLabelRenderMode())
    {
    this->Superclass::SetLabelRenderMode(mode);
    if (mode == vtkRenderView::FREETYPE)
      {
      this->AreaLabelActor = vtkSmartPointer<vtkActor2D>::New();
      this->AreaLabelActor->PickableOff();

      vtkSmartPointer<vtkDynamic2DLabelMapper> mapper =
        vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
      this->SetAreaLabelMapper(mapper);
      }
    else if (mode == vtkRenderView::QT)
      {
#ifdef VTK_USE_QT
      this->AreaLabelActor = vtkSmartPointer<vtkTexturedActor2D>::New();
      this->AreaLabelActor->PickableOff();

      vtkSmartPointer<vtkQtTreeRingLabelMapper> mapper =
        vtkSmartPointer<vtkQtTreeRingLabelMapper>::New();
      this->SetAreaLabelMapper(mapper);
#else
      vtkErrorMacro("Qt label rendering not supported.");
#endif
      }
    else
      {
      vtkErrorMacro("Unknown label render mode.");
      }
    }
}

bool vtkRenderedTreeAreaRepresentation::AddToView(vtkView* view)
{
  this->Superclass::AddToView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    this->EdgeScalarBar->SetInteractor(rv->GetInteractor());
    rv->GetRenderer()->AddActor(this->AreaActor);
    //rv->GetRenderer()->AddActor(this->AreaLabelActor);
    rv->GetRenderer()->AddActor(this->HighlightActor);
    rv->GetRenderer()->AddActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->AddLabels(this->AreaLabelHierarchy->GetOutputPort());

#if 0
    // Debug code: display underlying tree
    vtkSmartPointer<vtkGraphMapper> mapper = vtkSmartPointer<vtkGraphMapper>::New();
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    mapper->SetInputConnection(this->AreaLayout->GetOutputPort(1));
    actor->SetMapper(mapper);
    rv->GetRenderer()->AddActor(actor);
#endif

    rv->RegisterProgress(this->TreeAggregation);
    rv->RegisterProgress(this->VertexDegree);
    rv->RegisterProgress(this->AreaLayout);
    rv->RegisterProgress(this->AreaToPolyData);
    return true;
    }
  return false;
}

bool vtkRenderedTreeAreaRepresentation::RemoveFromView(vtkView* view)
{
  this->Superclass::RemoveFromView(view);
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (rv)
    {
    rv->GetRenderer()->RemoveActor(this->AreaActor);
    rv->GetRenderer()->RemoveActor(this->AreaLabelActor);
    rv->GetRenderer()->RemoveActor(this->HighlightActor);
    rv->GetRenderer()->RemoveActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->UnRegisterProgress(this->TreeAggregation);
    rv->UnRegisterProgress(this->VertexDegree);
    rv->UnRegisterProgress(this->AreaLayout);
    rv->UnRegisterProgress(this->AreaToPolyData);
    return true;
    }
  return false;
}

vtkSelection* vtkRenderedTreeAreaRepresentation::ConvertSelection(
  vtkView* view, vtkSelection* sel)
{
  vtkSelection* converted = vtkSelection::New();

  // TODO: Somehow to figure out single select mode.
  unsigned int rect[4];
  rect[0] = 0;
  rect[1] = 0;
  rect[2] = 0;
  rect[3] = 0;
  bool singleSelectMode = false;
  if (rect[0] == rect[2] && rect[1] == rect[3])
    {
    singleSelectMode = true;
    }

  for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
    {
    vtkSelectionNode* node = sel->GetNode(i);
    vtkProp* prop = vtkProp::SafeDownCast(
        node->GetProperties()->Get(vtkSelectionNode::PROP()));
    if (prop == this->AreaActor.GetPointer())
      {
      vtkSmartPointer<vtkIdTypeArray> vertexIds;
      vertexIds = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());

      // If we are in single select mode, make sure to select only the vertex
      // that is being hovered over.
      vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
      if (rv && singleSelectMode)
        {
        vtkInteractorStyleAreaSelectHover* style =
          vtkInteractorStyleAreaSelectHover::SafeDownCast(
            rv->GetInteractorStyle());
        if (style)
          {
          vtkIdType v = style->GetIdAtPos(rect[0], rect[1]);
          vertexIds = vtkSmartPointer<vtkIdTypeArray>::New();
          if (v >= 0)
            {
            vertexIds->InsertNextValue(v);
            }
          }
        }

      // Create a vertex selection.
      vtkSmartPointer<vtkSelection> vertexIndexSelection =
        vtkSmartPointer<vtkSelection>::New();
      vtkSmartPointer<vtkSelectionNode> vertexIndexNode =
        vtkSmartPointer<vtkSelectionNode>::New();
      vertexIndexNode->SetContentType(vtkSelectionNode::INDICES);
      vertexIndexNode->SetFieldType(vtkSelectionNode::CELL);
      vertexIndexNode->SetSelectionList(vertexIds);
      vertexIndexSelection->AddNode(vertexIndexNode);

      // Convert to pedigree ids.
      // Make it a vertex selection.
      this->AreaToPolyData->Update();
      vtkSmartPointer<vtkSelection> vertexSelection;
      vertexSelection.TakeReference(vtkConvertSelection::ToSelectionType(
        vertexIndexSelection, this->AreaToPolyData->GetOutput(),
        vtkSelectionNode::PEDIGREEIDS));
      vtkSelectionNode* vnode = vertexSelection->GetNode(0);
      if (vnode && vnode->GetSelectionList()->GetNumberOfTuples() > 0)
        {
        vnode->SetFieldType(vtkSelectionNode::VERTEX);
        converted->AddNode(vnode);

        // Find matching vertex pedigree ids in all input graphs
        // and add outgoing edges to selection

        vtkAbstractArray* arr = vnode->GetSelectionList();
        size_t numGraphs = static_cast<size_t>(this->GetNumberOfInputConnections(1));
        vtkSmartPointer<vtkOutEdgeIterator> iter = vtkSmartPointer<vtkOutEdgeIterator>::New();
        for (size_t k = 0; k < numGraphs; ++k)
          {
          vtkSmartPointer<vtkSelection> edgeIndexSelection =
            vtkSmartPointer<vtkSelection>::New();
          vtkSmartPointer<vtkSelectionNode> edgeIndexNode =
            vtkSmartPointer<vtkSelectionNode>::New();
          edgeIndexNode->SetContentType(vtkSelectionNode::INDICES);
          edgeIndexNode->SetFieldType(vtkSelectionNode::EDGE);
          vtkSmartPointer<vtkIdTypeArray> edgeIds =
            vtkSmartPointer<vtkIdTypeArray>::New();
          edgeIndexNode->SetSelectionList(edgeIds);
          edgeIndexSelection->AddNode(edgeIndexNode);

          vtkGraph* g = vtkGraph::SafeDownCast(this->GetInternalOutputPort(1, static_cast<int>(k))->GetProducer()->GetOutputDataObject(0));
          vtkAbstractArray* arr2 = g->GetVertexData()->GetPedigreeIds();
          vtkStringArray* domainArr = vtkStringArray::SafeDownCast(g->GetVertexData()->GetAbstractArray("domain"));
          for(vtkIdType j=0; j<arr->GetNumberOfTuples(); ++j)
            {
            vtkIdType id = arr2->LookupValue(arr->GetVariantValue(j));
            if (id == -1)
              {
              continue;
              }

            // Before adding vertex's edges, make sure its in the same domain as selected vertex
            vtkStdString domain;
            if(domainArr)
              {
              domain = domainArr->GetValue(id);
              }
            else
              {
              domain = arr2->GetName();
              }
            if(domain != arr->GetName())
              {
              continue;
              }

            g->GetOutEdges(id, iter);
            while(iter->HasNext())
              {
              edgeIds->InsertNextValue(iter->Next().Id);
              }
            }

          vtkSmartPointer<vtkSelection> edgeSelection;
          edgeSelection.TakeReference(vtkConvertSelection::ToSelectionType(
            edgeIndexSelection, g,
            vtkSelectionNode::PEDIGREEIDS));
          converted->AddNode(edgeSelection->GetNode(0));
          }
        }
      }
    }
  // Graph edge selections.
  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
    {
    vtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    vtkSelection* conv = p->ConvertSelection(this, sel);
    if (conv)
      {
      for (unsigned int j = 0; j < conv->GetNumberOfNodes(); ++j)
        {
        converted->AddNode(conv->GetNode(j));
        }
      conv->Delete();
      }
    }
  //cerr << "Tree converted: " << endl;
  //converted->Dump();

  return converted;
}

int vtkRenderedTreeAreaRepresentation::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Tree area connections
  this->TreeLevels->SetInputConnection(this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());

  // Add new graph objects if needed.
  size_t numGraphs = static_cast<size_t>(this->GetNumberOfInputConnections(1));
  while (numGraphs > this->Implementation->Graphs.size())
    {
    this->Implementation->Graphs.push_back(
      vtkSmartPointer<vtkHierarchicalGraphPipeline>::New());
    }

  // Keep track of actors to remove if the number of input connections
  // decreased.
  for (size_t i = numGraphs; i < this->Implementation->Graphs.size(); ++i)
    {
    this->RemovePropOnNextRender(this->Implementation->Graphs[i]->GetActor());
    this->RemovePropOnNextRender(this->Implementation->Graphs[i]->GetLabelActor());
    }
  this->Implementation->Graphs.resize(numGraphs);

  // Make sure all hierarchical graph edges inputs are up to date.
  for (size_t i = 0; i < numGraphs; ++i)
    {
    this->AddPropOnNextRender(this->Implementation->Graphs[i]->GetActor());
    this->AddPropOnNextRender(this->Implementation->Graphs[i]->GetLabelActor());
    vtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->PrepareInputConnections(
      this->GetInternalOutputPort(1, static_cast<int>(i)),
      this->AreaLayout->GetOutputPort(1),
      this->GetInternalAnnotationOutputPort(1, static_cast<int>(i)));
    }
  return 1;
}

void vtkRenderedTreeAreaRepresentation::PrepareForRendering(vtkRenderView* view)
{
#if 0
  // Make hover highlight up to date
  int pos[2] = {0, 0};
  if (view->GetInteractorStyle() && view->GetInteractorStyle()->GetInteractor())
    {
    view->GetInteractorStyle()->GetInteractor()->GetEventPosition(pos);
    this->UpdateHoverHighlight(view, pos[0], pos[1]);
    }
#ifdef VTK_USE_QT
  vtkQtTreeRingLabelMapper* mapper =
    vtkQtTreeRingLabelMapper::SafeDownCast(this->AreaLabelMapper);
  if (mapper)
    {
    mapper->SetRenderer(view->GetRenderer());
    }
  //view->GetRenderer()->AddActor(this->AreaLabelActor);
#endif
#endif

  // Make sure all the graphs are registered.
  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
    {
    this->Implementation->Graphs[i]->RegisterProgress(view);
    }

  this->Superclass::PrepareForRendering(view);
}

void vtkRenderedTreeAreaRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->EdgeScalarBar->GetScalarBarActor()->SetLookupTable(theme->GetCellLookupTable());

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

  this->GetAreaLabelTextProperty()->ShallowCopy(theme->GetPointTextProperty());

  // Make sure we have the right number of graphs
  if (this->GetNumberOfInputConnections(1) !=
      static_cast<int>(this->Implementation->Graphs.size()))
    {
    this->Update();
    }

  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
    {
    vtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->ApplyViewTheme(theme);
    }
}

int vtkRenderedTreeAreaRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    return 1;
    }
  return 0;
}

void vtkRenderedTreeAreaRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
  os << indent << "AreaHoverArrayName: " << (this->AreaHoverArrayName ? this->AreaHoverArrayName : "(none)") << endl;
  os << indent << "AreaToPolyData: ";
  if (this->AreaToPolyData)
    {
    os << "\n";
    this->AreaToPolyData->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "AreaLabelMapper: ";
  if (this->AreaLabelMapper)
    {
    os << "\n";
    this->AreaLabelMapper->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
