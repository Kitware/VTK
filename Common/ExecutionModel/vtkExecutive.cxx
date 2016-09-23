/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExecutive.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vector>
#include <sstream>

#include "vtkCompositeDataPipeline.h"

vtkInformationKeyMacro(vtkExecutive, ALGORITHM_AFTER_FORWARD, Integer);
vtkInformationKeyMacro(vtkExecutive, ALGORITHM_BEFORE_FORWARD, Integer);
vtkInformationKeyMacro(vtkExecutive, ALGORITHM_DIRECTION, Integer);
vtkInformationKeyMacro(vtkExecutive, CONSUMERS, ExecutivePortVector);
vtkInformationKeyMacro(vtkExecutive, FORWARD_DIRECTION, Integer);
vtkInformationKeyMacro(vtkExecutive, FROM_OUTPUT_PORT, Integer);
vtkInformationKeyMacro(vtkExecutive, KEYS_TO_COPY, KeyVector);
vtkInformationKeyMacro(vtkExecutive, PRODUCER, ExecutivePort);

//----------------------------------------------------------------------------
class vtkExecutiveInternals
{
public:
  std::vector<vtkInformationVector*> InputInformation;
  vtkExecutiveInternals();
  ~vtkExecutiveInternals();
  vtkInformationVector** GetInputInformation(int newNumberOfPorts);
};

//----------------------------------------------------------------------------
vtkExecutiveInternals::vtkExecutiveInternals()
{
}

//----------------------------------------------------------------------------
vtkExecutiveInternals::~vtkExecutiveInternals()
{
  // Delete all the input information vectors.
  for(std::vector<vtkInformationVector*>::iterator
        i = this->InputInformation.begin();
      i != this->InputInformation.end(); ++i)
  {
    if(vtkInformationVector* v = *i)
    {
      v->Delete();
    }
  }
}

//----------------------------------------------------------------------------
vtkInformationVector**
vtkExecutiveInternals::GetInputInformation(int newNumberOfPorts)
{
  // Adjust the number of vectors.
  int oldNumberOfPorts = static_cast<int>(this->InputInformation.size());
  if(newNumberOfPorts > oldNumberOfPorts)
  {
    // Create new vectors.
    this->InputInformation.resize(newNumberOfPorts, 0);
    for(int i=oldNumberOfPorts; i < newNumberOfPorts; ++i)
    {
      this->InputInformation[i] = vtkInformationVector::New();
    }
  }
  else if(newNumberOfPorts < oldNumberOfPorts)
  {
    // Delete old vectors.
    for(int i=newNumberOfPorts; i < oldNumberOfPorts; ++i)
    {
      if(vtkInformationVector* v = this->InputInformation[i])
      {
        // Set the pointer to NULL first to avoid reporting of the
        // entry if deleting the vector causes a garbage collection
        // reference walk.
        this->InputInformation[i] = 0;
        v->Delete();
      }
    }
    this->InputInformation.resize(newNumberOfPorts);
  }

  // Return the array of information vector pointers.
  if(newNumberOfPorts > 0)
  {
    return &this->InputInformation[0];
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkExecutive::vtkExecutive()
{
  this->ExecutiveInternal = new vtkExecutiveInternals;
  this->OutputInformation = vtkInformationVector::New();
  this->Algorithm = 0;
  this->InAlgorithm = 0;
  this->SharedInputInformation = 0;
  this->SharedOutputInformation = 0;
}

//----------------------------------------------------------------------------
vtkExecutive::~vtkExecutive()
{
  this->SetAlgorithm(0);
  if(this->OutputInformation)
  {
    this->OutputInformation->Delete();
  }
  delete this->ExecutiveInternal;
}

//----------------------------------------------------------------------------
void vtkExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->Algorithm)
  {
    os << indent << "Algorithm: " << this->Algorithm << "\n";
  }
  else
  {
    os << indent << "Algorithm: (none)\n";
  }
}

