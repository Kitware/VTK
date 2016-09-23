/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkApplyColors.cxx

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
#include "vtkApplyColors.h"

#include "vtkAnnotation.h"
#include "vtkAnnotationLayers.h"
#include "vtkCellData.h"
#include "vtkConvertSelection.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkDataSet.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkApplyColors);
vtkCxxSetObjectMacro(vtkApplyColors, PointLookupTable, vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkApplyColors, CellLookupTable, vtkScalarsToColors);

vtkApplyColors::vtkApplyColors()
{
  this->PointLookupTable = 0;
  this->CellLookupTable = 0;
  this->DefaultPointColor[0] = 0.0;
  this->DefaultPointColor[1] = 0.0;
  this->DefaultPointColor[2] = 0.0;
  this->DefaultPointOpacity = 1.0;
  this->DefaultCellColor[0] = 0.0;
  this->DefaultCellColor[1] = 0.0;
  this->DefaultCellColor[2] = 0.0;
  this->DefaultCellOpacity = 1.0;
  this->SelectedPointColor[0] = 0.0;
  this->SelectedPointColor[1] = 0.0;
  this->SelectedPointColor[2] = 0.0;
  this->SelectedPointOpacity = 1.0;
  this->SelectedCellColor[0] = 0.0;
  this->SelectedCellColor[1] = 0.0;
  this->SelectedCellColor[2] = 0.0;
  this->SelectedCellOpacity = 1.0;
  this->SetNumberOfInputPorts(2);
  this->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_VERTICES,
    vtkDataSetAttributes::SCALARS);
  this->SetInputArrayToProcess(1, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_EDGES,
    vtkDataSetAttributes::SCALARS);
  this->ScalePointLookupTable = true;
  this->ScaleCellLookupTable = true;
  this->UsePointLookupTable = false;
  this->UseCellLookupTable = false;
  this->PointColorOutputArrayName = 0;
  this->CellColorOutputArrayName = 0;
  this->SetPointColorOutputArrayName("vtkApplyColors color");
  this->SetCellColorOutputArrayName("vtkApplyColors color");
  this->UseCurrentAnnotationColor = false;
}

vtkApplyColors::~vtkApplyColors()
{
  this->SetPointLookupTable(0);
  this->SetCellLookupTable(0);
  this->SetPointColorOutputArrayName(0);
  this->SetCellColorOutputArrayName(0);
}

int vtkApplyColors::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAnnotationLayers");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

