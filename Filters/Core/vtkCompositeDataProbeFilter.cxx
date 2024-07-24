// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataProbeFilter.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkFindCellStrategy.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridProbeFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCompositeDataProbeFilter);
//------------------------------------------------------------------------------
vtkCompositeDataProbeFilter::vtkCompositeDataProbeFilter()
{
  this->PassPartialArrays = false;
}

//------------------------------------------------------------------------------
vtkCompositeDataProbeFilter::~vtkCompositeDataProbeFilter() = default;

//------------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 1)
  {
    // We have to save vtkDataObject since this filter can work on vtkDataSet
    // and vtkCompositeDataSet consisting of vtkDataSet leaf nodes.
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkExecutive* vtkCompositeDataProbeFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* sourceDS = vtkDataSet::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkCompositeDataSet* sourceComposite =
    vtkCompositeDataSet::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkHyperTreeGrid* sourceHTG =
    vtkHyperTreeGrid::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    return 0;
  }

  if (!sourceDS && !sourceComposite && !sourceHTG)
  {
    vtkErrorMacro("vtkDataSet, vtkCompositeDataSet or vtkHyperTreeGrid is expected as the input "
                  "on port 1");
    return 0;
  }

  if (sourceDS)
  {
    // Superclass knowns exactly what to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  if (sourceHTG)
  {
    vtkNew<vtkHyperTreeGridProbeFilter> htgProbe;
    htgProbe->SetContainerAlgorithm(this);
    htgProbe->SetPassCellArrays(this->GetPassCellArrays());
    htgProbe->SetPassPointArrays(this->GetPassPointArrays());
    htgProbe->SetPassFieldArrays(this->GetPassFieldArrays());
    htgProbe->SetValidPointMaskArrayName(this->GetValidPointMaskArrayName());
    htgProbe->SetInputData(input);
    htgProbe->SetSourceData(sourceHTG);
    htgProbe->SetTolerance(this->Tolerance);
    htgProbe->SetComputeTolerance(this->ComputeTolerance);
    htgProbe->SetUseImplicitArrays(this->UseImplicitArrays);
    htgProbe->Update();
    vtkDataSet* htgOutput = htgProbe->GetOutput();
    output->ShallowCopy(htgOutput);

    // Need to copy the MaskPoints of the htg prober
    // so that it can be exploited in the pipeline with the
    // vtkProbeFilter::GetValidPoints method.
    vtkCharArray* maskArray = vtkCharArray::SafeDownCast(
      htgOutput->GetPointData()->GetArray(this->GetValidPointMaskArrayName()));
    if (this->MaskPoints == nullptr)
    {
      this->MaskPoints = vtkCharArray::New();
    }
    this->MaskPoints->ShallowCopy(maskArray);
    return 1;
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->BuildFieldList(sourceComposite))
  {
    this->InitializeForProbing(input, output);

    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(sourceComposite->NewIterator());
    // We do reverse traversal, so that for hierarchical datasets, we traverse the
    // higher resolution blocks first.
    int idx = 0;
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (this->CheckAbort())
      {
        break;
      }
      sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      sourceHTG = vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject());
      if (!sourceDS && !sourceHTG)
      {
        vtkErrorMacro(
          "All leaves in the multiblock dataset must either be vtkDataSet or vtkHyperTreeGrid.");
        return 0;
      }

      if (sourceHTG)
      {
        vtkNew<vtkHyperTreeGridProbeFilter> htgProbe;
        htgProbe->SetContainerAlgorithm(this);
        htgProbe->SetPassCellArrays(this->GetPassCellArrays());
        htgProbe->SetPassPointArrays(this->GetPassPointArrays());
        htgProbe->SetPassFieldArrays(this->GetPassFieldArrays());
        htgProbe->SetValidPointMaskArrayName(this->GetValidPointMaskArrayName());
        htgProbe->SetInputData(input);
        htgProbe->SetTolerance(this->Tolerance);
        htgProbe->SetComputeTolerance(this->ComputeTolerance);
        htgProbe->SetSourceData(sourceHTG);
        htgProbe->Update();
        // merge the output for this block with the total output
        vtkNew<vtkIdList> addPoints;
        addPoints->Initialize();
        vtkDataSet* locOutput = htgProbe->GetOutput();
        vtkCharArray* locMask = vtkCharArray::SafeDownCast(
          locOutput->GetPointData()->GetArray(this->GetValidPointMaskArrayName()));
        auto locPointMaskRange = vtk::DataArrayValueRange<1>(locMask);
        auto globPointMaskRange = vtk::DataArrayValueRange<1>(this->MaskPoints);
        auto locIt = locPointMaskRange.begin();
        auto globIt = globPointMaskRange.begin();
        vtkIdType index = 0;
        for (; (locIt != locPointMaskRange.end()) && (globIt != globPointMaskRange.end());
             locIt++, globIt++, index++)
        {
          // if the global mask does not have the point but the local one does then add the index
          if (!(*globIt) && (*locIt))
          {
            addPoints->InsertNextId(index);
            *globIt = *locIt; // Mark the global point as valid
          }
        }
        vtkIdType nArrays = sourceHTG->GetCellData()->GetNumberOfArrays();
        for (vtkIdType iA = 0; iA < nArrays; iA++)
        {
          const char* arrName = sourceHTG->GetCellData()->GetAbstractArray(iA)->GetName();
          vtkAbstractArray* locA = locOutput->GetPointData()->GetAbstractArray(arrName);
          if (!locA)
          {
            vtkGenericWarningMacro("Could not find array " << arrName << " in local scope output.");
            continue;
          }
          vtkAbstractArray* globA = output->GetPointData()->GetAbstractArray(arrName);
          if (!globA)
          {
            output->GetPointData()->AddArray(locA);
            continue;
          }
          globA->InsertTuples(addPoints, addPoints, locA);
        }
        continue;
      }

      if (sourceDS->GetNumberOfPoints() == 0)
      {
        continue;
      }

      auto strategyIt = this->StrategyMap.find(sourceDS);
      if (strategyIt != this->StrategyMap.end())
      {
        this->SetFindCellStrategy(strategyIt->second);
      }
      else
      {
        this->SetFindCellStrategy(nullptr);
      }

      this->InitializeSourceArrays(sourceDS);
      this->DoProbing(input, idx, sourceDS, output);
      idx++;
    }
  }

  this->PassAttributeData(input, sourceComposite, output);
  return 1;
}