//----------------------------------------------------------------------------
void vtkExecutive::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkExecutive::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkExecutive::SetAlgorithm(vtkAlgorithm* newAlgorithm)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Algorithm to " << newAlgorithm);
  vtkAlgorithm* oldAlgorithm = this->Algorithm;
  if(oldAlgorithm != newAlgorithm)
  {
    if(newAlgorithm)
    {
      newAlgorithm->Register(this);
    }
    this->Algorithm = newAlgorithm;
    if(oldAlgorithm)
    {
      oldAlgorithm->UnRegister(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkExecutive::GetAlgorithm()
{
  return this->Algorithm;
}

//----------------------------------------------------------------------------
vtkInformationVector** vtkExecutive::GetInputInformation()
{
  // Use the shared input information vector if any is set.
  if(this->SharedInputInformation)
  {
    return this->SharedInputInformation;
  }

  // Use this executive's input information vector.
  if(this->Algorithm)
  {
    int numPorts = this->Algorithm->GetNumberOfInputPorts();
    return this->ExecutiveInternal->GetInputInformation(numPorts);
  }
  else
  {
    return this->ExecutiveInternal->GetInputInformation(0);
  }
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetInputInformation(int port, int connection)
{
  if(!this->InputPortIndexInRange(port, "get connected input information from"))
  {
    return 0;
  }
  vtkInformationVector* inVector = this->GetInputInformation()[port];
  return inVector->GetInformationObject(connection);
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkExecutive::GetInputInformation(int port)
{
  if(!this->InputPortIndexInRange(port, "get input information vector from"))
  {
    return 0;
  }
  return this->GetInputInformation()[port];
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkExecutive::GetOutputInformation()
{
  // Use the shared output information vector if any is set.
  if(this->SharedOutputInformation)
  {
    return this->SharedOutputInformation;
  }

  // Use this executive's output information vector.
  if (!this->Algorithm)
  {
    return 0;
  }
  // Set the length of the vector to match the number of ports.
  int oldNumberOfPorts =
    this->OutputInformation->GetNumberOfInformationObjects();
  this->OutputInformation
    ->SetNumberOfInformationObjects(this->GetNumberOfOutputPorts());

  // For any new information obects, set the executive pointer and
  // port number on the information object to tell it what produces
  // it.
  int nop = this->Algorithm->GetNumberOfOutputPorts();
  for(int i = oldNumberOfPorts; i < nop; ++i)
  {
    vtkInformation* info = this->OutputInformation->GetInformationObject(i);
    vtkExecutive::PRODUCER()->Set(info, this, i);
  }

  return this->OutputInformation;
}

//----------------------------------------------------------------------------
vtkInformation* vtkExecutive::GetOutputInformation(int port)
{
  return this->GetOutputInformation()->GetInformationObject(port);
}

//----------------------------------------------------------------------------
vtkExecutive* vtkExecutive::GetInputExecutive(int port, int index)
{
  if(index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    vtkErrorMacro("Attempt to get executive for connection index " << index
                  << " on input port " << port << " of algorithm "
                  << this->Algorithm->GetClassName() << "(" << this->Algorithm
                  << "), which has "
                  << this->GetNumberOfInputConnections(port)
                  << " connections.");
    return 0;
  }
  if(vtkAlgorithmOutput* input = this->Algorithm->GetInputConnection(port, index))
  {
    return input->GetProducer()->GetExecutive();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkExecutive::ReportReferences(vtkGarbageCollector* collector)
{
  // Report reference to our algorithm.
  vtkGarbageCollectorReport(collector, this->Algorithm, "Algorithm");

  for(int i=0; i < int(this->ExecutiveInternal->InputInformation.size()); ++i)
  {
    vtkGarbageCollectorReport(collector,
                              this->ExecutiveInternal->InputInformation[i],
                              "Input Information Vector");
  }

  vtkGarbageCollectorReport(collector, this->OutputInformation,
                            "Output Information Vector");
  this->Superclass::ReportReferences(collector);
}

//----------------------------------------------------------------------------
int vtkExecutive::Update()
{
  if (this->Algorithm->GetNumberOfOutputPorts())
  {
    return this->Update(0);
  }
  return this->Update(-1);
}

//----------------------------------------------------------------------------
int vtkExecutive::Update(int)
{
  vtkErrorMacro("This class does not implement Update.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::GetNumberOfInputPorts()
{
  if(this->Algorithm)
  {
    return this->Algorithm->GetNumberOfInputPorts();
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::GetNumberOfOutputPorts()
{
  if(this->Algorithm)
  {
    return this->Algorithm->GetNumberOfOutputPorts();
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::GetNumberOfInputConnections(int port)
{
  vtkInformationVector* inputs = this->GetInputInformation(port);
  if (inputs)
  {
    return inputs->GetNumberOfInformationObjects();
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::InputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if(!this->Algorithm)
  {
    vtkErrorMacro("Attempt to " << (action?action:"access") <<
                  " input port index " << port << " with no algorithm set.");
    return 0;
  }

  // Make sure the index of the input port is in range.
  if(port < 0 || port >= this->Algorithm->GetNumberOfInputPorts())
  {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " input port index " << port << " for algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << "), which has "
                  << this->Algorithm->GetNumberOfInputPorts()
                  << " input ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExecutive::OutputPortIndexInRange(int port, const char* action)
{
  // Make sure the algorithm is set.
  if(!this->Algorithm)
  {
    vtkErrorMacro("Attempt to " << (action?action:"access") <<
                  " output port index " << port << " with no algorithm set.");
    return 0;
  }

  // Make sure the index of the output port is in range.
  if(port < 0 || port >= this->Algorithm->GetNumberOfOutputPorts())
  {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " output port index " << port << " for algorithm "
                  << this->Algorithm->GetClassName()
                  << "(" << this->Algorithm << "), which has "
                  << this->Algorithm->GetNumberOfOutputPorts()
                  << " output ports.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// vtkAlgorithmOutput* vtkExecutive::GetProducerPort(vtkDataObject* d)
// {
//   if (!this->Algorithm)
//     {
//     return 0;
//     }

//   int numPorts = this->GetNumberOfOutputPorts();
//   for (int i=0; i<numPorts; i++)
//     {
//     vtkInformation* info = this->GetOutputInformation(i);
//     if (info->Has(vtkDataObject::DATA_OBJECT()) &&
//         info->Get(vtkDataObject::DATA_OBJECT()) == d)
//       {
//       return this->Algorithm->GetOutputPort(port);
//       }
//     }
//   return 0;

// }

//----------------------------------------------------------------------------
void vtkExecutive::SetSharedInputInformation(vtkInformationVector** inInfoVec)
{
  this->SharedInputInformation = inInfoVec;
}

//----------------------------------------------------------------------------
void vtkExecutive::SetSharedOutputInformation(vtkInformationVector* outInfoVec)
{
  this->SharedOutputInformation = outInfoVec;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetOutputData(int port)
{
  if(!this->OutputPortIndexInRange(port, "get data for"))
  {
    return 0;
  }

  vtkInformation* info = this->GetOutputInformation(port);
  if (!info)
  {
    return 0;
  }

  // for backward compatibility we bring Outputs up to date if they do not
  // already exist
  if (!this->InAlgorithm && !info->Has(vtkDataObject::DATA_OBJECT()))
  {
    // Bring the data object up to date only if it isn't already there
    this->UpdateDataObject();
  }

  // Return the data object.
  return info->Get(vtkDataObject::DATA_OBJECT());
}

//----------------------------------------------------------------------------
void vtkExecutive::SetOutputData(int newPort, vtkDataObject* newOutput)
{
  vtkInformation *info = this->GetOutputInformation(newPort);
  this->SetOutputData(newPort, newOutput, info);
}

//----------------------------------------------------------------------------
void vtkExecutive::SetOutputData(int newPort, vtkDataObject* newOutput,
                                 vtkInformation* info)
{
  if(info)
  {
    vtkDataObject* currentOutput = info->Get(vtkDataObject::DATA_OBJECT());
    if(newOutput != currentOutput)
    {
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);

      // Output has changed.  Reset the pipeline information.
      this->ResetPipelineInformation(newPort, info);
    }
  }
  else
  {
    vtkErrorMacro("Could not set output on port " << newPort << ".");
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetInputData(int port, int index)
{
  if(index < 0 || index >= this->GetNumberOfInputConnections(port))
  {
    return 0;
  }

  vtkInformationVector* inVector = this->GetInputInformation()[port];
  vtkInformation* info = inVector->GetInformationObject(index);
  vtkExecutive* e;
  int producerPort;
  vtkExecutive::PRODUCER()->Get(info,e,producerPort);
  if(e)
  {
    return e->GetOutputData(producerPort);
  }
  else
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkExecutive::GetInputData
(int port, int index, vtkInformationVector **inInfoVec)
{
  if (!inInfoVec[port])
  {
    return 0;
  }
  vtkInformation *info = inInfoVec[port]->GetInformationObject(index);
  if (!info)
  {
    return 0;
  }
  return info->Get(vtkDataObject::DATA_OBJECT());
}

//----------------------------------------------------------------------------
int vtkExecutive::ProcessRequest(vtkInformation* request,
                                 vtkInformationVector** inInfo,
                                 vtkInformationVector* outInfo)
{
  if(request->Has(FORWARD_DIRECTION()))
  {
    // Request will be forwarded.
    if(request->Get(FORWARD_DIRECTION()) == vtkExecutive::RequestUpstream)
    {
      if(this->Algorithm && request->Get(ALGORITHM_BEFORE_FORWARD()))
      {
        if(!this->CallAlgorithm(request, vtkExecutive::RequestUpstream,
                                inInfo, outInfo))
        {
          return 0;
        }
      }
      if(!this->ForwardUpstream(request))
      {
        return 0;
      }
      if(this->Algorithm && request->Get(ALGORITHM_AFTER_FORWARD()))
      {
        if(!this->CallAlgorithm(request, vtkExecutive::RequestDownstream,
                                inInfo, outInfo))
        {
          return 0;
        }
      }
    }
    if(request->Get(FORWARD_DIRECTION()) == vtkExecutive::RequestDownstream)
    {
      vtkErrorMacro("Downstream forwarding not yet implemented.");
      return 0;
    }
  }
  else
  {
    // Request will not be forwarded.
    vtkErrorMacro("Non-forwarded requests are not yet implemented.");
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkExecutive::ComputePipelineMTime(vtkInformation*,
                                       vtkInformationVector**,
                                       vtkInformationVector*,
                                       int, vtkMTimeType*)
{
  // Demand-driven executives that use this request should implement
  // this method.
  vtkErrorMacro("ComputePipelineMTime not implemented for this executive.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::ForwardDownstream(vtkInformation*)
{
  // Do not forward downstream if the output is shared with another
  // executive.
  if(this->SharedOutputInformation)
  {
    return 1;
  }

  // Forwarding downstream is not yet implemented.
  vtkErrorMacro("ForwardDownstream not yet implemented.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkExecutive::ForwardUpstream(vtkInformation* request)
{
  // Do not forward upstream if the input is shared with another
  // executive.
  if(this->SharedInputInformation)
  {
    return 1;
  }

  if (!this->Algorithm->ModifyRequest(request, BeforeForward))
  {
    return 0;
  }

  // Forward the request upstream through all input connections.
  int result = 1;
  for(int i=0; i < this->GetNumberOfInputPorts(); ++i)
  {
    int nic = this->Algorithm->GetNumberOfInputConnections(i);
    vtkInformationVector* inVector = this->GetInputInformation()[i];
    for(int j=0; j < nic; ++j)
    {
      vtkInformation* info = inVector->GetInformationObject(j);
      // Get the executive producing this input.  If there is none, then
      // it is a NULL input.
      vtkExecutive* e;
      int producerPort;
      vtkExecutive::PRODUCER()->Get(info,e,producerPort);
      if(e)
      {
        int port = request->Get(FROM_OUTPUT_PORT());
        request->Set(FROM_OUTPUT_PORT(), producerPort);
        if(!e->ProcessRequest(request,
                              e->GetInputInformation(),
                              e->GetOutputInformation()))
        {
          result = 0;
        }
        request->Set(FROM_OUTPUT_PORT(), port);
      }
    }
  }

  if (!this->Algorithm->ModifyRequest(request, AfterForward))
  {
    return 0;
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkExecutive::CopyDefaultInformation(vtkInformation* request,
                                          int direction,
                                          vtkInformationVector** inInfoVec,
                                          vtkInformationVector* outInfoVec)
{
  if(direction == vtkExecutive::RequestDownstream)
  {
    // Copy information from the first input to all outputs.
    if(this->GetNumberOfInputPorts() > 0 &&
       inInfoVec[0]->GetNumberOfInformationObjects() > 0)
    {
      vtkInformationKey** keys = request->Get(KEYS_TO_COPY());
      int length = request->Length(KEYS_TO_COPY());
      vtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);

      vtkSmartPointer<vtkInformationIterator> infoIter =
        vtkSmartPointer<vtkInformationIterator>::New();
      infoIter->SetInformationWeak(inInfo);

      int oiobj = outInfoVec->GetNumberOfInformationObjects();
      for(int i=0; i < oiobj; ++i)
      {
        vtkInformation* outInfo = outInfoVec->GetInformationObject(i);
        for(int j=0; j < length; ++j)
        {
          // Copy the entry.
          outInfo->CopyEntry(inInfo, keys[j]);

          // If the entry is a key vector, copy all the keys listed.
          if(vtkInformationKeyVectorKey* vkey =
             vtkInformationKeyVectorKey::SafeDownCast(keys[j]))
          {
            outInfo->CopyEntries(inInfo, vkey);
          }
        }

        // Give the keys an opportunity to copy themselves.
        infoIter->InitTraversal();
        while(!infoIter->IsDoneWithTraversal())
        {
          vtkInformationKey* key = infoIter->GetCurrentKey();
          key->CopyDefaultInformation(request, inInfo, outInfo);
          infoIter->GoToNextItem();
        }
      }
    }
  }
  else
  {
    // Get the output port from which the request was made.  Use zero
    // if output port was not specified.
    int outputPort = 0;
    if(request->Has(FROM_OUTPUT_PORT()))
    {
      outputPort = request->Get(FROM_OUTPUT_PORT());
      outputPort = outputPort == -1 ? 0 : outputPort;
    }

    // Copy information from the requesting output to all inputs.
    if(outputPort >= 0 &&
       outputPort < outInfoVec->GetNumberOfInformationObjects())
    {
      vtkInformationKey** keys = request->Get(KEYS_TO_COPY());
      int length = request->Length(KEYS_TO_COPY());
      vtkInformation* outInfo = outInfoVec->GetInformationObject(outputPort);

      vtkSmartPointer<vtkInformationIterator> infoIter =
        vtkSmartPointer<vtkInformationIterator>::New();
      infoIter->SetInformationWeak(outInfo);

      for(int i=0; i < this->GetNumberOfInputPorts(); ++i)
      {
        for(int j=0; j < inInfoVec[i]->GetNumberOfInformationObjects(); ++j)
        {
          vtkInformation* inInfo = inInfoVec[i]->GetInformationObject(j);
          for(int k=0; k < length; ++k)
          {
            // Copy the entry.
            inInfo->CopyEntry(outInfo, keys[k]);

            // If the entry is a key vector, copy all the keys listed.
            if(vtkInformationKeyVectorKey* vkey =
               vtkInformationKeyVectorKey::SafeDownCast(keys[k]))
            {
              inInfo->CopyEntries(outInfo, vkey);
            }
          }

          // Give the keys an opportunity to copy themselves.
          infoIter->InitTraversal();
          while(!infoIter->IsDoneWithTraversal())
          {
            vtkInformationKey* key = infoIter->GetCurrentKey();
            key->CopyDefaultInformation(request, outInfo, inInfo);
            infoIter->GoToNextItem();
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
int vtkExecutive::CallAlgorithm(vtkInformation* request, int direction,
                                vtkInformationVector** inInfo,
                                vtkInformationVector* outInfo)
{
  // Copy default information in the direction of information flow.
  this->CopyDefaultInformation(request, direction, inInfo, outInfo);

  // Invoke the request on the algorithm.
  this->InAlgorithm = 1;
  int result = this->Algorithm->ProcessRequest(request, inInfo, outInfo);
  this->InAlgorithm = 0;

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

//----------------------------------------------------------------------------
int vtkExecutive::CheckAlgorithm(const char* method,
                                 vtkInformation* request)
{
  if(this->InAlgorithm)
  {
    if(request)
    {
      std::ostringstream rqmsg;
      request->Print(rqmsg);
      vtkErrorMacro(<< method << " invoked during another request.  "
                    "Returning failure to algorithm "
                    << this->Algorithm->GetClassName() << "("
                    << this->Algorithm << ") for the recursive request:\n"
                    << rqmsg.str().c_str());
    }
    else
    {
      vtkErrorMacro(<< method << " invoked during another request.  "
                    "Returning failure to algorithm "
                    << this->Algorithm->GetClassName() << "("
                    << this->Algorithm << ").");
    }

    // Tests should fail when this happens because there is a bug in
    // the code.
    if(getenv("DASHBOARD_TEST_FROM_CTEST") || getenv("DART_TEST_FROM_DART"))
    {
      abort();
    }
    return 0;
  }
  return 1;
}
