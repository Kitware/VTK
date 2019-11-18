/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkmHistogram.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"

#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include "vtkmFilterPolicy.h"
#include <vtkm/cont/ArrayRangeCompute.hxx>
#include <vtkm/filter/Histogram.h>

vtkStandardNewMacro(vtkmHistogram);

//------------------------------------------------------------------------------
vtkmHistogram::vtkmHistogram()
{
  this->CustomBinRange[0] = 0;
  this->CustomBinRange[0] = 100;
  this->UseCustomBinRanges = false;
  this->CenterBinsAroundMinAndMax = false;
  this->NumberOfBins = 10;
}

//------------------------------------------------------------------------------
vtkmHistogram::~vtkmHistogram() {}

//-----------------------------------------------------------------------------
int vtkmHistogram::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkmHistogram::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkTable* output = vtkTable::GetData(outputVector, 0);
  output->Initialize();

  // These are the mid-points for each of the bins
  vtkSmartPointer<vtkDoubleArray> binExtents = vtkSmartPointer<vtkDoubleArray>::New();
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(static_cast<vtkIdType>(this->NumberOfBins));
  binExtents->SetName("bin_extents");
  binExtents->FillComponent(0, 0.0);

  // Grab the input array to process to determine the field we want to apply histogram
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if ((association != vtkDataObject::FIELD_ASSOCIATION_POINTS &&
        association != vtkDataObject::FIELD_ASSOCIATION_CELLS) ||
    fieldArray == nullptr || fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    vtkErrorMacro(<< "Invalid field: Requires a point or cell field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    vtkm::cont::DataSet in = tovtkm::Convert(input);
    auto field = tovtkm::Convert(fieldArray, association);
    in.AddField(field);

    vtkmInputFilterPolicy policy;
    vtkm::filter::Histogram filter;

    filter.SetNumberOfBins(static_cast<vtkm::Id>(this->NumberOfBins));
    filter.SetActiveField(fieldName, field.GetAssociation());
    if (this->UseCustomBinRanges)
    {
      if (this->CustomBinRange[0] > this->CustomBinRange[1])
      {
        vtkWarningMacro("Custom bin range adjusted to keep min <= max value");
        double min = this->CustomBinRange[1];
        double max = this->CustomBinRange[0];
        this->CustomBinRange[0] = min;
        this->CustomBinRange[1] = max;
      }
      filter.SetRange(vtkm::Range(this->CustomBinRange[0], this->CustomBinRange[1]));
    }
    auto result = filter.Execute(in, policy);
    this->BinDelta = filter.GetBinDelta();
    this->ComputedRange[0] = filter.GetComputedRange().Min;
    this->ComputedRange[1] = filter.GetComputedRange().Max;

    // Convert the result back
    vtkDataArray* resultingArray = fromvtkm::Convert(result.GetField("histogram"));
    resultingArray->SetName("bin_values");
    if (resultingArray == nullptr)
    {
      vtkErrorMacro(<< "Unable to convert result array from VTK-m to VTK");
      return 0;
    }
    this->FillBinExtents(binExtents);
    output->GetRowData()->AddArray(binExtents);
    output->GetRowData()->AddArray(resultingArray);

    resultingArray->FastDelete();
  }
  catch (const vtkm::cont::Error& e)
  {
    vtkErrorMacro(<< "VTK-m error: " << e.GetMessage());
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkmHistogram::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << NumberOfBins << "\n";
  os << indent << "UseCustomBinRanges: " << UseCustomBinRanges << "\n";
  os << indent << "CenterBinsAroundMinAndMax: " << CenterBinsAroundMinAndMax << "\n";
  os << indent << "CustomBinRange: " << CustomBinRange[0] << ", " << CustomBinRange[1] << "\n";
}

//------------------------------------------------------------------------------
void vtkmHistogram::FillBinExtents(vtkDoubleArray* binExtents)
{
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(static_cast<vtkIdType>(this->NumberOfBins));
  double binDelta = this->CenterBinsAroundMinAndMax
    ? ((this->ComputedRange[1] - this->ComputedRange[0]) / (this->NumberOfBins - 1))
    : this->BinDelta;
  double halfBinDelta = binDelta / 2.0;
  for (vtkIdType i = 0; i < static_cast<vtkIdType>(this->NumberOfBins); i++)
  {
    binExtents->SetValue(i,
      this->ComputedRange[0] + (i * binDelta) +
        (this->CenterBinsAroundMinAndMax ? 0.0 : halfBinDelta));
  }
}
