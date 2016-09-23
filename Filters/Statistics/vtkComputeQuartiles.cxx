/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkComputeQuartiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkComputeQuartiles.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOrderStatistics.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <sstream>
#include <string>

vtkStandardNewMacro(vtkComputeQuartiles);
//-----------------------------------------------------------------------------
vtkComputeQuartiles::vtkComputeQuartiles()
{
  this->SetInputArrayToProcess(0, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
    vtkDataSetAttributes::SCALARS);
  this->FieldAssociation = -1;
}

//-----------------------------------------------------------------------------
vtkComputeQuartiles::~vtkComputeQuartiles()
{
}

//-----------------------------------------------------------------------------
void vtkComputeQuartiles::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
int vtkComputeQuartiles::FillInputPortInformation (int port,
                                                 vtkInformation *info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkComputeQuartiles::GetInputFieldAssociation()
{
  vtkInformationVector *inArrayVec =
    this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);
  return inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
}

//-----------------------------------------------------------------------------
vtkFieldData* vtkComputeQuartiles::GetInputFieldData(vtkDataObject* input)
{
  if (!input)
  {
    vtkErrorMacro(<<"Cannot extract fields from null input");
    return 0;
  }

  if (vtkTable::SafeDownCast(input))
  {
    this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_ROWS;
  }

  if (this->FieldAssociation < 0)
  {
    this->FieldAssociation = this->GetInputFieldAssociation();
  }

  switch (this->FieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      return vtkDataSet::SafeDownCast(input)->GetPointData();

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      return vtkDataSet::SafeDownCast(input)->GetCellData();

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return input->GetFieldData();

    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      return vtkGraph::SafeDownCast(input)->GetVertexData();

    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      return vtkGraph::SafeDownCast(input)->GetEdgeData();

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return vtkTable::SafeDownCast(input)->GetRowData();
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkComputeQuartiles::RequestData(vtkInformation* /*request*/,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkTable* outputTable = vtkTable::GetData(outputVector, 0);

  vtkCompositeDataSet *cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    vtkCompositeDataIterator* iter = cdin->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet *o = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (o)
      {
        ComputeTable(o, outputTable, iter->GetCurrentFlatIndex());
      }
    }
  }
  else if (vtkDataObject *o = vtkDataObject::SafeDownCast(input))
  {
    ComputeTable(o, outputTable, -1);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkComputeQuartiles::ComputeTable(vtkDataObject* input,
                                       vtkTable* outputTable, vtkIdType blockId)
{
  vtkFieldData *field = this->GetInputFieldData(input);

  if (!field || field->GetNumberOfArrays() == 0)
  {
    vtkDebugMacro(<< "No field found!");
    return;
  }

  // Fill table for descriptive statistics input.
  vtkNew<vtkTable> inDescStats;
  vtkNew<vtkOrderStatistics> os;
  os->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, inDescStats.GetPointer());

  for (int i = 0; i < field->GetNumberOfArrays(); i++)
  {
    vtkDataArray *dataArray = field->GetArray(i);
    if (!dataArray || dataArray->GetNumberOfComponents() != 1)
    {
      vtkDebugMacro(<< "Field " << i << " empty or not scalar");
      continue;
    }

    // If field doesn't have a name, give a default one
    if (!dataArray->GetName())
    {
      std::ostringstream s;
      s << "Field " << i;
      dataArray->SetName(s.str().c_str());
    }
    inDescStats->AddColumn(dataArray);
    os->AddColumn(dataArray->GetName());
  }

  if (inDescStats->GetNumberOfColumns() == 0)
  {
    return;
  }

  os->SetLearnOption(true);
  os->SetDeriveOption(true);
  os->SetTestOption(false);
  os->SetAssessOption(false);
  os->Update();

  // Get the ouput table of the descriptive statistics that contains quantiles
  // of the input data series.
  vtkMultiBlockDataSet *outputModelDS =
    vtkMultiBlockDataSet::SafeDownCast(
    os->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
  unsigned nbq = outputModelDS->GetNumberOfBlocks() - 1;
  vtkTable* outputQuartiles =
    vtkTable::SafeDownCast(outputModelDS->GetBlock(nbq));
  if (!outputQuartiles || outputQuartiles->GetNumberOfColumns() < 2)
  {
    return;
  }

  vtkIdType currLen = outputTable->GetNumberOfColumns();
  vtkIdType outLen = outputQuartiles->GetNumberOfColumns() - 1;

  // Fill the table
  for (int j = 0; j < outLen; j++)
  {
    vtkNew<vtkDoubleArray> ncol;
    ncol->SetNumberOfComponents(1);
    ncol->SetNumberOfValues(5);
    outputTable->AddColumn(ncol.GetPointer());
    if (blockId >= 0)
    {
      std::stringstream ss;
      ss << inDescStats->GetColumnName(j) << "_Block_" << blockId;
      ncol->SetName(ss.str().c_str());
    }
    else
    {
      ncol->SetName(inDescStats->GetColumnName(j));
    }

    vtkAbstractArray *col = outputQuartiles->GetColumnByName(inDescStats->GetColumnName(j));
    for (int k = 0; k < 5; k++)
    {
      outputTable->SetValue(k, currLen + j, col ? col->GetVariantValue(k).ToDouble() : 0.0);
    }
  }
}
