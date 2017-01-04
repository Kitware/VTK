/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedCompositeDataPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkThreadedCompositeDataPipeline.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"

#include "vtkMultiThreader.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkTimerLog.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkDebugLeaks.h"
#include "vtkImageData.h"

#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSMPProgressObserver.h"

#include <vector>
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkThreadedCompositeDataPipeline);

//----------------------------------------------------------------------------
namespace
{
  static vtkInformationVector** Clone(vtkInformationVector** src, int n)
  {
    vtkInformationVector** dst = new vtkInformationVector*[n];
    for(int i=0; i<n; ++i)
    {
      dst[i] = vtkInformationVector::New();
      dst[i]->Copy(src[i],1);
    }
    return dst;
  }
  static void DeleteAll(vtkInformationVector** dst, int n)
  {
    for(int i=0; i<n; ++i)
    {
      dst[i]->Delete();
    }
    delete []dst;
  }
};

//----------------------------------------------------------------------------
class ProcessBlockData: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(ProcessBlockData, vtkObjectBase);
  vtkInformationVector** In;
  vtkInformationVector* Out;
  int InSize;

  static ProcessBlockData* New()
  {
    // Can't use object factory macros, this is not a vtkObject.
    ProcessBlockData *ret = new ProcessBlockData;
    ret->InitializeObjectBase();
    return ret;
  }

  void Construct(vtkInformationVector** inInfoVec,
                 int inInfoVecSize,
                 vtkInformationVector* outInfoVec)
  {
    this->InSize  = inInfoVecSize;
    this->In = Clone(inInfoVec, inInfoVecSize);
    this->Out = vtkInformationVector::New();
    this->Out->Copy(outInfoVec,1);
  }

  ~ProcessBlockData() VTK_OVERRIDE
  {
    DeleteAll(this->In, this->InSize);
    this->Out->Delete();
  }

protected:
  ProcessBlockData():
    In(NULL),
    Out(NULL)
  {

  }
};
//----------------------------------------------------------------------------
class ProcessBlock
{
public:
  ProcessBlock(vtkThreadedCompositeDataPipeline* exec,
               vtkInformationVector** inInfoVec,
               vtkInformationVector* outInfoVec,
               int compositePort,
               int connection,
               vtkInformation* request,
               const std::vector<vtkDataObject*>& inObjs,
               std::vector<vtkDataObject*>& outObjs)
    : Exec(exec),
      InInfoVec(inInfoVec),
      OutInfoVec(outInfoVec),
      CompositePort(compositePort),
      Connection(connection),
      Request(request),
      InObjs(inObjs)
  {
    int numInputPorts = this->Exec->GetNumberOfInputPorts();
    this->OutObjs = &outObjs[0];
    this->InfoPrototype = vtkSmartPointer<ProcessBlockData>::New();
    this->InfoPrototype->Construct(this->InInfoVec, numInputPorts, this->OutInfoVec);
  }

  ~ProcessBlock()
  {
    vtkSMPThreadLocal<vtkInformationVector**>::iterator itr1 =
      this->InInfoVecs.begin();
    vtkSMPThreadLocal<vtkInformationVector**>::iterator end1 =
      this->InInfoVecs.end();
    while (itr1 != end1)
    {
      DeleteAll(*itr1, this->InfoPrototype->InSize);
      ++itr1;
    }

    vtkSMPThreadLocal<vtkInformationVector*>::iterator itr2 =
      this->OutInfoVecs.begin();
    vtkSMPThreadLocal<vtkInformationVector*>::iterator end2 =
      this->OutInfoVecs.end();
    while (itr2 != end2)
    {
      (*itr2)->Delete();
      ++itr2;
    }
  }

  void Initialize()
  {
    vtkInformationVector**& inInfoVec = this->InInfoVecs.Local();
    vtkInformationVector*& outInfoVec = this->OutInfoVecs.Local();

    inInfoVec = Clone(this->InfoPrototype->In, this->InfoPrototype->InSize);
    outInfoVec = vtkInformationVector::New();
    outInfoVec->Copy(this->InfoPrototype->Out, 1);

    vtkInformation*& request = this->Requests.Local();
    request->Copy(this->Request, 1);

  }

