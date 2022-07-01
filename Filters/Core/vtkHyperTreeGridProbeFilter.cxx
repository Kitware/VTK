/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridProbeFilter.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridLocator.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <numeric>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridProbeFilter);

//------------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(vtkHyperTreeGridProbeFilter, Locator, vtkHyperTreeGridLocator);

//------------------------------------------------------------------------------
vtkHyperTreeGridProbeFilter::vtkHyperTreeGridProbeFilter()
  : Locator(vtkSmartPointer<vtkHyperTreeGridGeometricLocator>::New())
  , ValidPointMaskArrayName(nullptr)
  , ValidPoints(vtkSmartPointer<vtkIdTypeArray>::New())
  , MaskPoints(nullptr)
{
  this->SetNumberOfInputPorts(2);
  this->SetValidPointMaskArrayName("vtkValidPointMask");
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Locator)
  {
    os << indent << "Locator: ";
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: none\n";
  }
  os << indent << "PassCellArrays: " << (this->PassCellArrays ? "On\n" : "Off\n");
  os << indent << "PassPointArrays: " << (this->PassPointArrays ? "On\n" : "Off\n");
  os << indent << "PassFieldArrays: " << (this->PassFieldArrays ? "On\n" : "Off\n");
}

//------------------------------------------------------------------------------
vtkHyperTreeGridLocator* vtkHyperTreeGridProbeFilter::GetLocator()
{
  return this->Locator;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridProbeFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridProbeFilter::SetSourceData(vtkHyperTreeGrid* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return vtkHyperTreeGrid::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridProbeFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridProbeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->UpdateProgress(0.0);

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkHyperTreeGrid* source =
    vtkHyperTreeGrid::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!input || !source || !output)
  {
    vtkErrorMacro("Could not get either the input, source or output");
    return 0;
  }

  // setup output
  if (!(this->Initialize(input, source, output)))
  {
    vtkErrorMacro("Could not initialize output arrays");
    return 0;
  }

  this->UpdateProgress(0.1);

  vtkNew<vtkIdList> localPointIds;
  localPointIds->Initialize();
  // run probing on each source individually
  if (!(this->DoProbing(input, source, output, localPointIds)))
  {
    vtkErrorMacro("Could not perform serial probing correctly");
    return 0;
  }

  this->UpdateProgress(0.7);

  // gather and sort results
  if (!(this->Reduce(source, output, localPointIds)))
  {
    vtkErrorMacro("Failed to reduce results");
    return 0;
  }

  this->UpdateProgress(1.0);
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridProbeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  return 1;
}

//------------------------------------------------------------------------------
class vtkHyperTreeGridProbeFilter::ProbingWorklet
{
public:
  vtkSmartPointer<vtkHyperTreeGridLocator> Locator;
  vtkSmartPointer<vtkDataSet> Probe;
  vtkSmartPointer<vtkIdList> ThreadGlobPointIds;
  vtkSmartPointer<vtkIdList> ThreadGlobCellIds;
  struct LocalData
  {
    std::vector<vtkIdType> pointIds;
    std::vector<vtkIdType> cellIds;
  };
  vtkSMPThreadLocal<LocalData> ThreadLocal;

  ProbingWorklet(vtkSmartPointer<vtkDataSet> probe,
    vtkSmartPointer<vtkHyperTreeGridLocator> locator, vtkSmartPointer<vtkIdList> pointIds,
    vtkSmartPointer<vtkIdList> cellIds)
  {
    this->Probe = probe;
    this->Locator = locator;
    this->ThreadGlobPointIds = pointIds;
    this->ThreadGlobCellIds = cellIds;
  }

  void Initialize()
  {
    this->ThreadLocal.Local().pointIds = std::vector<vtkIdType>();
    this->ThreadLocal.Local().cellIds = std::vector<vtkIdType>();
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    std::vector<double> pt(3, 0.0);
    for (vtkIdType iP = begin; iP < end; iP++)
    {
      this->Probe->GetPoint(iP, pt.data());
      vtkIdType id = this->Locator->Search(pt.data());
      if (!(id < 0))
      {
        this->ThreadLocal.Local().pointIds.emplace_back(iP);
        this->ThreadLocal.Local().cellIds.emplace_back(id);
      }
    }
  }

