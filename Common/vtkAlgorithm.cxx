/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAlgorithm.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDistributedExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkAlgorithm, "1.2");
vtkStandardNewMacro(vtkAlgorithm);

//----------------------------------------------------------------------------
class vtkAlgorithmInternals
{
public:
  // The executive currently managing this algorithm.
  vtkSmartPointer<vtkExecutive> Executive;

  // Port information for this algorithm.
  vtkSmartPointer<vtkInformationVector> InputPortInformation;
  vtkSmartPointer<vtkInformationVector> OutputPortInformation;

  // Connections are stored at each end by pointing at the algorithm
  // and input/output port index of the other end of the connection.
  struct PortEntry
  {
    vtkSmartPointer< vtkAlgorithm > Algorithm;
    int PortIndex;
  };

  // An output port may be connected to zero or more consumers.  An
  // input port may be connected to zero or more producers.
  struct Port: public vtkstd::vector<PortEntry>
  {
    iterator Find(vtkAlgorithm* algorithm, int portIndex)
      {
      for(iterator i = begin(); i != end(); ++i)
        {
        if(i->Algorithm == algorithm && i->PortIndex == portIndex)
          {
          return i;
          }
        }
      return this->end();
      }
    void Insert(vtkAlgorithm* algorithm, int portIndex)
      {
      if(this->Find(algorithm, portIndex) == end())
        {
        this->resize(this->size()+1);
        (end()-1)->Algorithm = algorithm;
        (end()-1)->PortIndex = portIndex;
        }
      }
    void Remove(vtkAlgorithm* algorithm, int portIndex)
      {
      this->erase(this->Find(algorithm, portIndex));
      }
  };

  // Each algorithm has zero or more input ports and zero or more
  // output ports.
  vtkstd::vector<Port> InputPorts;
  vtkstd::vector<Port> OutputPorts;

  // Proxy object instances for use in establishing connections from
  // the output ports to other algorithms.
  vtkstd::vector< vtkSmartPointer<vtkAlgorithmOutput> > Outputs;
};

static void vtkAlgorithmIgnoreUnused(void*) {}

//----------------------------------------------------------------------------
class vtkAlgorithmToExecutiveFriendship
{
public:
  static void AddAlgorithm(vtkExecutive* executive, vtkAlgorithm* algorithm)
    {
    executive->AddAlgorithm(algorithm);
    }
  static void RemoveAlgorithm(vtkExecutive* executive, vtkAlgorithm* algorithm)
    {
    executive->RemoveAlgorithm(algorithm);
    }
};

