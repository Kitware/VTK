/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnsembleSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnsembleSource.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationDataObjectMetaDataKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <vector>

vtkStandardNewMacro(vtkEnsembleSource);
vtkCxxSetObjectMacro(vtkEnsembleSource, MetaData, vtkTable);

vtkInformationKeyMacro(vtkEnsembleSource, META_DATA, DataObjectMetaData);
vtkInformationKeyMacro(vtkEnsembleSource, DATA_MEMBER, Integer);

// Subclass vtkInformationIntegerRequestKey to set the DataKey.
class vtkInformationEnsembleMemberRequestKey : public vtkInformationIntegerRequestKey
{
public:
  vtkInformationEnsembleMemberRequestKey(const char* name, const char* location) :
    vtkInformationIntegerRequestKey(name, location)
  {
    this->DataKey = vtkEnsembleSource::DATA_MEMBER();
  }
};
vtkInformationKeySubclassMacro(vtkEnsembleSource, UPDATE_MEMBER, EnsembleMemberRequest, IntegerRequest);

struct vtkEnsembleSourceInternal
{
  std::vector<vtkSmartPointer<vtkAlgorithm> > Algorithms;
};

vtkEnsembleSource::vtkEnsembleSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internal = new vtkEnsembleSourceInternal;

  this->CurrentMember = 0;

  this->MetaData = 0;
}

vtkEnsembleSource::~vtkEnsembleSource()
{
  delete this->Internal;

  if (this->MetaData)
  {
    this->MetaData->Delete();
    this->MetaData = 0;
  }
}

int vtkEnsembleSource::ProcessRequest(vtkInformation *request,
                                      vtkInformationVector **inputVector,
                                      vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkAlgorithm* currentReader = this->GetCurrentReader(outInfo);
  if (currentReader)
  {
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
      // Make sure to initialize our output to the right type.
      // Note all ensemble members are expected to produce the same
      // data type or we are toast.
      currentReader->UpdateDataObject();
      vtkDataObject* rOutput = currentReader->GetOutputDataObject(0);
      vtkDataObject* output = rOutput->NewInstance();
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(),
        output);
      output->Delete();
      return 1;
    }
    if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
      if (this->MetaData)
      {
        outputVector->GetInformationObject(0)->Set(META_DATA(),
          this->MetaData);
      }
      // Call RequestInformation on all readers as they may initialize
      // data structures there. Note that this has to be done here
      // because current reader can be changed with a pipeline request
      // which does not cause REQUEST_INFORMATION to happen again.
      std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator iter =
        this->Internal->Algorithms.begin();
      std::vector<vtkSmartPointer<vtkAlgorithm> >::iterator end =
        this->Internal->Algorithms.end();
      for(; iter != end; ++iter)
      {
        int retVal = (*iter)->ProcessRequest(request, inputVector, outputVector);
        if (!retVal)
        {
          return retVal;
        }
      }
      return 1;
    }

    return currentReader->ProcessRequest(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void vtkEnsembleSource::AddMember(vtkAlgorithm* alg)
{
  this->Internal->Algorithms.push_back(alg);
}

void vtkEnsembleSource::RemoveAllMembers()
{
  this->Internal->Algorithms.clear();
}

unsigned int vtkEnsembleSource::GetNumberOfMembers()
{
  return static_cast<unsigned int>(this->Internal->Algorithms.size());
}

vtkAlgorithm* vtkEnsembleSource::GetCurrentReader(vtkInformation* outInfo)
{
  unsigned int currentMember = 0;
  if (outInfo->Has(UPDATE_MEMBER()))
  {
    currentMember = static_cast<unsigned int>(outInfo->Get(UPDATE_MEMBER()));
  }
  else
  {
    currentMember = this->CurrentMember;
  }
  if (currentMember >= this->GetNumberOfMembers())
  {
    return 0;
  }
  return this->Internal->Algorithms[currentMember];
}

int vtkEnsembleSource::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

void vtkEnsembleSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Current member: " << this->CurrentMember << endl;
  os << indent << "MetaData: " << endl;
  if (this->MetaData)
  {
    this->MetaData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "(NULL)" << endl;
  }
}
