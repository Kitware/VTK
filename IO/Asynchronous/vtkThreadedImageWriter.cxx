/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreadedImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkThreadedImageWriter.h"

#include "vtkBMPWriter.h"
#include "vtkCommand.h"
#include "vtkConditionVariable.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkPointData.h"
#include "vtkTIFFWriter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkZLibDataCompressor.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <queue>

#include <vtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 32
//****************************************************************************
namespace
{
class vtkSharedData
{
public:
  struct InputValueType
  {
  public:
    vtkSmartPointer<vtkImageData> Image;
    std::string FileName;

    InputValueType()
      : Image(nullptr)
      , FileName("")
    {
    }
  };

  typedef std::queue<InputValueType> InputQueueType;
  typedef std::map<vtkTypeUInt32, InputValueType> InputMapType;

private:
  bool Done;
  vtkSimpleMutexLock DoneLock;

  vtkSimpleMutexLock ThreadDoneLock;
  vtkSimpleConditionVariable ThreadDone;
  int ActiveThreadCount;

  //------------------------------------------------------------------------
  // Constructs used to synchronization.
  vtkSimpleMutexLock InputsLock;
  vtkSimpleConditionVariable InputsAvailable;

  //------------------------------------------------------------------------
  // InputsLock must be held before accessing any of the following members.
  InputQueueType Inputs;

public:
  //------------------------------------------------------------------------
  vtkSharedData()
    : Done(false)
    , ActiveThreadCount(0)
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
    bool val = this->Done;
    this->DoneLock.Unlock();
    return val;
  }

  //------------------------------------------------------------------------
  void PushImageToQueue(vtkImageData*& data, const char* fileName)
  {
    this->InputsLock.Lock();
    {
      vtkSharedData::InputValueType value;
      value.Image = data;
      value.FileName = fileName;
      this->Inputs.push(value);
      data = nullptr;
    }
    this->InputsLock.Unlock();
    this->InputsAvailable.Signal();
  }

  //------------------------------------------------------------------------
  // NOTE: This method may suspend the calling thread until inputs become
  // available.
  void GetNextInputToProcess(vtkSmartPointer<vtkImageData>& image, std::string& fileName)
  {
    this->InputsLock.Lock();
    do
    {
      // Check if we have an input available, if so, return it.
      if (!this->Inputs.empty())
      {
        InputValueType& input = this->Inputs.front();
        image = input.Image;
        input.Image = nullptr;
        fileName = input.FileName;
        this->Inputs.pop();
      }

      if (image == nullptr && !this->IsDone())
      {
        // No data is available, let's wait till it becomes available.
        this->InputsAvailable.Wait(this->InputsLock);
      }
    } while (image == nullptr && !this->IsDone());

    this->InputsLock.Unlock();
  }
};

