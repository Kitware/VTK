/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkGraphAnnotationLayersFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkGraphAnnotationLayersFilter.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkConvexHull2D.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkGraphAnnotationLayersFilter);

//-----------------------------------------------------------------------------
vtkGraphAnnotationLayersFilter::vtkGraphAnnotationLayersFilter()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);

  this->HullAppend = vtkSmartPointer<vtkAppendPolyData>::New();
  this->OutlineAppend = vtkSmartPointer<vtkAppendPolyData>::New();
  this->ConvexHullFilter = vtkSmartPointer<vtkConvexHull2D>::New();
}

//-----------------------------------------------------------------------------
vtkGraphAnnotationLayersFilter::~vtkGraphAnnotationLayersFilter()
{
  //
}

//-----------------------------------------------------------------------------
int vtkGraphAnnotationLayersFilter::FillInputPortInformation(int port,
  vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
      "vtkGraph");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
      "vtkAnnotationLayers");
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::OutlineOn()
{
  this->ConvexHullFilter->OutlineOn();
}

void vtkGraphAnnotationLayersFilter::OutlineOff()
{
  this->ConvexHullFilter->OutlineOff();
}

void vtkGraphAnnotationLayersFilter::SetOutline(bool b)
{
  this->ConvexHullFilter->SetOutline(b);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetScaleFactor(double scale)
{
  this->ConvexHullFilter->SetScaleFactor(scale);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetHullShapeToBoundingRectangle()
{
  this->ConvexHullFilter->SetHullShape(vtkConvexHull2D::BoundingRectangle);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetHullShapeToConvexHull()
{
  this->ConvexHullFilter->SetHullShape(vtkConvexHull2D::ConvexHull);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetMinHullSizeInWorld(double size)
{
  this->ConvexHullFilter->SetMinHullSizeInWorld(size);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetMinHullSizeInDisplay(int size)
{
  this->ConvexHullFilter->SetMinHullSizeInDisplay(size);
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::SetRenderer(vtkRenderer* renderer)
{
  this->ConvexHullFilter->SetRenderer(renderer);
}

//-----------------------------------------------------------------------------
unsigned long vtkGraphAnnotationLayersFilter::GetMTime()
{
  if (this->ConvexHullFilter)
    {
    return this->ConvexHullFilter->GetMTime();
    }
  else
    {
    return this->MTime;
    }
}

//-----------------------------------------------------------------------------
int vtkGraphAnnotationLayersFilter::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // Get the input and output.
  vtkInformation *inGraphInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *inLayersInfo = inputVector[1]->GetInformationObject(0);

  vtkGraph* graph = vtkGraph::SafeDownCast(
    inGraphInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPoints* inputPoints = graph->GetPoints();
  vtkAnnotationLayers* layers = vtkAnnotationLayers::SafeDownCast(
    inLayersInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation *outInfo1 = outputVector->GetInformationObject(1);

  vtkPolyData *outputHull = vtkPolyData::SafeDownCast(outInfo0->Get(
    vtkDataObject::DATA_OBJECT()));
  vtkPolyData *outputOutline = vtkPolyData::SafeDownCast(outInfo1->Get(
    vtkDataObject::DATA_OBJECT()));

  this->HullAppend->RemoveAllInputs();
  this->OutlineAppend->RemoveAllInputs();

  unsigned numberOfAnnotations = layers->GetNumberOfAnnotations();

  // Generate one hull/polydata per selection node
  vtkIdType hullId = 0;
  for (unsigned annotationId = 0; annotationId < numberOfAnnotations;
    ++annotationId)
    {
    vtkAnnotation* annotation = layers->GetAnnotation(annotationId);
    if (annotation->GetInformation()->Get(vtkAnnotation::ENABLE()) == 0)
      {
      continue;
      }

    vtkSelection* selection = annotation->GetSelection();
    unsigned numberOfSelectionNodes = selection->GetNumberOfNodes();

    for (unsigned selectionNodeId = 0; selectionNodeId < numberOfSelectionNodes;
      ++selectionNodeId)
      {
      vtkSmartPointer<vtkPoints> hullPoints = vtkSmartPointer<vtkPoints>::New();

      hullId++;
      vtkSelectionNode* selectionNode = selection->GetNode(selectionNodeId);
      if (selectionNode->GetFieldType() != vtkSelectionNode::VERTEX)
        {
        continue;
        }
      vtkIdTypeArray* vertexIds = vtkIdTypeArray::SafeDownCast(
        selectionNode->GetSelectionList());

      // Get points from graph
      vtkIdType numberOfNodePoints = vertexIds->GetNumberOfTuples();
      if (numberOfNodePoints == 0)
        {
        continue;
        }
      for (vtkIdType i = 0; i < numberOfNodePoints; ++i)
        {
        hullPoints->InsertNextPoint(inputPoints->GetPoint(vertexIds->GetValue(i)));
        }

      // Create filled polygon
      vtkSmartPointer<vtkPolyData> hullPolyData =
        vtkSmartPointer<vtkPolyData>::New();
      hullPolyData->SetPoints(hullPoints);
      ConvexHullFilter->SetInputData(hullPolyData);
      ConvexHullFilter->Update();
      hullPolyData->ShallowCopy(ConvexHullFilter->GetOutput());

      // Add data arrays to the polydata
      vtkIdType representativeVertex = vertexIds->GetValue(0);
      vtkIdType numberOfCells = hullPolyData->GetNumberOfCells();

      vtkUnsignedCharArray* outColors = vtkUnsignedCharArray::New();
      outColors->SetNumberOfComponents(4);
      outColors->SetName("Hull color");
      double* color = annotation->GetInformation()->Get(vtkAnnotation::COLOR());
      double opacity = annotation->GetInformation()->Get(vtkAnnotation::OPACITY());
      unsigned char outColor[4] = {
        static_cast<unsigned char>(color[0] * 255),
        static_cast<unsigned char>(color[1] * 255),
        static_cast<unsigned char>(color[2] * 255),
        static_cast<unsigned char>(opacity * 255) };
      for (vtkIdType i = 0; i < numberOfCells; ++i)
        {
        outColors->InsertNextTupleValue(outColor);
        }
      hullPolyData->GetCellData()->AddArray(outColors);
      outColors->Delete();

      vtkIdTypeArray* hullIds = vtkIdTypeArray::New();
      hullIds->SetName("Hull id");
      for (vtkIdType i = 0; i < numberOfCells; ++i)
        {
        hullIds->InsertNextValue(hullId);
        }
      hullPolyData->GetCellData()->AddArray(hullIds);
      hullIds->Delete();

      vtkStringArray* hullName = vtkStringArray::New();
      hullName->SetName("Hull name");
      for (vtkIdType i = 0; i < numberOfCells; ++i)
        {
        hullName->InsertNextValue(
          annotation->GetInformation()->Get(vtkAnnotation::LABEL()));
        }
      hullPolyData->GetCellData()->AddArray(hullName);
      hullName->Delete();

      vtkDoubleArray* hullCentreVertex = vtkDoubleArray::New();
      hullCentreVertex->SetName("Hull point");
      hullCentreVertex->SetNumberOfComponents(3);
      for (vtkIdType i = 0; i < numberOfCells; ++i)
        {
        hullCentreVertex->InsertNextTuple(
          inputPoints->GetPoint(representativeVertex));
        }
      hullPolyData->GetCellData()->AddArray(hullCentreVertex);
      hullCentreVertex->Delete();

      this->HullAppend->AddInputData(hullPolyData);

      if (this->ConvexHullFilter->GetOutline())
        {
        vtkSmartPointer<vtkPolyData> outlinePolyData =
          vtkSmartPointer<vtkPolyData>::New();
        outlinePolyData->ShallowCopy(ConvexHullFilter->GetOutput(1));
        this->OutlineAppend->AddInputData(outlinePolyData);
        }
      } // Next selection node.
    } // Next annotation.

  // Send data to output
  if (this->HullAppend->GetNumberOfInputConnections(0) > 0)
    {
    this->HullAppend->Update();
    outputHull->ShallowCopy(this->HullAppend->GetOutput());
    }
  if (this->OutlineAppend->GetNumberOfInputConnections(0) > 0)
    {
    this->OutlineAppend->Update();
    outputOutline->ShallowCopy(this->OutlineAppend->GetOutput());
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGraphAnnotationLayersFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConvexHull2D: ";
    if (this->ConvexHullFilter)
      {
      os << endl;
      this->ConvexHullFilter->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << "(none)" << endl;
      }
}
