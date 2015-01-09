/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3Writer.h"

#include "vtkDataObject.h"
#include "vtkDirectedGraph.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkXdmf3DataSet.h"

#include "XdmfDomain.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfWriter.hpp"
#include <stack>

//=============================================================================
class vtkXdmf3Writer::Internals {
public:
  Internals()
  {
  }
  ~Internals() {};
  void Init(const char *filename)
  {
    this->Domain = XdmfDomain::New();
    this->Writer = XdmfWriter::New(filename);
    this->Writer->setLightDataLimit(0);
    this->Writer->getHeavyDataWriter()->setReleaseData(true);
    this->NumberOfTimeSteps = 1;
    this->CurrentTimeIndex = 0;
    this->DestinationGroups.push(this->Domain);
    this->Destination = this->DestinationGroups.top();

  }
  void SwitchToTemporal()
  {
    shared_ptr<XdmfGridCollection> dest = XdmfGridCollection::New();
    dest->setType(XdmfGridCollectionType::Temporal());
    this->DestinationGroups.push(dest);
    this->Destination = this->DestinationGroups.top();
    this->Domain->insert(dest);
  }
  void WriteDataObject(vtkDataObject *dataSet, bool hasTime, double time,
    const char* name = 0)
  {
    if (!dataSet)
      {
      return;
      }
    switch (dataSet->GetDataObjectType())
      {
      case VTK_MULTIBLOCK_DATA_SET:
        {
        shared_ptr<XdmfGridCollection> group = XdmfGridCollection::New();
        this->Destination->insert(group);
        this->DestinationGroups.push(group);
        this->Destination = this->DestinationGroups.top();
        vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::SafeDownCast(dataSet);
        for (unsigned int i = 0; i< mbds->GetNumberOfBlocks(); i++)
          {
          vtkDataObject *next = mbds->GetBlock(i);
          const char* blockName = mbds->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
          this->WriteDataObject(next, hasTime, time, blockName);
          }
        this->DestinationGroups.pop();
        this->Destination = this->DestinationGroups.top();
        break;
        }
      case VTK_STRUCTURED_POINTS:
      case VTK_IMAGE_DATA:
      case VTK_UNIFORM_GRID:
        {
        vtkXdmf3DataSet::VTKToXdmf(
          vtkImageData::SafeDownCast(dataSet),
          this->Destination.get(),
          hasTime, time, name);
        break;
        }
      case VTK_RECTILINEAR_GRID:
        {
        vtkXdmf3DataSet::VTKToXdmf(
          vtkRectilinearGrid::SafeDownCast(dataSet),
          this->Destination.get(),
          hasTime, time, name);
        break;
        }
      case VTK_STRUCTURED_GRID:
        {
        vtkXdmf3DataSet::VTKToXdmf(
          vtkStructuredGrid::SafeDownCast(dataSet),
          this->Destination.get(),
          hasTime, time, name);
        break;
        }
      case VTK_POLY_DATA:
      case VTK_UNSTRUCTURED_GRID:
        {
        vtkXdmf3DataSet::VTKToXdmf(
          vtkPointSet::SafeDownCast(dataSet),
          this->Destination.get(),
          hasTime, time, name);
        break;
        }
      //case VTK_GRAPH:
      case VTK_DIRECTED_GRAPH:
      //case VTK_UNDIRECTED_GRAPH:
        {
        vtkXdmf3DataSet::VTKToXdmf(
          vtkDirectedGraph::SafeDownCast(dataSet),
          this->Destination.get(),
          hasTime, time, name);
        break;
        }
      default:
        {
        }
      }

    this->Domain->accept(this->Writer);
  }
  boost::shared_ptr<XdmfDomain> Domain;
  boost::shared_ptr<XdmfDomain> Destination;
  boost::shared_ptr<XdmfWriter> Writer;

  std::stack<boost::shared_ptr<XdmfDomain> > DestinationGroups;

  int NumberOfTimeSteps;
  int CurrentTimeIndex;
};

//==============================================================================

vtkStandardNewMacro(vtkXdmf3Writer);

//----------------------------------------------------------------------------
vtkXdmf3Writer::vtkXdmf3Writer()
{
  this->FileName = NULL;
  this->LightDataLimit = 100;
  this->WriteAllTimeSteps = false;
  this->Internal = new Internals();
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
vtkXdmf3Writer::~vtkXdmf3Writer()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkXdmf3Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "LightDataLimit: " <<
    this->LightDataLimit << endl;
  os << indent << "WriteAllTimeSteps: " <<
    (this->WriteAllTimeSteps?"ON":"OFF") << endl;
}

//------------------------------------------------------------------------------
void vtkXdmf3Writer::SetInputData(vtkDataObject *input)
{
  this->SetInputDataInternal(0,input);
}

//------------------------------------------------------------------------------
int vtkXdmf3Writer::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }

  // always write even if the data hasn't changed
  this->Modified();

  if (!this->Internal)
  {
    this->Internal = new Internals();
  }
  this->Internal->Init(this->FileName);

  this->Update();

  //this->Internal->Domain->accept(this->Internal->Writer);

  delete this->Internal;
  this->Internal = NULL;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Writer::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Does the input have timesteps?
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->Internal->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->Internal->NumberOfTimeSteps = 1;
    }
  //cerr << "WRITER NUM TS = " << this->Internal->NumberOfTimeSteps << endl;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Writer::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  double *inTimes = inputVector[0]->GetInformationObject(0)->Get(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
    {
    //TODO:? Add a user ivar to specify a particular time,
    //which is different from current time. Can do it by updating
    //to a particular time then writing without writealltimesteps,
    //but that is annoying.
    double timeReq = inTimes[this->Internal->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        timeReq);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Writer::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Internal->Domain)
    {
    //call Write() instead of RD() directly. Write() does setup first before it calls RD().
    return 1;
    }

  this->Internal->Writer->setLightDataLimit(this->LightDataLimit);

  if (this->Internal->CurrentTimeIndex == 0 &&
      this->WriteAllTimeSteps &&
      this->Internal->NumberOfTimeSteps > 1)
    {
    // Tell the pipeline to start looping.
    this->Internal->SwitchToTemporal();
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation *inDataInfo = input->GetInformation();
  double dataT = 0;
  bool hasTime = false;
  if (inDataInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
    dataT = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    hasTime = true;
    }
  this->Internal->WriteDataObject(input, hasTime, dataT);

  this->Internal->CurrentTimeIndex++;
  if (this->Internal->CurrentTimeIndex >= this->Internal->NumberOfTimeSteps &&
      this->WriteAllTimeSteps)
    {
    // Tell the pipeline to stop looping.
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->Internal->CurrentTimeIndex = 0;
    }

  return 1;
}
