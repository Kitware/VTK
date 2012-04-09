/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedSurfaceRepresentation.cxx

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

#include "vtkRenderedSurfaceRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkApplyColors.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObject.h"
#include "vtkExtractSelection.h"
#include "vtkGeometryFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTransformFilter.h"
#include "vtkViewTheme.h"

vtkStandardNewMacro(vtkRenderedSurfaceRepresentation);
//----------------------------------------------------------------------------
vtkRenderedSurfaceRepresentation::vtkRenderedSurfaceRepresentation()
{
  this->TransformFilter         = vtkTransformFilter::New();
  this->ApplyColors             = vtkApplyColors::New();
  this->GeometryFilter          = vtkGeometryFilter::New();
  this->Mapper                  = vtkPolyDataMapper::New();
  this->Actor                   = vtkActor::New();

  this->CellColorArrayNameInternal = 0;
   
  // Connect pipeline
  this->ApplyColors->SetInputConnection(this->TransformFilter->GetOutputPort());
  this->GeometryFilter->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->Mapper->SetInputConnection(this->GeometryFilter->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
  this->Actor->GetProperty()->SetPointSize(10);
  
  // Set parameters
  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray("vtkApplyColors color");
  this->Mapper->SetScalarVisibility(true);

  // Apply default theme
  vtkSmartPointer<vtkViewTheme> theme =
    vtkSmartPointer<vtkViewTheme>::New();
  theme->SetCellOpacity(1.0);
  this->ApplyViewTheme(theme);
}

//----------------------------------------------------------------------------
vtkRenderedSurfaceRepresentation::~vtkRenderedSurfaceRepresentation()
{
  this->TransformFilter->Delete();
  this->ApplyColors->Delete();
  this->GeometryFilter->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->SetCellColorArrayNameInternal(0);
}

//----------------------------------------------------------------------------
int vtkRenderedSurfaceRepresentation::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector*)
{
  this->TransformFilter->SetInputConnection(0, this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  return 1;
}

//----------------------------------------------------------------------------
void vtkRenderedSurfaceRepresentation::PrepareForRendering(vtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);
  this->TransformFilter->SetTransform(view->GetTransform());
}

//----------------------------------------------------------------------------
bool vtkRenderedSurfaceRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    vtkErrorMacro("Can only add to a subclass of vtkRenderView.");
    return false;
    }
  rv->GetRenderer()->AddActor(this->Actor);
  return true;
}

//----------------------------------------------------------------------------
bool vtkRenderedSurfaceRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    return false;
    }
  rv->GetRenderer()->RemoveActor(this->Actor);
  return true;
}

//----------------------------------------------------------------------------
vtkSelection* vtkRenderedSurfaceRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view),
  vtkSelection* selection)
{
  vtkSmartPointer<vtkSelection> propSelection =
    vtkSmartPointer<vtkSelection>::New();

  // Extract the selection for the right prop
  if (selection->GetNumberOfNodes() > 1)
    {
    for (unsigned int i = 0; i < selection->GetNumberOfNodes(); i++)
      {
      vtkSelectionNode* node = selection->GetNode(i);
      vtkProp* prop = vtkProp::SafeDownCast(
        node->GetProperties()->Get(vtkSelectionNode::PROP()));
      if (prop == this->Actor)
        {
        vtkSmartPointer<vtkSelectionNode> nodeCopy =
          vtkSmartPointer<vtkSelectionNode>::New();
        nodeCopy->ShallowCopy(node);
        nodeCopy->GetProperties()->Remove(vtkSelectionNode::PROP());
        propSelection->AddNode(nodeCopy);
        }
      }
    }
  else
    {
    propSelection->ShallowCopy(selection);
    }

  // Start with an empty selection
  vtkSelection* converted = vtkSelection::New();
  vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
  node->SetContentType(this->SelectionType);
  node->SetFieldType(vtkSelectionNode::CELL);
  vtkSmartPointer<vtkIdTypeArray> empty =
    vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(empty);
  converted->AddNode(node);
  // Convert to the correct type of selection
  if (this->GetInput())
    {
    vtkDataObject* obj = this->GetInput();
    if (obj)
      {
      vtkSelection* index = vtkConvertSelection::ToSelectionType(
        propSelection, obj, this->SelectionType,
        this->SelectionArrayNames);
      converted->ShallowCopy(index);
      index->Delete();
      }
    }
  
  return converted;
}

//----------------------------------------------------------------------------
void vtkRenderedSurfaceRepresentation::SetCellColorArrayName(
 const char* arrayName)
{
  this->SetCellColorArrayNameInternal(arrayName);
  this->ApplyColors->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, arrayName);
}

//----------------------------------------------------------------------------
void vtkRenderedSurfaceRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->ApplyColors->SetCellLookupTable(theme->GetCellLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  //this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  //this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  this->ApplyColors->SetScalePointLookupTable(theme->GetScalePointLookupTable());
  this->ApplyColors->SetScaleCellLookupTable(theme->GetScaleCellLookupTable());

  float baseSize = static_cast<float>(theme->GetPointSize());
  float lineWidth = static_cast<float>(theme->GetLineWidth());
  this->Actor->GetProperty()->SetPointSize(baseSize);
  this->Actor->GetProperty()->SetLineWidth(lineWidth);

  // TODO: Enable labeling
  //this->VertexTextProperty->SetColor(theme->GetVertexLabelColor());
  //this->VertexTextProperty->SetLineOffset(-2*baseSize);
  //this->EdgeTextProperty->SetColor(theme->GetEdgeLabelColor());
}

//----------------------------------------------------------------------------
void vtkRenderedSurfaceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ApplyColors:" << endl;
  this->ApplyColors->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GeometryFilter:" << endl;
  this->GeometryFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:" << endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
}
