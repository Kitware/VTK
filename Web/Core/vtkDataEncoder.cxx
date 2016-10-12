/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataEncoder.h"

#include "vtkBase64Utilities.h"
#include "vtkCommand.h"
#include "vtkConditionVariable.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkMultiThreader.h"
#include "vtkMutexLock.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>
#include <cmath>
#include <map>
#include <vector>

#include <vtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 3
//****************************************************************************
namespace
{
  class vtkSharedData
  {
  public:
    struct OutputValueType
    {
    public:
      vtkTypeUInt32 TimeStamp;
      vtkSmartPointer<vtkUnsignedCharArray> Data;
      OutputValueType() : TimeStamp(0), Data(NULL)
      {
      }
    };

    struct InputValueType
    {
    public:
      vtkTypeUInt32 OutputStamp;
      vtkSmartPointer<vtkImageData> Image;
      int Quality;
      InputValueType() : OutputStamp(0), Image(NULL), Quality(100)
      {
      }
    };

    typedef std::map<vtkTypeUInt32, InputValueType > InputMapType;
    typedef std::map<vtkTypeUInt32, OutputValueType> OutputMapType;
  private:
    bool Done;
    vtkSimpleMutexLock DoneLock;
    vtkSimpleMutexLock OutputsLock;
    vtkSimpleConditionVariable OutputsAvailable;

    vtkSimpleMutexLock ThreadDoneLock;
    vtkSimpleConditionVariable ThreadDone;
    int ActiveThreadCount;

    //------------------------------------------------------------------------
    // OutputsLock must be held before accessing any of the following members.
    OutputMapType Outputs;

    //------------------------------------------------------------------------
    // Constructs used to synchronization.
    vtkSimpleMutexLock InputsLock;
    vtkSimpleConditionVariable InputsAvailable;

    //------------------------------------------------------------------------
    // InputsLock must be held before accessing any of the following members.
    InputMapType Inputs;

  public:
    //------------------------------------------------------------------------
    vtkSharedData() : Done(false), ActiveThreadCount(0)
    {
    }

    //------------------------------------------------------------------------
    // Each thread should call this method when it starts. It helps us clean up
    // threads when they are done.
    void BeginWorker()
    {
      this->ThreadDoneLock.Lock();
      this->ActiveThreadCount++;
      this->ThreadDoneLock.Unlock();
    }

    //------------------------------------------------------------------------
    // Each thread should call this method when it ends.
    void EndWorker()
    {
      this->ThreadDoneLock.Lock();
      this->ActiveThreadCount--;
      bool last_thread = (this->ActiveThreadCount == 0);
      this->ThreadDoneLock.Unlock();
      if (last_thread)
      {
        this->ThreadDone.Signal();
      }
    }

    //------------------------------------------------------------------------
    void RequestAndWaitForWorkersToEnd()
    {
      // Get the done lock so we other threads don't end up testing the Done
      // flag and quitting before this thread starts to wait for them to quit.
      this->DoneLock.Lock();
      this->Done = true;

      // Grab the ThreadDoneLock. so even if any thread ends up check this->Done
      // as soon as we release the lock, it won't get a chance to terminate.
      this->ThreadDoneLock.Lock();

      // release the done lock. Let threads test for this->Done flag.
      this->DoneLock.Unlock();

      // Tell all workers that inputs are available, so they will try to check
      // the input as well as the done flag.
      this->InputsAvailable.Broadcast();

      // Now wait for thread to terminate releasing this->ThreadDoneLock as soon
      // as we start waiting. Thus, no threads have got a chance to call
      // EndWorker() till the main thread starts waiting for them.
      this->ThreadDone.Wait(this->ThreadDoneLock);

      this->ThreadDoneLock.Unlock();

      // reset Done flag since all threads have died.
      this->Done = false;
    }

    //------------------------------------------------------------------------
    bool IsDone()
    {
      this->DoneLock.Lock();
      bool val  = this->Done;
      this->DoneLock.Unlock();
      return val;
    }

