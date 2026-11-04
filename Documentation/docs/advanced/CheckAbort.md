# CheckAbort and Progress events

## What is CheckAbort

VTK filter execution can take a while, so being able to abort a running filter is very important.
Aborting is done using `vtkAlgorithm::SetAbortExecute(on)`. However, as the algorithm is computing when we want to abort it,
this is usually called from another thread.

Within the algorithm, `vtkAlgorithm::CheckAbort` can be used to check that flag and setup the pipeline for aborting the algorithm. It is used like this:

```cpp
for (vtkIdType i = 0; !this->CheckAbort() && i < bigNumber; i++)
{
  // Long operation
}
```

This is fine for non-distributed filters, but is not recommended for application working in distributed, because it requires calling
`SetAbortExecute` on each rank at the exact same "step" in the algorithm; distributed algorithms need all ranks to abort at the same time to avoid deadlocks.
This is not feasible in practice.

See [this issue](https://gitlab.kitware.com/vtk/vtk/-/work_items/18463) for more context.

While `CheckAbort` is not deprecated, it is not recommended. Use `CheckAbortAndInvoke` instead.

## What is CheckAbortAndInvoke

As its name indicates, `CheckAbortAndInvoke` merely calls CheckAbort and then invoke a `CheckAbort` event.
An event can be observed by other class to execute code when invoked.
This lets applications using VTK observe this event and react accordingly, usually it means synchronising the state
of `AbortExecute` flag accross distributed ranks.

However this also implies that calling `CheckAbortAndInvoke` will trigger inter process communication, which can be very expensive.
Such event should be invoked infrequently and deliberately. Calling it on every loop iteration would create a lot of overhead.

## What are Progress Events

Progress Events are events invoked by vtkAlgorithm to signal processing progress of the algorithm, with a double progressing from 0.0 to 1.0 .
Applications using VTK can observe this event to show a progress bar.

Progress events are invoked regularly but only a few times, because they can imply communication between a client and a server, similarly as CheckAbortAndInvoke.
Moreover, applications using VTK may rely on that progress event to update the abort status using SetAbortExecute when needed.

This makes progress events and abort checks very similar; ParaView takes advantage of progress events to check for aborts. See [this discourse post](https://discourse.paraview.org/t/paraview-client-server-communication-and-abort-support/17430/3)
for more info.

## Implementation examples

### Simple case

Here is a simple example of a progress and abort check implementation in a filter:

```cpp
vtkIdType numCells = ds->GetNumberOfCells();
if (numCells == 0)
{
  return true;
}

// This harcoded value can vary depending on the complexity of the algorithm.
// The point is to avoid calling CheckAbort too fast even with small datasets
vtkIdType minCheckAbort = 1000;

// We iterate on the cells, so we will check abort and update progress 10 times during computation
vtkIdType checkAbortInterval = std::min(numCells / 10 + 1, minCheckAbort);

for (vtkIdType i = 0; i < numCells; i++)
{
  if (i % checkAbortInterval == 0)
  {
    this->UpdateProgress(static_cast<double>(i) / numCells);
    if (this->CheckAbortAndInvoke())
    {
      break;
    }

  }
  // <Data processing>
}
return true;
```

### Multi step case

Often, algorithms are split in multiple parts, so we can split the progress using `SetProgressShiftScale`.

```cpp
vtkIdType numCells = ds->GetNumberOfCells();
if (numCells == 0)
{
  return true;
}

vtkIdType minCheckAbort = 1000;
vtkIdType checkAbortInterval = std::min(numCells / 10 + 1, minCheckAbort);
bool abort = false;

this->SetProgressShiftScale(0, 0.3) // Progress will move from 0.0 to 0.3
for (vtkIdType i = 0; !abort && i < numCells; i++)
{
  if (i % checkAbortInterval == 0)
  {
    this->UpdateProgress(static_cast<double>(i) / numCells);
    if (this->CheckAbortAndInvoke())
    {
      abort = true;
      break;
    }

  }
  // Some short operations
}

if (abort)
{
  return true; // Aborting is not a failure case
}

this->UpdateProgress(1.0); // Not absolutely needed but in some case it may be useful to ensure the threshold is actually reached

if (this->CheckAbortAndInvoke()) // Ok to check anytime
{
  return true;
}

this->SetProgressShiftScale(0.3, 0.7) // Progress will move from 0.3 to 1.0
for (vtkIdType i = 0; i < numCells; i++)
{
  if (i % checkAbortInterval == 0)
  {
    this->UpdateProgress(static_cast<double>(i) / numCells);
    if (this->CheckAbortAndInvoke())
    {
      break;
    }
  }
  // Some long operations
}
return true;
```

### Multithreaded case

VTK Algorithms often multi-thread their critical loops. Here is how to abort during a multi-threaded worker execution:

```cpp
void operator()(vtkIdType begin, vtkIdType end)
{
  const bool isFirst = vtkSMPTools::GetSingleThread();
  constexpr vtkIdType minCheckAbortInterval = 1'000'000; // minCheckAbortInterval can go pretty high if the algorithm is fast
  vtkIdType checkAbortInterval = std::min(this->NumberOfCells / 10 + 1, minCheckAbortInterval);
  for (vtkIdType cellId = begin; cellId < end; ++cellId) // There is no insurance that begin and end will progress over time, but thats the best we got
  {
    if (isFirst && cellId % checkAbortInterval == 0) // Only the first thread should check
    {
      this->Self->UpdateProgress(static_cast<double>(cellId) / this->NumberOfCells); // The progress may be a bit erratic but generally it works
      if (this->Self->CheckAbortAndInvoke())
      {
        break;
      }
    }
    else if (this->Self->GetAbortOutput()) // All threads should abort
    {
      break;
    }
  }
}

[...]

MyWorker worker;
worker();
return 1;
}

```

### Internal filter case

Algorithm often use internal filters, that use their own CheckAbortAndInvoke and progress event, which should be forwarded to the main filter.

```cpp
namespace
{
void OnAbortCheckCallback(
  vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientData, void* vtkNotUsed(callData))
{
  vtkAlgorithm* internalAlgorithm = static_cast<vtkAlgorithm*>(caller);
  vtkMyFilter* self = static_cast<vtkMyFilter*>(clientData);

  if (self->CheckAbortAndInvoke())
  {
    internalAlgorithm->AbortExecuteOn();
  }
}
}

vtkMyFilter::vtkMyFilter()
{
  vtkNew<vtkEventForwarderCommand> forwarder;
  forwarder->SetTarget(this)
  this->InternalAlgorithm->AddObserver(vtkCommand::ProgressEvent, forwarder);

  this->InternalAbortCheckCallbackCommand->SetCallback(::OnAbortCheckCallback);
  this->InternalAbortCheckCallbackCommand->SetClientData(this);
  this->IntenalAlgorithm->AddObserver(
    vtkCommand::AbortCheckEvent, this->InternalAbortCheckCallbackCommand);
}

[...]

int vtkMyFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Make sure all filters are not aborted at this point
  this->InternalAlgorithm->AbortExecuteOff();

  // Execute internal filter
  return this->InternalAlgorithm->ProcessRequest(request, inputVector, outputVector);
}
```