  void operator() (vtkIdType begin, vtkIdType end)
  {
    vtkInformationVector** inInfoVec = this->InInfoVecs.Local();
    vtkInformationVector* outInfoVec = this->OutInfoVecs.Local();
    vtkInformation* request = this->Requests.Local();

    vtkInformation* inInfo = inInfoVec[this->CompositePort]->GetInformationObject(this->Connection);
    vtkInformation* outInfo = outInfoVec->GetInformationObject(0);

    for(vtkIdType i= begin; i<end; ++i)
    {
      vtkDataObject* outObj =
        this->Exec->ExecuteSimpleAlgorithmForBlock(&inInfoVec[0],
                                                   outInfoVec,
                                                   inInfo,
                                                   outInfo,
                                                   request,
                                                   this->InObjs[i]);
      this->OutObjs[i] = outObj;
    }
  }

  void Reduce()
  {
  }

protected:
  vtkThreadedCompositeDataPipeline* Exec;
  vtkInformationVector** InInfoVec;
  vtkInformationVector* OutInfoVec;
  vtkSmartPointer<ProcessBlockData> InfoPrototype;
  int CompositePort;
  int Connection;
  vtkInformation* Request;
  const std::vector<vtkDataObject*>& InObjs;
  vtkDataObject** OutObjs;

  vtkSMPThreadLocal<vtkInformationVector**> InInfoVecs;
  vtkSMPThreadLocal<vtkInformationVector*> OutInfoVecs;
  vtkSMPThreadLocalObject<vtkInformation> Requests;
};


//----------------------------------------------------------------------------
vtkThreadedCompositeDataPipeline::vtkThreadedCompositeDataPipeline()
{
}

//----------------------------------------------------------------------------
vtkThreadedCompositeDataPipeline::~vtkThreadedCompositeDataPipeline()
{
}

//-------------------------------------------------------------------------
void vtkThreadedCompositeDataPipeline::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void vtkThreadedCompositeDataPipeline::ExecuteEach(vtkCompositeDataIterator* iter,
                                                   vtkInformationVector** inInfoVec,
                                                   vtkInformationVector* outInfoVec,
                                                   int compositePort,
                                                   int connection,
                                                   vtkInformation* request,
                                                   vtkCompositeDataSet* compositeOutput)
{
  // from input data objects  itr -> (inObjs, indices)
  // inObjs are the non-null objects that we will loop over.
  // indices map the input objects to inObjs
  std::vector<vtkDataObject*> inObjs;
  std::vector<int> indices;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    if (dobj)
    {
      inObjs.push_back(dobj);
      indices.push_back(static_cast<int>(inObjs.size())-1);
    }
    else
    {
      indices.push_back(-1);
    }
  }

  // instantiate outObjs, the output objects that will be created from inObjs
  std::vector<vtkDataObject*> outObjs;
  outObjs.resize(indices.size(),NULL);

  // create the parallel task processBlock
  ProcessBlock processBlock(this,
                            inInfoVec,
                            outInfoVec,
                            compositePort,
                            connection,
                            request,
                            inObjs,outObjs);

  vtkSmartPointer<vtkProgressObserver> origPo(this->Algorithm->GetProgressObserver());
  vtkNew<vtkSMPProgressObserver> po;
  this->Algorithm->SetProgressObserver(po.GetPointer());
  vtkSMPTools::For(0, static_cast<vtkIdType>(inObjs.size()), processBlock);
  this->Algorithm->SetProgressObserver(origPo);

  int i =0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), i++)
  {
    int j = indices[i];
    if(j>=0)
    {
      vtkDataObject* outObj = outObjs[j];
      compositeOutput->SetDataSet(iter, outObj);
      if(outObj)
      {
        outObj->FastDelete();
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkThreadedCompositeDataPipeline::CallAlgorithm(vtkInformation* request, int direction,
                                                    vtkInformationVector** inInfo,
                                                    vtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  int result = this->Algorithm->ProcessRequest(request, inInfo, outInfo);

  // If the algorithm failed report it now.
  if(!result)
  {
    vtkErrorMacro("Algorithm " << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm
                  << ") returned failure for request: "
                  << *request);
  }

  return result;
}