    //------------------------------------------------------------------------
    void PushAndTakeReference(vtkTypeUInt32 key, vtkImageData* &data,
      vtkTypeUInt64 stamp, int quality)
    {
      this->InputsLock.Lock();
      {
        vtkSharedData::InputValueType &value = this->Inputs[key];
        value.Image.TakeReference(data);
        value.OutputStamp = stamp;
        value.Quality = quality;
        data = NULL;
      }
      this->InputsLock.Unlock();
      this->InputsAvailable.Signal();
    }

    //------------------------------------------------------------------------
    vtkTypeUInt64 GetExpectedOutputStamp(vtkTypeUInt32 key)
    {
      vtkTypeUInt64 stamp = 0;
      this->InputsLock.Lock();
      vtkSharedData::InputMapType::iterator iter = this->Inputs.find(key);
      if (iter != this->Inputs.end())
      {
        stamp = iter->second.OutputStamp;
      }
      this->InputsLock.Unlock();
      return stamp;
    }

    //------------------------------------------------------------------------
    // NOTE: This method may suspend the calling thread until inputs become
    // available.
    vtkTypeUInt64 GetNextInputToProcess(vtkTypeUInt32& key,
      vtkSmartPointer<vtkImageData>& image, int& quality)
    {
      vtkTypeUInt32 stamp = 0;

      this->InputsLock.Lock();
      do
      {
        // Check if we have an input available, if so, return it.
        InputMapType::iterator iter;
        for (iter = this->Inputs.begin(); iter != this->Inputs.end(); ++iter)
        {
          if (iter->second.Image.GetPointer() != NULL)
          {
            key = iter->first;
            image = iter->second.Image;
            iter->second.Image = NULL;
            stamp = iter->second.OutputStamp;
            quality = iter->second.Quality;
            break;
          }
        }
        if (image.GetPointer() == NULL && !this->IsDone())
        {
          // No data is available, let's wait till it becomes available.
          this->InputsAvailable.Wait(this->InputsLock);
        }

      } while (image.GetPointer() == NULL && !this->IsDone());

      this->InputsLock.Unlock();
      return stamp;
    }

    //------------------------------------------------------------------------
    void SetOutputReference(const vtkTypeUInt32 &key,
      vtkTypeUInt64 timestamp, vtkUnsignedCharArray* &dataRef)
    {
      this->OutputsLock.Lock();
      assert(dataRef->GetReferenceCount() == 1);
      OutputMapType::iterator iter = this->Outputs.find(key);
      if (iter == this->Outputs.end() ||
        iter->second.Data.GetPointer() == NULL ||
        iter->second.TimeStamp < timestamp)
      {
        //cout << "Done: " <<
        //  vtkMultiThreader::GetCurrentThreadID() << " "
        //  << key << ", " << timestamp << endl;
        this->Outputs[key].TimeStamp = timestamp;
        this->Outputs[key].Data.TakeReference(dataRef);
        dataRef = NULL;
      }
      else
      {
        dataRef->Delete();
        dataRef = NULL;
      }
      this->OutputsLock.Unlock();
      this->OutputsAvailable.Broadcast();
    }

    //------------------------------------------------------------------------
    bool CopyLatestOutputIfDifferent(
      vtkTypeUInt32 key, vtkUnsignedCharArray* data)
    {
      vtkTypeUInt64 dataTimeStamp = 0;
      this->OutputsLock.Lock();
      {
        const vtkSharedData::OutputValueType &output = this->Outputs[key];
        if (output.Data.GetPointer() != NULL &&
          (output.Data->GetMTime() > data->GetMTime() ||
           output.Data->GetNumberOfTuples() != data->GetNumberOfTuples()))
        {
          data->DeepCopy(output.Data.GetPointer());
          data->Modified();
        }
        dataTimeStamp = output.TimeStamp;
      }
      this->OutputsLock.Unlock();

      vtkTypeUInt64 outputTS = 0;

      this->InputsLock.Lock();
      vtkSharedData::InputMapType::iterator iter = this->Inputs.find(key);
      if (iter != this->Inputs.end())
      {
        outputTS = iter->second.OutputStamp;
      }
      this->InputsLock.Unlock();

      return (dataTimeStamp >= outputTS);
    }