//----------------------------------------------------------------------------
void vtkAlgorithm::ConnectionAdd(vtkAlgorithm* producer, int producerPort,
                                 vtkAlgorithm* consumer, int consumerPort)
{
  // Add the consumer's reference to the producer.
  consumer->AlgorithmInternal
    ->InputPorts[consumerPort].Insert(producer, producerPort);

  // Add the producer's reference to the consumer.
  producer->AlgorithmInternal
    ->OutputPorts[producerPort].Insert(consumer, consumerPort);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::ConnectionRemove(vtkAlgorithm* producer, int producerPort,
                                    vtkAlgorithm* consumer, int consumerPort)
{
  // Remove the consumer's reference to the producer.
  consumer->AlgorithmInternal
    ->InputPorts[consumerPort].Remove(producer, producerPort);

  // Remove the producer's reference to the consumer.
  producer->AlgorithmInternal
    ->OutputPorts[producerPort].Remove(consumer, consumerPort);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::ConnectionRemoveAllInput(vtkAlgorithm* consumer, int port)
{
  vtkAlgorithmInternals::Port& inputPort =
    consumer->AlgorithmInternal->InputPorts[port];

  // Remove all producers' references to this consumer.
  for(vtkAlgorithmInternals::Port::iterator i = inputPort.begin();
      i != inputPort.end(); ++i)
    {
    i->Algorithm->AlgorithmInternal
      ->OutputPorts[i->PortIndex].Remove(consumer, port);
    }

  // Remove this consumer's references to all producers.
  inputPort.clear();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::ConnectionRemoveAllOutput(vtkAlgorithm* producer, int port)
{
  vtkAlgorithmInternals::Port& outputPort =
    producer->AlgorithmInternal->OutputPorts[port];

  // Remove all consumers' references to this producer.
  for(vtkAlgorithmInternals::Port::iterator i = outputPort.begin();
      i != outputPort.end(); ++i)
    {
    i->Algorithm->AlgorithmInternal
      ->InputPorts[i->PortIndex].Remove(producer, port);
    }

  // Remove this producer's references to all consumers.
  outputPort.clear();
}

//----------------------------------------------------------------------------
vtkAlgorithm::vtkAlgorithm()
{
  this->AlgorithmInternal = new vtkAlgorithmInternals;
  this->GarbageCollecting = 0;
}

//----------------------------------------------------------------------------
vtkAlgorithm::~vtkAlgorithm()
{
  delete this->AlgorithmInternal;
}

//----------------------------------------------------------------------------
void vtkAlgorithm::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->HasExecutive())
    {
    os << indent << "Executive: "
       << this->AlgorithmInternal->Executive.GetPointer() << "\n";
    }
  else
    {
    os << indent << "Executive: (none)\n";
    }
}

//----------------------------------------------------------------------------
int vtkAlgorithm::HasExecutive()
{
  return this->AlgorithmInternal->Executive.GetPointer()? 1:0;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkAlgorithm::GetExecutive()
{
  // Create the default executive if we do not have one already.
  if(!this->HasExecutive())
    {
    vtkDistributedExecutive* e = vtkDistributedExecutive::New();
    this->SetExecutive(e);
    e->Delete();
    }
  return this->AlgorithmInternal->Executive.GetPointer();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetExecutive(vtkExecutive* executive)
{
  if(vtkExecutive* oldExecutive =
     this->AlgorithmInternal->Executive.GetPointer())
    {
    // If this algorithm is already managed by the executive, do
    // nothing.
    if(executive == oldExecutive)
      {
      return;
      }

    // The old executive is no longer managing this algorithm.
    vtkAlgorithmToExecutiveFriendship::RemoveAlgorithm(oldExecutive, this);
    }

  // The given executive now manages this algorithm.
  this->AlgorithmInternal->Executive = executive;
  if(executive)
    {
    vtkAlgorithmToExecutiveFriendship::AddAlgorithm(executive, this);
    }
}

//----------------------------------------------------------------------------
int vtkAlgorithm::ProcessUpstreamRequest(vtkInformationVector* inVector,
                                         vtkInformationVector* outVector)
{
  if(!inVector)
    {
    vtkErrorMacro("ProcessUpstreamRequest called with NULL input vector.");
    return 0;
    }
  if(!outVector)
    {
    vtkErrorMacro("ProcessUpstreamRequest called with NULL output vector.");
    return 0;
    }
  inVector->DeepCopy(outVector);
  return 0;
}

//----------------------------------------------------------------------------
int vtkAlgorithm::ProcessDownstreamRequest(vtkInformationVector* inVector,
                                           vtkInformationVector* outVector)
{
  if(!inVector)
    {
    vtkErrorMacro("ProcessDownstreamRequest called with NULL input vector.");
    return 0;
    }
  if(!outVector)
    {
    vtkErrorMacro("ProcessDownstreamRequest called with NULL output vector.");
    return 0;
    }
  outVector->DeepCopy(inVector);
  return 0;
}

//----------------------------------------------------------------------------
int vtkAlgorithm::GetNumberOfInputPorts()
{
  return static_cast<int>(this->AlgorithmInternal->InputPorts.size());
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetNumberOfInputPorts(int n)
{
  // Sanity check.
  if(n < 0)
    {
    vtkErrorMacro("Attempt to set number of input ports to " << n);
    n = 0;
    }

  // We must remove all connections from ports that are removed.
  for(int i=n; i < this->GetNumberOfInputPorts(); ++i)
    {
    vtkAlgorithm::ConnectionRemoveAllInput(this, i);
    }
  this->AlgorithmInternal->InputPorts.resize(n);
  this->AlgorithmInternal->InputPortInformation
    ->SetNumberOfInformationObjects(n);
}

//----------------------------------------------------------------------------
int vtkAlgorithm::GetNumberOfOutputPorts()
{
  return static_cast<int>(this->AlgorithmInternal->OutputPorts.size());
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetNumberOfOutputPorts(int n)
{
  // Sanity check.
  if(n < 0)
    {
    vtkErrorMacro("Attempt to set number of output ports to " << n);
    n = 0;
    }

  // We must remove all connections from ports that are removed.
  for(int i=n; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkAlgorithm::ConnectionRemoveAllOutput(this, i);
    }
  this->AlgorithmInternal->OutputPorts.resize(n);
  this->AlgorithmInternal->Outputs.resize(n);
  this->AlgorithmInternal->OutputPortInformation
    ->SetNumberOfInformationObjects(n);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetInput(int index, vtkAlgorithmOutput* input)
{
  this->SetInputConnection(index, input);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkAlgorithm::GetOutput(int index)
{
  return this->GetOutputPort(index);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetInputConnection(int index, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(index, "connect"))
    {
    return;
    }

  // Check if the connection is already present.
  if(input &&
     this->AlgorithmInternal->InputPorts[index].size() == 1 &&
     this->AlgorithmInternal->InputPorts[index].Find(input->GetProducer(),
                                                     input->GetIndex()) !=
     this->AlgorithmInternal->InputPorts[index].end())
    {
    // The connection is the only one present.  No change is needed.
    return;
    }
  else if(!input && this->AlgorithmInternal->InputPorts[index].empty())
    {
    // New connection is NULL and there are no connections to remove.
    return;
    }

  // Hold an extra reference to this object and the producer of the
  // new input in case an existing connection is the only reference to
  // either.
  vtkSmartPointer<vtkAlgorithm> consumer = this;
  vtkSmartPointer<vtkAlgorithm> producer = input?input->GetProducer():0;
  vtkAlgorithmIgnoreUnused(&consumer);
  vtkAlgorithmIgnoreUnused(&producer);

  // Remove all other connections.
  if(!this->AlgorithmInternal->InputPorts[index].empty())
    {
    vtkAlgorithm::ConnectionRemoveAllInput(this, index);
    }

  // Add the new connection.
  if(input)
    {
    vtkAlgorithm::ConnectionAdd(input->GetProducer(), input->GetIndex(),
                                this, index);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::AddInputConnection(int index, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(index, "connect"))
    {
    return;
    }

  // Check if the connection is already present.
  if(!input ||
     this->AlgorithmInternal->InputPorts[index].Find(input->GetProducer(),
                                                     input->GetIndex()) !=
     this->AlgorithmInternal->InputPorts[index].end())
    {
    return;
    }

  // Add the new connection.
  vtkAlgorithm::ConnectionAdd(input->GetProducer(), input->GetIndex(),
                              this, index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::RemoveInputConnection(int index, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(index, "disconnect"))
    {
    return;
    }

  // Check if the connection is present.
  if(!input ||
     this->AlgorithmInternal->InputPorts[index].Find(input->GetProducer(),
                                                     input->GetIndex()) ==
     this->AlgorithmInternal->InputPorts[index].end())
    {
    return;
    }

  // Remove the connection.
  vtkAlgorithm::ConnectionRemove(input->GetProducer(), input->GetIndex(),
                                 this, index);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkAlgorithm::GetOutputPort(int index)
{
  if(!this->OutputPortIndexInRange(index, "get"))
    {
    return 0;
    }

  // Create the vtkAlgorithmOutput proxy object if there is not one.
  if(!this->AlgorithmInternal->Outputs[index].GetPointer())
    {
    vtkAlgorithmOutput* output = vtkAlgorithmOutput::New();
    output->SetProducer(this);
    output->SetIndex(index);
    this->AlgorithmInternal->Outputs[index] = output;
    output->Delete();
    }

  // Return the proxy object instance.
  return this->AlgorithmInternal->Outputs[index].GetPointer();
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkAlgorithm::GetInputPortInformation()
{
  if(!this->AlgorithmInternal->InputPortInformation.GetPointer())
    {
    vtkInformationVector* infoVector = vtkInformationVector::New();
    this->AlgorithmInternal->InputPortInformation = infoVector;
    this->FillInputPortInformation(infoVector);
    infoVector->Delete();
    }
  return this->AlgorithmInternal->InputPortInformation.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformationVector* vtkAlgorithm::GetOutputPortInformation()
{
  if(!this->AlgorithmInternal->OutputPortInformation.GetPointer())
    {
    vtkInformationVector* infoVector = vtkInformationVector::New();
    this->AlgorithmInternal->OutputPortInformation = infoVector;
    this->FillOutputPortInformation(infoVector);
    infoVector->Delete();
    }
  return this->AlgorithmInternal->OutputPortInformation.GetPointer();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::FillInputPortInformation(vtkInformationVector*)
{
  vtkErrorMacro("vtkAlgorithm subclasses must have FillInputPortInformation.");
}

//----------------------------------------------------------------------------
void vtkAlgorithm::FillOutputPortInformation(vtkInformationVector*)
{
  vtkErrorMacro("vtkAlgorithm subclasses must have FillOutputPortInformation.");
}

//----------------------------------------------------------------------------
int vtkAlgorithm::GetNumberOfInputConnections(int port)
{
  if(!this->InputPortIndexInRange(port, "get number of connections for"))
    {
    return 0;
    }
  return static_cast<int>(this->AlgorithmInternal->InputPorts[port].size());
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkAlgorithm::GetInputConnection(int port, int index)
{
  if(!this->InputPortIndexInRange(index, "get number of connections for"))
    {
    return 0;
    }
  if(index < 0 || index >= this->GetNumberOfInputConnections(port))
    {
    vtkErrorMacro("Attempt to get connection index " << index
                  << " for input port " << port << ", which has "
                  << this->GetNumberOfInputConnections(port)
                  << " connections.");
    return 0;
    }
  vtkAlgorithmInternals::Port& inputPort =
    this->AlgorithmInternal->InputPorts[port];
  return inputPort[index].Algorithm->GetOutputPort(inputPort[index].PortIndex);
}

//----------------------------------------------------------------------------
int vtkAlgorithm::InputPortIndexInRange(int index, const char* action)
{
  // Make sure the index of the input port is in range.
  if(index < 0 || index >= this->GetNumberOfInputPorts())
    {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " input port index " << index
                  << " for an algorithm with "
                  << this->GetNumberOfInputPorts() << " input ports.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAlgorithm::OutputPortIndexInRange(int index, const char* action)
{
  // Make sure the index of the output port is in range.
  if(index < 0 || index >= this->GetNumberOfOutputPorts())
    {
    vtkErrorMacro("Attempt to " << (action?action:"access")
                  << " output port index " << index
                  << " for an algorithm with "
                  << this->GetNumberOfOutputPorts() << " output ports.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkAlgorithm::Update()
{
  this->GetExecutive()->Update(this);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::UnRegister(vtkObjectBase* o)
{
  int check = (this->GetReferenceCount() > 1);
  this->Superclass::UnRegister(o);
  if(check && !this->GarbageCollecting)
    {
    vtkGarbageCollector::Check(this);
    }
}

//----------------------------------------------------------------------------
void vtkAlgorithm::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  collector->ReportReference(this->AlgorithmInternal->Executive.GetPointer());
  vtkstd::vector<vtkAlgorithmInternals::Port>::iterator i;

  // Report producers.
  for(i = this->AlgorithmInternal->InputPorts.begin();
      i != this->AlgorithmInternal->InputPorts.end(); ++i)
    {
    for(vtkAlgorithmInternals::Port::iterator j = i->begin();
        j != i->end(); ++j)
      {
      collector->ReportReference(j->Algorithm.GetPointer());
      }
    }

  // Report consumers.
  for(i = this->AlgorithmInternal->OutputPorts.begin();
      i != this->AlgorithmInternal->OutputPorts.end(); ++i)
    {
    for(vtkAlgorithmInternals::Port::iterator j = i->begin();
        j != i->end(); ++j)
      {
      collector->ReportReference(j->Algorithm.GetPointer());
      }
    }
}

//----------------------------------------------------------------------------
void vtkAlgorithm::GarbageCollectionStarting()
{
  this->GarbageCollecting = 1;
  this->Superclass::GarbageCollectionStarting();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::RemoveReferences()
{
  this->AlgorithmInternal->Executive = 0;
  this->Superclass::RemoveReferences();
}