//------------------------------------------------------------------------------
void vtkCompositeDataProbeFilter::InitializeOutputArrays(vtkPointData* outPD, vtkIdType numPts)
{
  if (!this->PassPartialArrays)
  {
    this->Superclass::InitializeOutputArrays(outPD, numPts);
  }
  else
  {
    for (int cc = 0; cc < outPD->GetNumberOfArrays(); cc++)
    {
      vtkDataArray* da = outPD->GetArray(cc);
      if (da)
      {
        da->SetNumberOfTuples(numPts);
        double null_value = 0.0;
        if (da->IsA("vtkDoubleArray") || da->IsA("vtkFloatArray"))
        {
          null_value = vtkMath::Nan();
        }
        da->Fill(null_value);
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkCompositeDataProbeFilter::BuildFieldList(vtkCompositeDataSet* source)
{
  delete this->PointList;
  delete this->CellList;
  this->PointList = nullptr;
  this->CellList = nullptr;

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(source->NewIterator());

  int numDatasets = 0;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()))
    {
      continue;
    }
    if (!sourceDS)
    {
      vtkErrorMacro("All leaves in the multiblock dataset must be vtkDataSet.");
      return 0;
    }
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    numDatasets++;
  }

  this->PointList = new vtkDataSetAttributes::FieldList(numDatasets);
  this->CellList = new vtkDataSetAttributes::FieldList(numDatasets);

  bool initializedPD = false;
  bool initializedCD = false;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* sourceDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (vtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject()))
    {
      continue;
    }
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    if (!initializedPD)
    {
      this->PointList->InitializeFieldList(sourceDS->GetPointData());
      initializedPD = true;
    }
    else
    {
      if (this->PassPartialArrays)
      {
        this->PointList->UnionFieldList(sourceDS->GetPointData());
      }
      else
      {
        this->PointList->IntersectFieldList(sourceDS->GetPointData());
      }
    }

    if (sourceDS->GetNumberOfCells() > 0)
    {
      if (!initializedCD)
      {
        this->CellList->InitializeFieldList(sourceDS->GetCellData());
        initializedCD = true;
      }
      else
      {
        if (this->PassPartialArrays)
        {
          this->CellList->UnionFieldList(sourceDS->GetCellData());
        }
        else
        {
          this->CellList->IntersectFieldList(sourceDS->GetCellData());
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkCompositeDataProbeFilter::SetFindCellStrategyMap(
  const std::map<vtkDataSet*, vtkSmartPointer<vtkFindCellStrategy>>& map)
{
  for (const auto& keyVal : map)
  {
    auto it = this->StrategyMap.find(keyVal.first);
    if (it == this->StrategyMap.end() || it->second.GetPointer() != keyVal.second.GetPointer())
    {
      this->StrategyMap = map;
      this->Modified();
      return;
    }
  }
}

//------------------------------------------------------------------------------
void vtkCompositeDataProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "PassPartialArrays: " << this->PassPartialArrays << endl;
}
VTK_ABI_NAMESPACE_END
