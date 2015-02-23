#include "vtkStreamedCompositeDataPipeline.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGridAMRDataIterator2.h"

#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationKey.h"

#include "vtkSMPTools.h"
#include "vtkUniformGridAMR.h"

vtkStandardNewMacro(vtkStreamedCompositeDataPipeline);

vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, STREAM_DATA, Request);
vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, INIT_STREAM, Request);
vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, FINALIZE_STREAM, Request);
vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, START_STREAM, ObjectBase);
vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, STREAM_BLOCK_ID, ObjectBase);
vtkInformationKeyMacro(vtkStreamedCompositeDataPipeline, STREAM_BLOCK, ObjectBase);

vtkStreamedCompositeDataPipeline::vtkStreamedCompositeDataPipeline()
{
  this->CompositePort = -1;
}

vtkStreamedCompositeDataPipeline::~vtkStreamedCompositeDataPipeline()
{
  vtkSMPThreadLocal<vtkInformationVector**>::iterator it;
  int numPorts = this->GetNumberOfInputPorts();
  it = this->LocalInputInformations.begin();
  while (it != this->LocalInputInformations.end())
    {
    if (*it)
      {
      for (int i = 0; i < numPorts; ++i)
        {
        (*it)[i]->Delete();
        }
      delete [] (*it);
      }
    ++it;
    }
}

class vtkStreamingFunctor
{
  protected:
    vtkStreamedCompositeDataPipeline* self;
    int returnValue;
    vtkUniformGridAMRDataIterator2* baseIter;
    vtkInformation* baseRequest;
    vtkSMPThreadLocal<int> returnValues;
    vtkSMPThreadLocalObject<vtkInformation> requests;
    vtkSMPThreadLocalObject<vtkUniformGridAMRDataIterator2> iters;

  public:
    vtkStreamingFunctor(vtkStreamedCompositeDataPipeline* s,
                        vtkInformation* request)
      : self(s),
        baseIter(vtkUniformGridAMRDataIterator2::SafeDownCast(
          request->Get(vtkStreamedCompositeDataPipeline::STREAM_BLOCK_ID()))),
        baseRequest(request),
        iters(baseIter)
      {
      this->returnValue = 0;
      this->baseRequest->Remove(vtkStreamedCompositeDataPipeline::STREAM_BLOCK_ID());
      }

    virtual ~vtkStreamingFunctor()
      {
      }

    vtkIdType GetNumberOfBlocks()
      {
      return this->baseIter->GetNumberOfBlocks();
      }

    void Initialize()
      {
      this->returnValues.Local() = 1;
      vtkInformation*& request = this->requests.Local();
      request->Copy(baseRequest, 1);
      request->Set(vtkStreamedCompositeDataPipeline::INIT_STREAM());
      self->ProcessRequest(request,NULL,NULL);
      request->Remove(vtkStreamedCompositeDataPipeline::INIT_STREAM());

      request->Set(vtkStreamedCompositeDataPipeline::STREAM_DATA());
      vtkUniformGridAMRDataIterator2*& iter = this->iters.Local();
      iter->CopyAndInit(baseIter, 1);
      request->Set(vtkStreamedCompositeDataPipeline::STREAM_BLOCK_ID(), iter);
      }

    void operator()(vtkIdType begin, vtkIdType end)
      {
      vtkIdType blockId = 0;
      vtkUniformGridAMRDataIterator2*& iter = this->iters.Local();
      iter->GoToFirstItem();
      vtkInformation*& request = this->requests.Local();
      for (; blockId < begin; ++blockId, iter->GoToNextItem()) {}
      for (; blockId < end; ++blockId)
        {
        request->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), blockId);
        this->returnValues.Local() &= self->ProcessRequest(request,NULL,NULL);
        request->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
        iter->GoToNextItem();
        }
      }

    void Reduce()
      {
      this->returnValue = 1;
      vtkSMPThreadLocal<int>::iterator valueIter = this->returnValues.begin();
      while (valueIter != this->returnValues.end())
        {
        this->returnValue &= *valueIter;
        ++valueIter;
        }
      }

    int GetReturnValue()
      {
      return returnValue;
      }
};

void vtkStreamedCompositeDataPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkStreamedCompositeDataPipeline
::ProcessRequest(vtkInformation* request,
                 vtkInformationVector** inInfoVec,
                 vtkInformationVector* outInfoVec)
{
  // Check if we will have to launch the second pass
  if (request->Has(REQUEST_DATA()) && !request->Has(START_STREAM()))
    {
    request->Set(START_STREAM(), this);
    }
  // Post-second pass, mark filters up to date
  if (request->Has(FINALIZE_STREAM()))
    {
    vtkInformationVector** in = inInfoVec ? inInfoVec : this->GetInputInformation();
    vtkInformationVector* out = outInfoVec ? outInfoVec : this->GetOutputInformation();
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
      {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      }
    if (this->NeedToExecuteData(outputPort, in, out))
      {
      if(!this->ForwardUpstream(request))
        {
        return 0;
        }
      vtkDataObject* outData = this->GetOutputInformation()->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
      outData->DataHasBeenGenerated();
      this->DataTime.Modified();
      this->InformationTime.Modified();
      this->DataObjectTime.Modified();
      }
    return 1;
    }
  // Pre-second pass, init thread local data
  if (request->Has(INIT_STREAM()))
    {
    vtkInformationVector** in = inInfoVec ? inInfoVec : this->GetInputInformation();
    vtkInformationVector* out = outInfoVec ? outInfoVec : this->GetOutputInformation();
    int outputPort = -1;
    if(request->Has(FROM_OUTPUT_PORT()))
      {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      }
    if (this->NeedToExecuteData(outputPort, in, out))
    {
      if(!this->ForwardUpstream(request))
      {
        return 0;
      }
      if (!request->Has(START_STREAM())) request->Set(START_STREAM(), this);
      this->InitLocalData();
    }
    return 1;
    }
  // Second pass, let the blocs flow down
  if (this->Algorithm && request->Has(STREAM_DATA()))
  {
    if(request->Get(START_STREAM()) != this && !this->ForwardUpstream(request))
    {
      return 0;
    }
    vtkInformationVector** in = inInfoVec ? inInfoVec : this->GetInputInformation();
    vtkInformationVector* out = outInfoVec ? outInfoVec : this->GetOutputInformation();
    return this->ProcessBlock(request, in, out);
  }

  // First pass and other requests are handled by superclass
  if (!this->Superclass::ProcessRequest(request, inInfoVec, outInfoVec))
  {
    return 0;
  }

  // Launch second pass
  if (request->Get(START_STREAM()) == this && request->Has(STREAM_BLOCK_ID()))
  {
    request->Remove(START_STREAM());
    vtkStreamingFunctor functor(this, request);
    vtkSMPTools::For(0, functor.GetNumberOfBlocks(), functor);
    request->Remove(STREAM_BLOCK_ID());
    vtkInformation* r = vtkInformation::New();
    r->Copy(request, 1);
    r->Set(FINALIZE_STREAM());
    this->ProcessRequest(r, inInfoVec, outInfoVec);
    r->Delete();
    return functor.GetReturnValue();
  }
  return 1;
}

int vtkStreamedCompositeDataPipeline
::ExecuteData(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  bool composite = this->CompositePort >= 0 || this->ShouldIterateOverInput(inInfoVec, this->CompositePort);
  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);
  vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (composite)
  {
    vtkInformation* inInfo = inInfoVec[this->CompositePort]->GetInformationObject(0);
    vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!request->Has(STREAM_BLOCK_ID()))
    {
      request->Set(STREAM_BLOCK_ID(), input->NewIterator());
    }
    output->PrepareForNewData();
    output->CopyStructure(input);
    return 1;
  }

  this->CompositePort = -1;
  return this->Superclass::ExecuteData(request, inInfoVec, outInfoVec);
}

int vtkStreamedCompositeDataPipeline
::ProcessBlock(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  if (this->CompositePort >= 0)
  {
    vtkCompositeDataIterator* iter =
      vtkCompositeDataIterator::SafeDownCast(request->Get(STREAM_BLOCK_ID()));
    vtkInformation* r = this->LocalRequests.Local();
    vtkInformationVector** inInfoV = this->LocalInputInformations.Local();
    vtkInformationVector* outInfoV = this->LocalOutputInformations.Local();
    vtkInformation* inInfo = inInfoV[this->CompositePort]->GetInformationObject(0);
    vtkInformation* outInfo = outInfoV->GetInformationObject(0);
    vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(
        inInfoVec[this->CompositePort]->GetInformationObject(0)
        ->Get(vtkDataObject::DATA_OBJECT()));
    vtkDataObject* dobj = input->GetDataSet(iter);
    if (dobj)
    {
      this->InLocalLoop = 1;
      vtkDataObject* outObj = this->ExecuteSimpleAlgorithmForBlock(inInfoV,
          outInfoV,
          inInfo,
          outInfo,
          r,
          dobj);
      this->InLocalLoop = 0;
      if (outObj)
      {
        vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(
            outInfoVec->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
        output->SetDataSet(iter, outObj);
        outObj->FastDelete();
        return 1;
      }
    }
    return 0;
  }

  return 1;
}

void vtkStreamedCompositeDataPipeline::Modified()
{
  this->CompositePort = -1;
  this->Superclass::Modified();
}

void vtkStreamedCompositeDataPipeline::InitLocalData()
{
  vtkInformationVector*& outInfoVec = this->LocalOutputInformations.Local();
  outInfoVec->Copy(this->GetOutputInformation(), 1);
  vtkInformation*& request = this->LocalRequests.Local();
  request->Set(vtkExecutive::FORWARD_DIRECTION(), vtkExecutive::RequestUpstream);
  request->Set(vtkExecutive::ALGORITHM_AFTER_FORWARD(), 1);
  request->Set(FROM_OUTPUT_PORT(),
      PRODUCER()->GetPort(outInfoVec->GetInformationObject(0)));
  int numPorts = this->GetNumberOfInputPorts();
  vtkInformationVector** inInfoVec = this->LocalInputInformations.Local();
  vtkInformationVector** globalIn = this->GetInputInformation();
  if (inInfoVec)
  {
    for(int i = 0; i < numPorts; ++i)
    {
      inInfoVec[i]->Delete();
      inInfoVec[i] = vtkInformationVector::New();
      inInfoVec[i]->Copy(globalIn[i], 1);
    }
  }
  else
  {
    inInfoVec = new vtkInformationVector*[numPorts];
    for(int i = 0; i < numPorts; ++i)
    {
      inInfoVec[i] = vtkInformationVector::New();
      inInfoVec[i]->Copy(globalIn[i], 1);
    }
  }
  this->LocalInputInformations.Local() = inInfoVec;
}

int vtkStreamedCompositeDataPipeline::NeedToExecuteData(
  int outputPort,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  return this->vtkDemandDrivenPipeline::
    NeedToExecuteData(outputPort, inInfoVec, outInfoVec);
}
