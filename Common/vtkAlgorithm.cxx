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
#include "vtkCommand.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkAlgorithm, "1.15");
vtkStandardNewMacro(vtkAlgorithm);

vtkCxxSetObjectMacro(vtkAlgorithm,Information,vtkInformation);

//----------------------------------------------------------------------------
class vtkAlgorithmInternals
{
public:
  // The executive currently managing this algorithm.
  vtkSmartPointer<vtkExecutive> Executive;

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
      this->resize(this->size()+1);
      (end()-1)->Algorithm = algorithm;
      (end()-1)->PortIndex = portIndex;
      }
    void Remove(vtkAlgorithm* algorithm, int portIndex)
      {
      this->erase(this->Find(algorithm, portIndex));
      }

    vtkSmartPointer<vtkInformation> Information;
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
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->ProgressText = NULL;
  this->AlgorithmInternal = new vtkAlgorithmInternals;
  this->GarbageCollecting = 0;
  this->Information = vtkInformation::New();
}

//----------------------------------------------------------------------------
vtkAlgorithm::~vtkAlgorithm()
{
  this->SetInformation(0);
  delete this->AlgorithmInternal;
  delete [] this->ProgressText;
  this->ProgressText = NULL;
}

// Update the progress of the process object. If a ProgressMethod exists,
// executes it. Then set the Progress ivar to amount. The parameter amount
// should range between (0,1).
void vtkAlgorithm::UpdateProgress(double amount)
{
  this->Progress = amount;
  this->InvokeEvent(vtkCommand::ProgressEvent,(void *)&amount);
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

  if ( this->Information )
    {
    os << indent << "Information: " << this->Information << "\n";
    }
  else
    {
    os << indent << "Information: (none)\n";
    }

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";
  if ( this->ProgressText )
    {
    os << indent << "Progress Text: " << this->ProgressText << "\n";
    }
  else
    {
    os << indent << "Progress Text: (None)\n";
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
    vtkExecutive* e = this->CreateDefaultExecutive();
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
int vtkAlgorithm::ProcessUpstreamRequest(vtkInformation*,
                                         vtkInformationVector* inVector,
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
  return 1;
}

//----------------------------------------------------------------------------
int vtkAlgorithm::ProcessDownstreamRequest(vtkInformation*,
                                           vtkInformationVector* inVector,
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
  return 1;
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
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetInput(int index, vtkAlgorithmOutput* input)
{
  this->SetInputConnection(index, input);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkAlgorithm::GetOutputDataObject(int port)
{
  if(!this->OutputPortIndexInRange(port, "get the data object for"))
    {
    return 0;
    }
  return this->GetExecutive()->GetOutputData(this, port);
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(port, "connect"))
    {
    return;
    }

  // Check if the connection is already present.
  if(input &&
     this->AlgorithmInternal->InputPorts[port].size() == 1 &&
     this->AlgorithmInternal->InputPorts[port].Find(input->GetProducer(),
                                                    input->GetIndex()) !=
     this->AlgorithmInternal->InputPorts[port].end())
    {
    // The connection is the only one present.  No change is needed.
    return;
    }
  else if(!input && this->AlgorithmInternal->InputPorts[port].empty())
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
  if(!this->AlgorithmInternal->InputPorts[port].empty())
    {
    vtkDebugMacro("Removing all connections to input port " << port << ".");
    vtkAlgorithm::ConnectionRemoveAllInput(this, port);
    }

  // Add the new connection.
  if(input)
    {
    vtkDebugMacro("Adding connection from output port index "
                  << input->GetIndex() << " on algorithm "
                  << (input->GetProducer()?
                      input->GetProducer()->GetClassName() : "NULL")
                  << "(" << input->GetProducer() << ") to input port "
                  << port << ".");
    vtkAlgorithm::ConnectionAdd(input->GetProducer(), input->GetIndex(),
                                this, port);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(port, "connect"))
    {
    return;
    }

  // Add the new connection.
  vtkDebugMacro("Adding connection from output port index "
                << input->GetIndex() << " on algorithm "
                << (input->GetProducer()?
                    input->GetProducer()->GetClassName() : "NULL")
                << "(" << input->GetProducer() << ") to input port "
                << port << ".");
  vtkAlgorithm::ConnectionAdd(input->GetProducer(), input->GetIndex(),
                              this, port);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(port, "disconnect"))
    {
    return;
    }

  // Check if the connection is present.
  if(!input ||
     this->AlgorithmInternal->InputPorts[port].Find(input->GetProducer(),
                                                    input->GetIndex()) ==
     this->AlgorithmInternal->InputPorts[port].end())
    {
    return;
    }

  // Remove the connection.
  vtkDebugMacro("Removing connection from output port index "
                << input->GetIndex() << " on algorithm "
                << (input->GetProducer()?
                    input->GetProducer()->GetClassName() : "NULL")
                << "(" << input->GetProducer() << ") to input port "
                << port << ".");
  vtkAlgorithm::ConnectionRemove(input->GetProducer(), input->GetIndex(),
                                 this, port);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAlgorithm::SetNthInputConnection(int port, int index,
                                         vtkAlgorithmOutput* input)
{
  if(!this->InputPortIndexInRange(port, "replace connection"))
    {
    return;
    }

  // Check if the connection index exists.
  if(!input || index < 0 || index >= this->GetNumberOfInputConnections(port))
    {
    return;
    }

  // Add the new connection.
  int oldNumberOfConnections = this->GetNumberOfInputConnections(port);
  this->AddInputConnection(port, input);
  if(this->GetNumberOfInputConnections(port) > oldNumberOfConnections)
    {
    // The connection was really added.  Swap it into the correct
    // connection index.
    vtkAlgorithmInternals::PortEntry temp =
      this->AlgorithmInternal->InputPorts[port][index];
    this->AlgorithmInternal->InputPorts[port][index] =
      this->AlgorithmInternal->InputPorts[port][oldNumberOfConnections];
    this->AlgorithmInternal->InputPorts[port][oldNumberOfConnections] = temp;

    // Now remove the connection that was previously at this index.
    this->RemoveInputConnection(port,
                                temp.Algorithm->GetOutputPort(temp.PortIndex));
    }
  else
    {
    // The connection was already present.
    vtkErrorMacro("SetNthInputConnection cannot duplicate another input.");
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkAlgorithm::GetOutputPort(int port)
{
  if(!this->OutputPortIndexInRange(port, "get"))
    {
    return 0;
    }

  // Create the vtkAlgorithmOutput proxy object if there is not one.
  if(!this->AlgorithmInternal->Outputs[port].GetPointer())
    {
    vtkAlgorithmOutput* output = vtkAlgorithmOutput::New();
    output->SetProducer(this);
    output->SetIndex(port);
    this->AlgorithmInternal->Outputs[port] = output;
    output->Delete();
    }

  // Return the proxy object instance.
  return this->AlgorithmInternal->Outputs[port].GetPointer();
}

//----------------------------------------------------------------------------
vtkInformation* vtkAlgorithm::GetInputPortInformation(int port)
{
  if(!this->InputPortIndexInRange(port, "get information object for"))
    {
    return 0;
    }
  if(!this->AlgorithmInternal->InputPorts[port].Information.GetPointer())
    {
    vtkInformation* info = vtkInformation::New();
    if(!this->FillInputPortInformation(port, info))
      {
      info->Clear();
      }
    this->AlgorithmInternal->InputPorts[port].Information = info;
    info->Delete();
    }
  return this->AlgorithmInternal->InputPorts[port].Information.GetPointer();
}

//----------------------------------------------------------------------------
vtkInformation* vtkAlgorithm::GetOutputPortInformation(int port)
{
  if(!this->OutputPortIndexInRange(port, "get information object for"))
    {
    return 0;
    }
  if(!this->AlgorithmInternal->OutputPorts[port].Information.GetPointer())
    {
    vtkInformation* info = vtkInformation::New();
    if(!this->FillOutputPortInformation(port, info))
      {
      info->Clear();
      }
    this->AlgorithmInternal->OutputPorts[port].Information = info;
    info->Delete();
    }
  return this->AlgorithmInternal->OutputPorts[port].Information.GetPointer();
}

//----------------------------------------------------------------------------
int vtkAlgorithm::FillInputPortInformation(int, vtkInformation*)
{
  vtkErrorMacro("FillInputPortInformation is not implemented.");
  return 0;
}

//----------------------------------------------------------------------------
int vtkAlgorithm::FillOutputPortInformation(int, vtkInformation*)
{
  vtkErrorMacro("FillOutputPortInformation is not implemented.");
  return 0;
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
  if(!this->InputPortIndexInRange(port, "get number of connections for"))
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
vtkExecutive* vtkAlgorithm::CreateDefaultExecutive()
{
  return vtkStreamingDemandDrivenPipeline::New();
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
  collector->ReportReference(this->AlgorithmInternal->Executive.GetPointer(),
                             "Executive");
  vtkstd::vector<vtkAlgorithmInternals::Port>::iterator i;

  // Report producers.
  for(i = this->AlgorithmInternal->InputPorts.begin();
      i != this->AlgorithmInternal->InputPorts.end(); ++i)
    {
    for(vtkAlgorithmInternals::Port::iterator j = i->begin();
        j != i->end(); ++j)
      {
      collector->ReportReference(j->Algorithm.GetPointer(), "InputPorts");
      }
    }

  // Report consumers.
  for(i = this->AlgorithmInternal->OutputPorts.begin();
      i != this->AlgorithmInternal->OutputPorts.end(); ++i)
    {
    for(vtkAlgorithmInternals::Port::iterator j = i->begin();
        j != i->end(); ++j)
      {
      collector->ReportReference(j->Algorithm.GetPointer(), "OutputPorts");
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
  this->AlgorithmInternal->InputPorts.clear();
  this->AlgorithmInternal->OutputPorts.clear();
  this->Superclass::RemoveReferences();
}