    //------------------------------------------------------------------------
    void Flush(vtkTypeUInt32 key, vtkTypeUInt64 timestamp)
    {
      this->OutputsLock.Lock();
      while (this->Outputs[key].TimeStamp < timestamp)
      {
        // output is not yet ready, we have to wait.
        this->OutputsAvailable.Wait(this->OutputsLock);
      }

      this->OutputsLock.Unlock();
    }
  };

  VTK_THREAD_RETURN_TYPE Worker(void *calldata)
  {
    //cout << "Start Thread: " << vtkMultiThreader::GetCurrentThreadID() << endl;
    vtkMultiThreader::ThreadInfo* info =
      reinterpret_cast<vtkMultiThreader::ThreadInfo*>(calldata);
    vtkSharedData* sharedData = reinterpret_cast<vtkSharedData*>(info->UserData);

    sharedData->BeginWorker();

    while (true)
    {
      vtkTypeUInt32 key = 0;
      vtkSmartPointer<vtkImageData> image;
      vtkTypeUInt64 timestamp = 0;
      int quality = 100;

      timestamp = sharedData->GetNextInputToProcess(key, image, quality);

      if (timestamp == 0 || image.GetPointer() == NULL)
      {
        // end thread.
        break;
      }

      //cout << "Working Thread: " << vtkMultiThreader::GetCurrentThreadID() << endl;

      // Do the encoding.
      vtkNew<vtkJPEGWriter> writer;
      writer->WriteToMemoryOn();
      writer->SetInputData(image);
      writer->SetQuality(quality);
      writer->Write();
      vtkUnsignedCharArray* data = writer->GetResult();

      vtkUnsignedCharArray* result = vtkUnsignedCharArray::New();
      result->SetNumberOfComponents(1);
      result->SetNumberOfTuples(std::ceil(1.5 * data->GetNumberOfTuples()));
      unsigned long size = vtkBase64Utilities::Encode(
        data->GetPointer(0),
        data->GetNumberOfTuples(),
        result->GetPointer(0), /*mark_end=*/ 0);
      result->SetNumberOfTuples(static_cast<vtkIdType>(size)+1);
      result->SetValue(size, 0);

      // Pass over the "result" reference.
      sharedData->SetOutputReference(key, timestamp, result);
      assert(result == NULL);
    }

    //cout << "Closing Thread: " << vtkMultiThreader::GetCurrentThreadID() << endl;
    sharedData->EndWorker();
    return VTK_THREAD_RETURN_VALUE;
  }
}

//****************************************************************************
class vtkDataEncoder::vtkInternals
{
private:
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkUnsignedCharArray> > ClonedOutputs;
public:
  vtkNew<vtkMultiThreader> Threader;
  vtkSharedData SharedData;
  vtkTypeUInt64 Counter;

  vtkSmartPointer<vtkUnsignedCharArray> lastBase64Image;

  vtkInternals() : Counter(0)
  {
    lastBase64Image = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }

  void TerminateAllWorkers()
  {
    // request and wait for all threads to close.
    this->SharedData.RequestAndWaitForWorkersToEnd();
  }

  void SpawnWorkers()
  {
    for (int cc=0; cc < MAX_NUMBER_OF_THREADS_IN_POOL; cc++)
    {
      this->Threader->SpawnThread(&Worker, &this->SharedData);
    }
  }

  // Since changes to vtkObjectBase::ReferenceCount are not thread safe, we have
  // this level of indirection between the outputs stored in SharedData and
  // passed back to the user/main thread.
  bool GetLatestOutput(
    vtkTypeUInt32 key, vtkSmartPointer<vtkUnsignedCharArray>& data)
  {
    vtkSmartPointer<vtkUnsignedCharArray>& output = this->ClonedOutputs[key];
    if (!output)
    {
      output = vtkSmartPointer<vtkUnsignedCharArray>::New();
    }
    data = output;
    return this->SharedData.CopyLatestOutputIfDifferent(key, data);
  }

