/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridPProbeFilter.h"

#include <numeric>
#include <vector>

#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkAbstractArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometricLocator.h"
#include "vtkHyperTreeGridLocator.h"
#include "vtkIdList.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkHyperTreeGridPProbeFilter);

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkHyperTreeGridPProbeFilter, Controller, vtkMultiProcessController);

//------------------------------------------------------------------------------
vtkCxxSetSmartPointerMacro(vtkHyperTreeGridPProbeFilter, Locator, vtkHyperTreeGridLocator);

//------------------------------------------------------------------------------
vtkHyperTreeGridPProbeFilter::vtkHyperTreeGridPProbeFilter()
  : PassCellArrays(false)
  , PassPointArrays(false)
  , PassFieldArrays(true)
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  vtkNew<vtkHyperTreeGridGeometricLocator> thisLocator;
  this->SetLocator(thisLocator); // default to GeometricLocator
  this->SetNumberOfInputPorts(2);
} // vtkHyperTreeGridPProbeFilter

//------------------------------------------------------------------------------
vtkHyperTreeGridPProbeFilter::~vtkHyperTreeGridPProbeFilter()
{
  this->SetController(nullptr);
  this->SetLocator(nullptr);
} //~vtkHyperTreeGridPProbeFilter

//------------------------------------------------------------------------------
vtkHyperTreeGridLocator* vtkHyperTreeGridPProbeFilter::GetLocator()
{
  return this->Locator;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridPProbeFilter::FillInputPortInformation(int port, vtkInformation* info)
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
} // FillInputPortInformation

//------------------------------------------------------------------------------
void vtkHyperTreeGridPProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
} // SetSourceConnection

//------------------------------------------------------------------------------
void vtkHyperTreeGridPProbeFilter::SetSourceData(vtkHyperTreeGrid* input)
{
  this->SetInputData(1, input);
} // SetSourceData

//------------------------------------------------------------------------------
vtkHyperTreeGrid* vtkHyperTreeGridPProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return vtkHyperTreeGrid::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
} // GetSource

//------------------------------------------------------------------------------
int vtkHyperTreeGridPProbeFilter::RequestInformation(vtkInformation* vtkNotUsed(request),
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
} // RequestInformation

//------------------------------------------------------------------------------
int vtkHyperTreeGridPProbeFilter::RequestData(vtkInformation* vtkNotUsed(request),
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

  if (!input)
  {
    vtkErrorMacro("Could not get vtkDataSet input");
    return 0;
  }
  if (!source)
  {
    vtkErrorMacro("Could not get vtkHyperTreeGrid source");
    return 0;
  }
  if (!output)
  {
    vtkErrorMacro("Could not get output");
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

  // gather the results to the master process
  if (!(this->Reduce(source, output, localPointIds)))
  {
    vtkErrorMacro("Failed to communicate results to master process");
    return 0;
  }

  this->UpdateProgress(1.0);
  return 1;
} // RequestData

//------------------------------------------------------------------------------
int vtkHyperTreeGridPProbeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  return 1;
} // RequestUpdateExtent

//------------------------------------------------------------------------------
class vtkHyperTreeGridPProbeFilter::ProbingWorklet
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
        this->ThreadLocal.Local().pointIds.push_back(iP);
        this->ThreadLocal.Local().cellIds.push_back(id);
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
    for (auto it = this->ThreadLocal.begin(); it != this->ThreadLocal.end(); it++)
    {
      std::copy(
        it->pointIds.begin(), it->pointIds.end(), this->ThreadGlobPointIds->begin() + nPointsFound);
      std::copy(
        it->cellIds.begin(), it->cellIds.end(), this->ThreadGlobCellIds->begin() + nPointsFound);
      nPointsFound += it->pointIds.size();
      it->pointIds.resize(0);
      it->cellIds.resize(0);
    }
  }

}; // ProbingWorklet

//------------------------------------------------------------------------------
bool vtkHyperTreeGridPProbeFilter::DoProbing(
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
} // DoProbing

//------------------------------------------------------------------------------
bool vtkHyperTreeGridPProbeFilter::Initialize(
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
  return true;
} // Initialize

//------------------------------------------------------------------------------
bool vtkHyperTreeGridPProbeFilter::Reduce(
  vtkHyperTreeGrid* source, vtkDataSet* output, vtkIdList* localPointIds)
{
  int procId = 0;
  int numProcs = 1;
  if (this->Controller)
  {
    procId = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
  }

  vtkIdType numPointsFound = localPointIds->GetNumberOfIds();
  if (procId != 0)
  {
    this->Controller->Send(&numPointsFound, 1, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
    if (numPointsFound > 0)
    {
      this->Controller->Send(output, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
      this->Controller->Send(
        localPointIds->GetPointer(0), numPointsFound, 0, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
    }
    output->ReleaseData();
    localPointIds->Initialize();
  }
  else
  {
    auto dealWithRemote = [](vtkIdList* remotePointIds, vtkDataSet* remoteOutput,
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
      da->Fill(vtkMath::Nan());
    }
    dealWithRemote(localPointIds, remoteOutput, source, output);
    remoteOutput->Initialize();

    // deal with other processes
    if (numProcs > 1)
    {
      for (int iProc = 1; iProc < numProcs; iProc++)
      {
        this->Controller->Receive(
          &numRemotePoints, 1, iProc, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
        if (numRemotePoints > 0)
        {
          this->Controller->Receive(remoteOutput, iProc, HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
          remotePointIds->Initialize();
          remotePointIds->SetNumberOfIds(numRemotePoints);
          this->Controller->Receive(remotePointIds->GetPointer(0), numRemotePoints, iProc,
            HYPERTREEGRID_PROBE_COMMUNICATION_TAG);
          dealWithRemote(remotePointIds, remoteOutput, source, output);
          remoteOutput->Initialize();
        }
      }
    }
  }
  return true;
} // Initialize

//------------------------------------------------------------------------------
// Straight up copy from vtkProbeFilter
bool vtkHyperTreeGridPProbeFilter::PassAttributeData(vtkDataSet* input, vtkDataSet* output)
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
