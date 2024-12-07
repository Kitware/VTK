// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLengthDistribution.h"

#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkReservoirSampler.h"
#include "vtkSMPTools.h"
#include "vtkSortDataArray.h"
#include "vtkTable.h"
#include "vtkVector.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLengthDistribution);

void vtkLengthDistribution::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SampleSize: " << this->SampleSize << "\n";
  os << indent << "SortSample: " << (this->SortSample ? "T" : "F") << "\n";
}

double vtkLengthDistribution::GetLengthQuantile(double qq)
{
  if (!this->SortSample)
  {
    throw std::logic_error("The length distribution is not a CDF.");
  }
  auto* table = this->GetOutput();
  auto* lengths =
    vtkDoubleArray::SafeDownCast(table ? table->GetColumnByName("cell length") : nullptr);
  if (!lengths)
  {
    throw std::logic_error("A length distribution is not available.");
  }
  if (qq < 0.0 || qq > 1.0)
  {
    throw std::invalid_argument("The quantile must be a number in [0.0, 1.0].");
  }
  vtkIdType entry = qq * (lengths->GetNumberOfTuples() - 1);
  if (entry == lengths->GetNumberOfTuples())
  {
    --entry;
  }
  return lengths->GetTuple1(entry);
}

int vtkLengthDistribution::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

int vtkLengthDistribution::RequestData(
  vtkInformation* /* request */, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  auto* dataIn = vtkDataSet::GetData(inInfo[0]);
  auto* table = vtkTable::GetData(outInfo);
  if (!dataIn)
  {
    vtkErrorMacro("No input data.");
    return 1;
  }

  // Prepare the table
  table->Initialize();
  table->RemoveAllColumns();
  vtkIdType numSamples =
    dataIn->GetNumberOfCells() < this->SampleSize ? dataIn->GetNumberOfCells() : this->SampleSize;
  vtkNew<vtkDoubleArray> lengths;
  lengths->SetNumberOfTuples(numSamples);
  lengths->SetName("cell length");
  table->AddColumn(lengths);

  // Call once from main thread to ensure thread-safe operation.
  vtkNew<vtkIdList> dummyIds;
  dataIn->GetCellPoints(0, dummyIds.GetPointer());

  vtkReservoirSampler<vtkIdType> sampler;
  std::vector<vtkIdType> ids = sampler(numSamples, dataIn->GetNumberOfCells());
  vtkSMPTools::For(0, static_cast<vtkIdType>(ids.size()),
    [&dataIn, &lengths, &sampler, &ids](vtkIdType begin, vtkIdType end)
    {
      vtkNew<vtkIdList> points;
      std::array<vtkVector3d, 2> endpoints;
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        vtkIdType cellId = ids[ii];
        dataIn->GetCellPoints(cellId, points);
        int ee = 0;
        for (const auto& connIdx : sampler(2, points->GetNumberOfIds()))
        {
          dataIn->GetPoint(points->GetId(connIdx), endpoints[ee].GetData());
          ++ee;
        }
        if (ee == 2)
        {
          lengths->SetTuple1(ii, (endpoints[1] - endpoints[0]).Norm());
        }
        else
        {
          lengths->SetTuple1(ii, 0.0); // point cells, degenerate cells, etc.
        }
      }
    });

  if (this->SortSample)
  {
    vtkSortDataArray::Sort(lengths);
  }
  return 1;
}
VTK_ABI_NAMESPACE_END
