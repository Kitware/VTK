// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef HDFTestUtilities_h
#define HDFTestUtilities_h

#include "vtkDataAssemblyUtilities.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTableAlgorithm.h"

#include <array>
#include <map>
#include <numeric>
#include <vector>

namespace HDFTestUtilities
{
/**
 * Custom source to generate time-dependent HTG.
 * Set a list of descriptors (and optionally masks), and generate time steps out of it.
 */
class vtkHTGChangingDescriptorSource : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHTGChangingDescriptorSource* New();
  vtkTypeMacro(vtkHTGChangingDescriptorSource, vtkHyperTreeGridAlgorithm);

  void SetDescriptors(const std::vector<std::string>& descriptors)
  {
    this->Descriptors = descriptors;
  }
  void SetMasks(const std::vector<std::string>& masks) { this->Masks = masks; }
  void SetDimensions(const std::array<unsigned int, 3>& dimensions)
  {
    this->Dimensions = dimensions;
  }
  void SetBranchFactor(const unsigned int bf) { this->BranchFactor = bf; }

protected:
  int FillOutputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
    return 1;
  }

  vtkHTGChangingDescriptorSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    std::vector<double> timeSteps(this->Descriptors.size());
    std::iota(timeSteps.begin(), timeSteps.end(), 0);
    double timeRange[2] = { timeSteps.front(), timeSteps.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data(),
      static_cast<int>(timeSteps.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange,
      static_cast<int>(sizeof(timeRange) / sizeof(timeRange[0])));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
    return 1;
  }

  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override
  {
    intSource->SetDimensions(3, 3, 2);
    intSource->SetBranchFactor(2);
    intSource->SetMaxDepth(4);
    intSource->SetUseMask(!this->Masks.empty());
    intSource->SetDimensions(this->Dimensions.data());
    intSource->SetBranchFactor(BranchFactor);
    if (intSource->GetUseMask())
    {
      intSource->SetMask(this->Masks.at(static_cast<int>(this->RequestedTime)).c_str());
    }

    intSource->SetDescriptor(this->Descriptors.at(static_cast<int>(this->RequestedTime)).c_str());

    return 1;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    auto* output = vtkHyperTreeGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!output)
    {
      return 0;
    }

    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      RequestedTime = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

    this->ProcessTrees(nullptr, nullptr);

    typedef vtkStreamingDemandDrivenPipeline vtkSDDP;
    vtkNew<vtkInformation> reqs;
    if (outInfo->Has(vtkSDDP::UPDATE_PIECE_NUMBER()))
    {
      intSource->UpdatePiece(outInfo->Get(vtkSDDP::UPDATE_PIECE_NUMBER()),
        outInfo->Get(vtkSDDP::UPDATE_NUMBER_OF_PIECES()), 0);
    }
    else
    {
      intSource->Update();
    }

    output->ShallowCopy(intSource->GetOutputDataObject(0));

    return 1;
  }

private:
  vtkHTGChangingDescriptorSource(const vtkHTGChangingDescriptorSource&) = delete;
  void operator=(const vtkHTGChangingDescriptorSource&) = delete;

  std::vector<std::string> Descriptors;
  std::vector<std::string> Masks;
  std::array<unsigned int, 3> Dimensions;
  unsigned int BranchFactor = 2;

  double RequestedTime = 0.0;
  vtkNew<vtkHyperTreeGridSource> intSource;
};

/**
 * Custom source to generate time-dependent vtkTable.
 * Set columns as 2D arrays of double or in; dimension 0 is time, dimension 1 are rows.
 * Number of rows must be consistent for all columns of a given time step.
 */
class vtkTemporalTableSource : public vtkTableAlgorithm
{
public:
  static vtkTemporalTableSource* New();
  vtkTypeMacro(vtkTemporalTableSource, vtkTableAlgorithm);

  void AddTemporalColumn(const std::string& name, const std::vector<std::vector<int>>& values)
  {
    this->IntCols.insert({ name, values });
  }

  void AddTemporalColumn(const std::string& name, const std::vector<std::vector<double>>& values)
  {
    this->DoubleCols.insert({ name, values });
  }

protected:
  int FillOutputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
    return 1;
  }

  vtkTemporalTableSource()
  {
    this->SetNumberOfInputPorts(0);
    this->SetNumberOfOutputPorts(1);
  }

  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    std::vector<double> timeSteps(this->IntCols.begin()->second.size());
    std::iota(timeSteps.begin(), timeSteps.end(), 0);
    double timeRange[2] = { timeSteps.front(), timeSteps.back() };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), timeSteps.data(),
      static_cast<int>(timeSteps.size()));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange,
      static_cast<int>(sizeof(timeRange) / sizeof(timeRange[0])));
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION(), 1);
    outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
    return 1;
  }

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    auto* output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

    int RequestedTime =
      static_cast<int>(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()));

    for (const auto& it : IntCols)
    {
      vtkNew<vtkIntArray> arr;
      arr->SetName(it.first.c_str());
      arr->SetNumberOfTuples(it.second.at(RequestedTime).size());
      for (int i = 0; i < arr->GetNumberOfTuples(); i++)
      {
        arr->SetValue(i, it.second.at(RequestedTime).at(i));
      }
      output->AddColumn(arr);
    }

    for (const auto& it : DoubleCols)
    {
      vtkNew<vtkDoubleArray> arr;
      arr->SetName(it.first.c_str());
      arr->SetNumberOfTuples(it.second.at(RequestedTime).size());
      for (int i = 0; i < arr->GetNumberOfTuples(); i++)
      {
        arr->SetValue(i, it.second.at(RequestedTime).at(i));
      }
      output->AddColumn(arr);
    }

    return 1;
  }

private:
  vtkTemporalTableSource(const vtkTemporalTableSource&) = delete;
  void operator=(const vtkTemporalTableSource&) = delete;

  std::map<std::string, std::vector<std::vector<int>>> IntCols;
  std::map<std::string, std::vector<std::vector<double>>> DoubleCols;
};
}
#endif