int vtkApplyColors::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* layersInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!this->PointColorOutputArrayName || !this->CellColorOutputArrayName)
  {
    vtkErrorMacro("Point and cell array names must be valid");
    return 0;
  }

  // get the input and output
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkAnnotationLayers* layers = 0;
  if (layersInfo)
  {
    layers = vtkAnnotationLayers::SafeDownCast(
      layersInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  output->ShallowCopy(input);

  vtkGraph* graph = vtkGraph::SafeDownCast(output);
  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(output);
  vtkTable* table = vtkTable::SafeDownCast(output);

  // initialize color arrays
  vtkSmartPointer<vtkUnsignedCharArray> colorArr1 =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  colorArr1->SetName(this->PointColorOutputArrayName);
  colorArr1->SetNumberOfComponents(4);
  if (graph)
  {
    colorArr1->SetNumberOfTuples(graph->GetNumberOfVertices());
    graph->GetVertexData()->AddArray(colorArr1);
  }
  else if (dataSet)
  {
    colorArr1->SetNumberOfTuples(dataSet->GetNumberOfPoints());
    dataSet->GetPointData()->AddArray(colorArr1);
  }
  else
  {
    colorArr1->SetNumberOfTuples(table->GetNumberOfRows());
    table->AddColumn(colorArr1);
  }
  vtkSmartPointer<vtkUnsignedCharArray> colorArr2 =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  colorArr2->SetName(this->CellColorOutputArrayName);
  colorArr2->SetNumberOfComponents(4);
  if (graph)
  {
    colorArr2->SetNumberOfTuples(graph->GetNumberOfEdges());
    graph->GetEdgeData()->AddArray(colorArr2);
  }
  else if (dataSet)
  {
    colorArr2->SetNumberOfTuples(dataSet->GetNumberOfCells());
    dataSet->GetCellData()->AddArray(colorArr2);
  }

  unsigned char pointColor[4];
  pointColor[0] = static_cast<unsigned char>(255*this->DefaultPointColor[0]);
  pointColor[1] = static_cast<unsigned char>(255*this->DefaultPointColor[1]);
  pointColor[2] = static_cast<unsigned char>(255*this->DefaultPointColor[2]);
  pointColor[3] = static_cast<unsigned char>(255*this->DefaultPointOpacity);
  vtkAbstractArray* arr1 = 0;
  if (this->PointLookupTable && this->UsePointLookupTable)
  {
    arr1 = this->GetInputAbstractArrayToProcess(0, inputVector);
  }
  this->ProcessColorArray(colorArr1, this->PointLookupTable, arr1,
    pointColor, this->ScalePointLookupTable);

  unsigned char cellColor[4];
  cellColor[0] = static_cast<unsigned char>(255*this->DefaultCellColor[0]);
  cellColor[1] = static_cast<unsigned char>(255*this->DefaultCellColor[1]);
  cellColor[2] = static_cast<unsigned char>(255*this->DefaultCellColor[2]);
  cellColor[3] = static_cast<unsigned char>(255*this->DefaultCellOpacity);
  vtkAbstractArray* arr2 = 0;
  if (this->CellLookupTable && this->UseCellLookupTable)
  {
    arr2 = this->GetInputAbstractArrayToProcess(1, inputVector);
  }
  this->ProcessColorArray(colorArr2, this->CellLookupTable, arr2,
    cellColor, this->ScaleCellLookupTable);

  if (layers)
  {
    vtkSmartPointer<vtkIdTypeArray> list1 =
      vtkSmartPointer<vtkIdTypeArray>::New();
    vtkSmartPointer<vtkIdTypeArray> list2 =
      vtkSmartPointer<vtkIdTypeArray>::New();
    unsigned char annColor[4] = {0, 0, 0, 0};
    unsigned char prev[4] = {0, 0, 0, 0};
    unsigned int numAnnotations = layers->GetNumberOfAnnotations();
    for (unsigned int a = 0; a < numAnnotations; ++a)
    {
      vtkAnnotation* ann = layers->GetAnnotation(a);
      if (ann->GetInformation()->Has(vtkAnnotation::ENABLE()) &&
          ann->GetInformation()->Get(vtkAnnotation::ENABLE())==0)
      {
        continue;
      }
      list1->Initialize();
      list2->Initialize();
      vtkSelection* sel = ann->GetSelection();
      bool hasColor = false;
      bool hasOpacity = false;
      if (ann->GetInformation()->Has(vtkAnnotation::COLOR()))
      {
        hasColor = true;
        double* color = ann->GetInformation()->Get(vtkAnnotation::COLOR());
        annColor[0] = static_cast<unsigned char>(255*color[0]);
        annColor[1] = static_cast<unsigned char>(255*color[1]);
        annColor[2] = static_cast<unsigned char>(255*color[2]);
      }
      if (ann->GetInformation()->Has(vtkAnnotation::OPACITY()))
      {
        hasOpacity = true;
        double opacity = ann->GetInformation()->Get(vtkAnnotation::OPACITY());
        annColor[3] = static_cast<unsigned char>(255*opacity);
      }
      if (!hasColor && !hasOpacity)
      {
        continue;
      }
      if (graph)
      {
        vtkConvertSelection::GetSelectedVertices(sel, graph, list1);
        vtkConvertSelection::GetSelectedEdges(sel, graph, list2);
      }
      else if (dataSet)
      {
        vtkConvertSelection::GetSelectedPoints(sel, dataSet, list1);
        vtkConvertSelection::GetSelectedCells(sel, dataSet, list2);
      }
      else
      {
        vtkConvertSelection::GetSelectedRows(sel, table, list1);
      }
      vtkIdType numIds = list1->GetNumberOfTuples();
      unsigned char curColor[4];
      for (vtkIdType i = 0; i < numIds; ++i)
      {
        if (list1->GetValue(i) >= colorArr1->GetNumberOfTuples())
        {
          continue;
        }
        colorArr1->GetTypedTuple(list1->GetValue(i), prev);
        if (hasColor)
        {
          curColor[0] = annColor[0];
          curColor[1] = annColor[1];
          curColor[2] = annColor[2];
        }
        else
        {
          curColor[0] = prev[0];
          curColor[1] = prev[1];
          curColor[2] = prev[2];
        }
        if (hasOpacity)
        {
          // Combine opacities
          curColor[3] = static_cast<unsigned char>((prev[3]/255.0)*annColor[3]);
        }
        else
        {
          curColor[3] = prev[3];
        }
        colorArr1->SetTypedTuple(list1->GetValue(i), curColor);
      }
      numIds = list2->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIds; ++i)
      {
        if (list2->GetValue(i) >= colorArr2->GetNumberOfTuples())
        {
          continue;
        }
        colorArr2->GetTypedTuple(list2->GetValue(i), prev);
        if (hasColor)
        {
          curColor[0] = annColor[0];
          curColor[1] = annColor[1];
          curColor[2] = annColor[2];
        }
        else
        {
          curColor[0] = prev[0];
          curColor[1] = prev[1];
          curColor[2] = prev[2];
        }
        if (hasOpacity)
        {
          // Combine opacities
          curColor[3] = static_cast<unsigned char>((prev[3]/255.0)*annColor[3]);
        }
        else
        {
          curColor[3] = prev[3];
        }
        colorArr2->SetTypedTuple(list2->GetValue(i), curColor);
      }
    }
    if (vtkAnnotation* ann = layers->GetCurrentAnnotation())
    {
      vtkSelection* selection = ann->GetSelection();
      list1 = vtkSmartPointer<vtkIdTypeArray>::New();
      list2 = vtkSmartPointer<vtkIdTypeArray>::New();
      unsigned char color1[4] = {0, 0, 0, 255};
      unsigned char color2[4] = {0, 0, 0, 255};
      if (this->UseCurrentAnnotationColor)
      {
        if (ann->GetInformation()->Has(vtkAnnotation::COLOR()))
        {
          double* color = ann->GetInformation()->Get(vtkAnnotation::COLOR());
          color1[0] = static_cast<unsigned char>(255*color[0]);
          color1[1] = static_cast<unsigned char>(255*color[1]);
          color1[2] = static_cast<unsigned char>(255*color[2]);
        }
        if (ann->GetInformation()->Has(vtkAnnotation::OPACITY()))
        {
          double opacity = ann->GetInformation()->Get(vtkAnnotation::OPACITY());
          color1[3] = static_cast<unsigned char>(255*opacity);
        }
        for (int c = 0; c < 4; ++c)
        {
          color2[c] = color1[c];
        }
      }
      else
      {
        color1[0] = static_cast<unsigned char>(255*this->SelectedPointColor[0]);
        color1[1] = static_cast<unsigned char>(255*this->SelectedPointColor[1]);
        color1[2] = static_cast<unsigned char>(255*this->SelectedPointColor[2]);
        color1[3] = static_cast<unsigned char>(255*this->SelectedPointOpacity);
        color2[0] = static_cast<unsigned char>(255*this->SelectedCellColor[0]);
        color2[1] = static_cast<unsigned char>(255*this->SelectedCellColor[1]);
        color2[2] = static_cast<unsigned char>(255*this->SelectedCellColor[2]);
        color2[3] = static_cast<unsigned char>(255*this->SelectedCellOpacity);
      }
      if (graph)
      {
        vtkConvertSelection::GetSelectedVertices(selection, graph, list1);
        vtkConvertSelection::GetSelectedEdges(selection, graph, list2);
      }
      else if (dataSet)
      {
        vtkConvertSelection::GetSelectedPoints(selection, dataSet, list1);
        vtkConvertSelection::GetSelectedCells(selection, dataSet, list2);
      }
      else
      {
        vtkConvertSelection::GetSelectedRows(selection, table, list1);
      }
      vtkIdType numIds = list1->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIds; ++i)
      {
        if (list1->GetValue(i) >= colorArr1->GetNumberOfTuples())
        {
          continue;
        }
        colorArr1->SetTypedTuple(list1->GetValue(i), color1);
      }
      numIds = list2->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numIds; ++i)
      {
        if (list2->GetValue(i) >= colorArr2->GetNumberOfTuples())
        {
          continue;
        }
        colorArr2->SetTypedTuple(list2->GetValue(i), color2);
      }
    }
  } // end if (layers)

  return 1;
}