  // Once an imagedata has been written to memory as a jpg or png, this
  // convenience function can encode that image as a Base64 string.
  const char* GetBase64EncodedImage(vtkUnsignedCharArray* encodedInputImage)
  {
    this->lastBase64Image->SetNumberOfComponents(1);
    this->lastBase64Image->SetNumberOfTuples(std::ceil(1.5 * encodedInputImage->GetNumberOfTuples()));
    unsigned long size = vtkBase64Utilities::Encode(
      encodedInputImage->GetPointer(0),
      encodedInputImage->GetNumberOfTuples(),
      this->lastBase64Image->GetPointer(0), /*mark_end=*/ 0);

    this->lastBase64Image->SetNumberOfTuples(static_cast<vtkIdType>(size)+1);
    this->lastBase64Image->SetValue(size, 0);

    return reinterpret_cast<char*>(this->lastBase64Image->GetPointer(0));
  }
};

vtkStandardNewMacro(vtkDataEncoder);
//----------------------------------------------------------------------------
vtkDataEncoder::vtkDataEncoder() :
  Internals(new vtkInternals())
{
  this->Internals->SpawnWorkers();
}

//----------------------------------------------------------------------------
vtkDataEncoder::~vtkDataEncoder()
{
  this->Internals->TerminateAllWorkers();
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkDataEncoder::Initialize()
{
  this->Internals->TerminateAllWorkers();
  this->Internals->SpawnWorkers();
}

//----------------------------------------------------------------------------
void vtkDataEncoder::PushAndTakeReference(vtkTypeUInt32 key, vtkImageData* &data, int quality)
{
  // if data->ReferenceCount != 1, it means the caller thread is keep an extra
  // reference and that's bad.
  assert(data->GetReferenceCount() == 1);

  this->Internals->SharedData.PushAndTakeReference(
    key, data, ++this->Internals->Counter, quality);
  assert(data == NULL);
}

//----------------------------------------------------------------------------
bool vtkDataEncoder::GetLatestOutput(
  vtkTypeUInt32 key, vtkSmartPointer<vtkUnsignedCharArray>& data)
{
  return this->Internals->GetLatestOutput(key, data);
}

//----------------------------------------------------------------------------
const char* vtkDataEncoder::EncodeAsBase64Png(vtkImageData* img, int compressionLevel)
{
  // Perform in-memory write of image as png
  vtkNew<vtkPNGWriter> writer;
  writer->WriteToMemoryOn();
  writer->SetInputData(img);
  writer->SetCompressionLevel(compressionLevel);
  writer->Write();

  // Return Base64-encoded string
  return this->Internals->GetBase64EncodedImage(writer->GetResult());
}

//----------------------------------------------------------------------------
const char* vtkDataEncoder::EncodeAsBase64Jpg(vtkImageData* img, int quality)
{
  // Perform in-memory write of image as jpg
  vtkNew<vtkJPEGWriter> writer;
  writer->WriteToMemoryOn();
  writer->SetInputData(img);
  writer->SetQuality(quality);
  writer->Write();

  // Return Base64-encoded string
  return this->Internals->GetBase64EncodedImage(writer->GetResult());
}

//----------------------------------------------------------------------------
void vtkDataEncoder::Flush(vtkTypeUInt32 key)
{
  vtkTypeUInt64 outputTS =
    this->Internals->SharedData.GetExpectedOutputStamp(key);
  if (outputTS != 0)
  {
    // Now wait till we see the outputTS in the output for key.
    //cout << "Wait till : " << outputTS << endl;
    this->Internals->SharedData.Flush(key, outputTS);
  }
}

//----------------------------------------------------------------------------
void vtkDataEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