  void Reduce()
  {
    vtkIdType nPointsFound = 0;
    for (auto it = this->ThreadLocal.begin(); it != this->ThreadLocal.end(); it++)
    {
      nPointsFound += it->pointIds.size();
    }
    this->ThreadGlobPointIds->SetNumberOfIds(nPointsFound);
    this->ThreadGlobCellIds->SetNumberOfIds(nPointsFound);
    nPointsFound = 0;

    auto mergeThreadResults = [&](LocalData& loc) {
      std::copy(
        loc.pointIds.begin(), loc.pointIds.end(), this->ThreadGlobPointIds->begin() + nPointsFound);
      std::copy(
        loc.cellIds.begin(), loc.cellIds.end(), this->ThreadGlobCellIds->begin() + nPointsFound);
      nPointsFound += loc.pointIds.size();
      loc.pointIds.resize(0);
      loc.cellIds.resize(0);
    };
    std::for_each(this->ThreadLocal.begin(), this->ThreadLocal.end(), mergeThreadResults);
  }
};

//------------------------------------------------------------------------------
bool vtkHyperTreeGridProbeFilter::DoProbing(
  vtkDataSet* probe, vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds)
{
  // locate all present points of probe
  unsigned int nPoints = probe->GetNumberOfPoints();
  vtkNew<vtkIdList> locCellIds;
  locCellIds->Initialize();
  ProbingWorklet worker(probe, this->Locator, localPointIds, locCellIds);
  vtkSMPTools::For(0, nPoints, worker);

  // copy values from source
  unsigned int numSourceCellArrays = source->GetCellData()->GetNumberOfArrays();
  for (unsigned int iA = 0; iA < numSourceCellArrays; iA++)
  {
    vtkDataArray* sourceArray = source->GetCellData()->GetArray(iA);
    if (!(output->GetPointData()->HasArray(sourceArray->GetName())))
    {
      vtkErrorMacro("Array " << sourceArray->GetName() << " missing in output");
      return false;
    }
    vtkDataArray* outputArray = output->GetPointData()->GetArray(sourceArray->GetName());
    outputArray->InsertTuplesStartingAt(0, locCellIds, sourceArray);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridProbeFilter::Initialize(
  vtkDataSet* input, vtkHyperTreeGrid* source, vtkDataSet* output)
{
  output->Initialize();

  output->CopyStructure(input);

  if (!(this->PassAttributeData(input, output)))
  {
    vtkErrorMacro("Failed to pass attribute data from inpu to output");
    return false;
  }

  unsigned int numSourceCellArrays = source->GetCellData()->GetNumberOfArrays();
  for (unsigned int iA = 0; iA < numSourceCellArrays; iA++)
  {
    vtkDataArray* da = source->GetCellData()->GetArray(iA);
    if (!(output->GetPointData()->HasArray(da->GetName())))
    {
      auto localInstance = vtk::TakeSmartPointer(da->NewInstance());
      localInstance->SetName(da->GetName());
      localInstance->SetNumberOfComponents(da->GetNumberOfComponents());
      output->GetPointData()->AddArray(localInstance);
      localInstance->Initialize();
    }
  }

  this->Locator->SetHTG(source);

  // if this is repeatedly called by the pipeline for a composite mesh,
  // you need a new array for each block
  // (that is you need to reinitialize the object)
  this->MaskPoints = vtk::TakeSmartPointer(vtkCharArray::New());
  this->MaskPoints->SetNumberOfComponents(1);
  this->MaskPoints->SetNumberOfTuples(input->GetNumberOfPoints());
  this->FillDefaultArray(this->MaskPoints);
  this->MaskPoints->SetName(
    this->ValidPointMaskArrayName ? this->ValidPointMaskArrayName : "vtkValidPointMask");
  output->GetPointData()->AddArray(this->MaskPoints);

  return true;
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridProbeFilter::Reduce(
  vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds)
{
  vtkIdType numPointsFound = localPointIds->GetNumberOfIds();

  auto dealWithRemote = [&](vtkIdList* remotePointIds, vtkDataSet* remoteOutput,
                          vtkHyperTreeGrid* source, vtkDataSet* totOutput) {
    if (remotePointIds->GetNumberOfIds() > 0)
    {
      vtkNew<vtkIdList> iotaIds;
      iotaIds->SetNumberOfIds(remotePointIds->GetNumberOfIds());
      std::iota(iotaIds->begin(), iotaIds->end(), 0);
      unsigned int numArrays = source->GetCellData()->GetNumberOfArrays();
      for (unsigned int iA = 0; iA < numArrays; iA++)
      {
        vtkDataArray* remoteArray =
          remoteOutput->GetPointData()->GetArray(source->GetCellData()->GetArray(iA)->GetName());
        vtkDataArray* totArray =
          totOutput->GetPointData()->GetArray(source->GetCellData()->GetArray(iA)->GetName());
        totArray->InsertTuples(remotePointIds, iotaIds, remoteArray);
      }
      vtkNew<vtkCharArray> ones;
      ones->SetNumberOfComponents(1);
      ones->SetNumberOfTuples(remotePointIds->GetNumberOfIds());
      auto range = vtk::DataArrayValueRange<1>(ones);
      vtkSMPTools::Fill(range.begin(), range.end(), static_cast<char>(1));
      totOutput->GetPointData()
        ->GetArray(this->GetValidPointMaskArrayName())
        ->InsertTuples(remotePointIds, iotaIds, ones);
    }
  };
  vtkIdType numRemotePoints = 0;
  vtkSmartPointer<vtkDataSet> remoteOutput = vtk::TakeSmartPointer(output->NewInstance());
  vtkNew<vtkIdList> remotePointIds;
  // deal with master process
  remoteOutput->CopyStructure(output);
  unsigned int numArrays = source->GetCellData()->GetNumberOfArrays();
  for (unsigned int iA = 0; iA < numArrays; iA++)
  {
    vtkDataArray* da =
      output->GetPointData()->GetArray(source->GetCellData()->GetArray(iA)->GetName());
    auto localInstance = vtk::TakeSmartPointer(da->NewInstance());
    localInstance->DeepCopy(da);
    remoteOutput->GetPointData()->AddArray(localInstance);
    da->SetNumberOfTuples(output->GetNumberOfPoints());
    this->FillDefaultArray(da);
  }
  dealWithRemote(localPointIds, remoteOutput, source, output);
  remoteOutput->Initialize();

  return true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridProbeFilter::FillDefaultArray(vtkDataArray* array) const
{
  if (auto* strArray = vtkStringArray::SafeDownCast(array))
  {
    vtkSMPTools::For(0, strArray->GetNumberOfValues(), [strArray](vtkIdType start, vtkIdType end) {
      for (vtkIdType i = start; i < end; ++i)
      {
        strArray->SetValue(i, "");
      }
    });
  }
  else if (auto* doubleArray = vtkDoubleArray::SafeDownCast(array))
  {
    auto range = vtk::DataArrayValueRange(doubleArray);
    vtkSMPTools::Fill(range.begin(), range.end(), vtkMath::Nan());
  }
  else if (auto* floatArray = vtkFloatArray::SafeDownCast(array))
  {
    auto range = vtk::DataArrayValueRange(floatArray);
    vtkSMPTools::Fill(range.begin(), range.end(), vtkMath::Nan());
  }
  else
  {
    auto range = vtk::DataArrayValueRange(array);
    vtkSMPTools::Fill(range.begin(), range.end(), 0);
  }
}

//------------------------------------------------------------------------------
// Straight up copy from vtkProbeFilter
bool vtkHyperTreeGridProbeFilter::PassAttributeData(vtkDataSet* input, vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
  {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; ++i)
    {
      vtkDataArray* da = input->GetPointData()->GetArray(i);
      if (!output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      vtkDataArray* da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      vtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
  return true;
}

//------------------------------------------------------------------------------
// Straight up copy from vtkProbeFilter
vtkIdTypeArray* vtkHyperTreeGridProbeFilter::GetValidPoints()
{
  if (this->MaskPoints && this->MaskPoints->GetMTime() > this->ValidPoints->GetMTime())
  {
    char* maskArray = this->MaskPoints->GetPointer(0);
    vtkIdType numPts = this->MaskPoints->GetNumberOfTuples();
    vtkIdType numValidPoints = std::count(maskArray, maskArray + numPts, static_cast<char>(1));
    this->ValidPoints->Allocate(numValidPoints);
    for (vtkIdType i = 0; i < numPts; ++i)
    {
      if (maskArray[i])
      {
        this->ValidPoints->InsertNextValue(i);
      }
    }
    this->ValidPoints->Modified();
  }

  return this->ValidPoints;
}