void vtkApplyColors::ProcessColorArray(
  vtkUnsignedCharArray* colorArr,
  vtkScalarsToColors* lut,
  vtkAbstractArray* arr,
  unsigned char color[4],
  bool scaleToArray)
{
  if (lut && arr)
  {
    // If scaling is on, use data min/max.
    // Otherwise, use the lookup table range.
    const double* rng = lut->GetRange();
    double minVal = rng[0];
    double maxVal = rng[1];
    if (scaleToArray)
    {
      minVal = VTK_DOUBLE_MAX;
      maxVal = VTK_DOUBLE_MIN;
      for (vtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
      {
        double val = arr->GetVariantValue(i).ToDouble();
        if (val > maxVal)
        {
          maxVal = val;
        }
        if (val < minVal)
        {
          minVal = val;
        }
      }
    }

    // Map the data values through the lookup table.
    double scale = 1.0;
    if (minVal != maxVal)
    {
      scale = (rng[1]-rng[0])/(maxVal-minVal);
    }
    unsigned char myColor[4] = {0, 0, 0, 0};
    for (vtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
    {
      double val = arr->GetVariantValue(i).ToDouble();
      const unsigned char* mappedColor = lut->MapValue(
        rng[0] + scale*(val - minVal));
      myColor[0] = mappedColor[0];
      myColor[1] = mappedColor[1];
      myColor[2] = mappedColor[2];
      // Combine the opacity of the lookup table with the
      // default color opacity.
      myColor[3] = static_cast<unsigned char>((color[3]/255.0)*mappedColor[3]);
      colorArr->SetTypedTuple(i, myColor);
    }
  }
  else
  {
    // If no lookup table, use default color.
    for (vtkIdType i = 0; i < colorArr->GetNumberOfTuples(); ++i)
    {
      colorArr->SetTypedTuple(i, color);
    }
  }
}

vtkMTimeType vtkApplyColors::GetMTime()
{
  vtkMTimeType mtime = Superclass::GetMTime();
  if (this->PointLookupTable && this->PointLookupTable->GetMTime() > mtime)
  {
    mtime = this->PointLookupTable->GetMTime();
  }
  if (this->CellLookupTable && this->CellLookupTable->GetMTime() > mtime)
  {
    mtime = this->CellLookupTable->GetMTime();
  }
  return mtime;
}

void vtkApplyColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PointLookupTable: "
     << (this->PointLookupTable ? "" : "(none)") << endl;
  if (this->PointLookupTable)
  {
    this->PointLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "CellLookupTable: "
     << (this->CellLookupTable ? "" : "(none)") << endl;
  if (this->CellLookupTable)
  {
    this->CellLookupTable->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "DefaultPointColor: "
    << this->DefaultPointColor[0] << ","
    << this->DefaultPointColor[1] << ","
    << this->DefaultPointColor[2] << endl;
  os << indent << "DefaultPointOpacity: " << this->DefaultPointOpacity << endl;
  os << indent << "DefaultCellColor: "
    << this->DefaultCellColor[0] << ","
    << this->DefaultCellColor[1] << ","
    << this->DefaultCellColor[2] << endl;
  os << indent << "DefaultCellOpacity: " << this->DefaultCellOpacity << endl;
  os << indent << "SelectedPointColor: "
    << this->SelectedPointColor[0] << ","
    << this->SelectedPointColor[1] << ","
    << this->SelectedPointColor[2] << endl;
  os << indent << "SelectedPointOpacity: " << this->SelectedPointOpacity << endl;
  os << indent << "SelectedCellColor: "
    << this->SelectedCellColor[0] << ","
    << this->SelectedCellColor[1] << ","
    << this->SelectedCellColor[2] << endl;
  os << indent << "SelectedCellOpacity: " << this->SelectedCellOpacity << endl;
  os << indent << "ScalePointLookupTable: "
    << (this->ScalePointLookupTable ? "on" : "off") << endl;
  os << indent << "ScaleCellLookupTable: "
    << (this->ScaleCellLookupTable ? "on" : "off") << endl;
  os << indent << "UsePointLookupTable: "
    << (this->UsePointLookupTable ? "on" : "off") << endl;
  os << indent << "UseCellLookupTable: "
    << (this->UseCellLookupTable ? "on" : "off") << endl;
  os << indent << "PointColorOutputArrayName: "
    << (this->PointColorOutputArrayName ? this->PointColorOutputArrayName : "(none)") << endl;
  os << indent << "CellColorOutputArrayName: "
    << (this->CellColorOutputArrayName ? this->CellColorOutputArrayName : "(none)") << endl;
  os << indent << "UseCurrentAnnotationColor: "
    << (this->UseCurrentAnnotationColor ? "on" : "off") << endl;
}
