#include "vtkStreamedCompositeSources.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkObjectFactory.h"

#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkStreamedCompositeSources);

vtkStreamedCompositeSources::vtkStreamedCompositeSources()
{
}

vtkStreamedCompositeSources::~vtkStreamedCompositeSources()
{
}

void vtkStreamedCompositeSources::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkStreamedCompositeSources
::ExecuteData(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  if (!this->Superclass::ExecuteData(request, inInfoVec, outInfoVec))
    {
    return 0;
    }

  vtkInformation* outInfo = outInfoVec->GetInformationObject(0);
  vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->GetNumberOfInputPorts() == 0 && output /* && this->Algorithm->IsStreamableSource()*/)
  {
    // At this point, output should contain only metadata. All blocks are empty.
    vtkCompositeDataIterator* iter = output->NewIterator();
    iter->SkipEmptyNodesOff();
    iter->GoToFirstItem();
    request->Set(STREAM_BLOCK_ID(), iter);
    this->PipelineMTime = output->GetUpdateTime() + 1;
  }

  return 1;
}

int vtkStreamedCompositeSources
::ProcessBlock(vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  vtkCompositeDataSet* output = vtkCompositeDataSet::SafeDownCast(
      outInfoVec->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));

  if (this->GetNumberOfInputPorts() == 0 && output /* && this->Algorithm->IsStreamableSource()*/)
  {
    // Proof of concept here, only using it on AMR datasets
    vtkCompositeDataIterator* iter =
      vtkCompositeDataIterator::SafeDownCast(request->Get(STREAM_BLOCK_ID()));
    vtkInformation* r = this->LocalRequests.Local();
    r->Set(REQUEST_DATA());
    r->Set(UPDATE_PIECE_NUMBER(), request->Get(UPDATE_PIECE_NUMBER()));
    vtkInformationVector** inInfoV = this->LocalInputInformations.Local();
    vtkInformationVector* outInfoV = this->LocalOutputInformations.Local();
    outInfoV->GetInformationObject(0)->Remove(vtkDataObject::DATA_OBJECT());
    this->Superclass::ExecuteData(r, inInfoV, outInfoV);
    r->Remove(REQUEST_DATA());
    vtkDataObject* dobj = vtkDataObject::SafeDownCast(
        outInfoV->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
    if (dobj)
    {
      vtkCompositeDataSet::SafeDownCast(
          outInfoVec->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()))
        ->SetDataSet(iter, dobj);
      return 1;
    }
    return 0;
  }

  return this->Superclass::ProcessBlock(request, inInfoVec, outInfoVec);
}