VTK_THREAD_RETURN_TYPE Worker(void* calldata)
{
  vtkMultiThreader::ThreadInfo* info = reinterpret_cast<vtkMultiThreader::ThreadInfo*>(calldata);
  vtkSharedData* sharedData = reinterpret_cast<vtkSharedData*>(info->UserData);

  sharedData->BeginWorker();

  while (true)
  {
    vtkSmartPointer<vtkImageData> image;
    std::string fileName = "";

    sharedData->GetNextInputToProcess(image, fileName);

    if (image == nullptr)
    {
      // end thread.
      break;
    }

    std::size_t pos = fileName.rfind(".");
    std::string extension = fileName.substr(pos + 1);

    if (extension == "Z")
    {
      vtkNew<vtkZLibDataCompressor> zLib;
      float* zBuf = static_cast<vtkFloatArray*>(image->GetPointData()->GetScalars())->GetPointer(0);
      size_t bufSize = image->GetNumberOfPoints() * sizeof(float);
      unsigned char* cBuffer = new unsigned char[bufSize];
      size_t compressSize = zLib->Compress((unsigned char*)zBuf, bufSize, cBuffer, bufSize);
      ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
      fileHandler.write((const char*)cBuffer, compressSize);
      delete[] cBuffer;
    }

    else if (extension == "png")
    {
      vtkNew<vtkPNGWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else if (extension == "jpg" || extension == "jpeg")
    {
      vtkNew<vtkJPEGWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else if (extension == "bmp")
    {
      vtkNew<vtkBMPWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else if (extension == "ppm")
    {
      vtkNew<vtkPNMWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else if (extension == "tif" || extension == "tiff")
    {
      vtkNew<vtkTIFFWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else if (extension == "vti")
    {
      vtkNew<vtkXMLImageDataWriter> writer;
      writer->SetFileName(fileName.c_str());
      writer->SetInputData(image);
      writer->Write();
    }

    else
    {
      vtkDataArray* scalars = image->GetPointData()->GetScalars();
      int scalarSize = scalars->GetDataTypeSize();
      const char* scalarPtr = static_cast<const char*>(scalars->GetVoidPointer(0));
      size_t numberOfScalars = image->GetNumberOfPoints();
      ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
      fileHandler.write(scalarPtr, numberOfScalars * scalarSize);
    }
  }
  sharedData->EndWorker();
  return VTK_THREAD_RETURN_VALUE;
}
}

//****************************************************************************
class vtkThreadedImageWriter::vtkInternals
{
private:
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkUnsignedCharArray> > ClonedOutputs;
  std::vector<int> RunningThreadIds;

public:
  vtkNew<vtkMultiThreader> Threader;
  vtkSharedData SharedData;
  vtkTypeUInt64 Counter;

  vtkSmartPointer<vtkUnsignedCharArray> lastBase64Image;

  vtkInternals()
    : Counter(0)
  {
    lastBase64Image = vtkSmartPointer<vtkUnsignedCharArray>::New();
  }

  void TerminateAllWorkers()
  {
    // request and wait for all threads to close.
    if (!this->RunningThreadIds.empty())
    {
      this->SharedData.RequestAndWaitForWorkersToEnd();
    }

    // Stop threads
    while (!this->RunningThreadIds.empty())
    {
      this->Threader->TerminateThread(this->RunningThreadIds.back());
      this->RunningThreadIds.pop_back();
    }
  }

  void SpawnWorkers(vtkTypeUInt32 numberOfThreads)
  {
    for (vtkTypeUInt32 cc = 0; cc < numberOfThreads; cc++)
    {
      this->RunningThreadIds.push_back(this->Threader->SpawnThread(&Worker, &this->SharedData));
    }
  }
};

vtkStandardNewMacro(vtkThreadedImageWriter);
//----------------------------------------------------------------------------
vtkThreadedImageWriter::vtkThreadedImageWriter()
  : Internals(new vtkInternals())
{
  this->MaxThreads = MAX_NUMBER_OF_THREADS_IN_POOL;
}

//----------------------------------------------------------------------------
vtkThreadedImageWriter::~vtkThreadedImageWriter()
{
  this->Internals->TerminateAllWorkers();
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::SetMaxThreads(vtkTypeUInt32 maxThreads)
{
  if (maxThreads < MAX_NUMBER_OF_THREADS_IN_POOL && maxThreads > 0)
  {
    this->MaxThreads = maxThreads;
  }
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::Initialize()
{
  // Stop any started thread first
  this->Internals->TerminateAllWorkers();

  // Make sure we don't keep adding new threads
  // this->Internals->TerminateAllWorkers();
  // Register new worker threads
  this->Internals->SpawnWorkers(this->MaxThreads);
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::EncodeAndWrite(vtkImageData* image,
                                            const char* fileName)
{
  // Error checking
  if (image == nullptr)
  {
    vtkErrorMacro(<< "Write:Please specify an input!");
    return;
  }

  this->PushImageToQueue(image, fileName);
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::PushImageToQueue(vtkImageData*& data,
                                              const char* fileName)
{
  this->Internals->SharedData.PushImageToQueue(data, fileName);
  assert(data == nullptr);
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkThreadedImageWriter::Finalize()
{
  this->Internals->TerminateAllWorkers();
}
