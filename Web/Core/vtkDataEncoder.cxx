// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataEncoder.h"

#include "vtkBase64Utilities.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>
#include <cmath>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <vtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 32

namespace detail
{
VTK_ABI_NAMESPACE_BEGIN

struct vtkWork
{
  vtkSmartPointer<vtkImageData> Image;
  int Quality = 0;
  int Encoding = 0;
  vtkTypeUInt64 TimeStamp = 0;
  vtkTypeUInt32 Key = 0;

  vtkWork() = default;
  vtkWork(vtkTypeUInt32 key, vtkImageData* image, int quality, int encoding)
    : Image(image)
    , Quality(quality)
    , Encoding(encoding)
    , TimeStamp(0)
    , Key(key)
  {
  }
  vtkWork(const vtkWork&) = default;
  vtkWork& operator=(const vtkWork&) = default;
};

class vtkWorkQueue
{
  mutable std::mutex ResultsMutex;
  std::map<vtkTypeUInt32, std::pair<vtkTypeUInt64, vtkSmartPointer<vtkUnsignedCharArray>>> Results;
  std::condition_variable ResultsCondition;

  std::map<vtkTypeUInt32, std::atomic<vtkTypeUInt32>> LastTimeStamp;

  std::mutex QueueMutex;
  std::queue<vtkWork> Queue;
  std::condition_variable QueueCondition;

  std::vector<std::thread> ThreadPool;
  std::atomic<bool> Terminate;

  static void DoWork(int threadIndex, vtkWorkQueue* self)
  {
    vtkLogger::SetThreadName("Worker " + std::to_string(threadIndex));
    vtkLogF(TRACE, "starting worker thread");
    vtkNew<vtkJPEGWriter> writer;
    writer->WriteToMemoryOn();
    while (!self->Terminate)
    {
      vtkWork work;
      {
        std::unique_lock<std::mutex> lock(self->QueueMutex);
        bool break_loop = false;
        do
        {
          self->QueueCondition.wait_for(lock, std::chrono::seconds(1),
            [self]() { return !self->Queue.empty() || self->Terminate; });
          if (self->Terminate)
          {
            break_loop = true;
            break;
          }
        } while (self->Queue.empty());
        if (break_loop)
        {
          break;
        }
        work = self->Queue.front();
        self->Queue.pop();
      }

      writer->SetInputData(work.Image);
      writer->SetQuality(work.Quality);
      writer->Write();

      auto result = vtkSmartPointer<vtkUnsignedCharArray>::New();
      if (work.Encoding)
      {
        vtkUnsignedCharArray* data = writer->GetResult();
        result->SetNumberOfComponents(1);
        result->SetNumberOfTuples(std::ceil(1.5 * data->GetNumberOfTuples()));
        unsigned long size = vtkBase64Utilities::Encode(
          data->GetPointer(0), data->GetNumberOfTuples(), result->GetPointer(0), /*mark_end=*/0);
        result->SetNumberOfTuples(static_cast<vtkIdType>(size) + 1);
        result->SetValue(size, 0);
      }
      else
      {
        // We must do a deep copy here as the writer reuse that array
        // and will change its values concurrently during its next job...
        result->DeepCopy(writer->GetResult());
      }
      writer->SetInputData(nullptr);

      {
        std::unique_lock<std::mutex> lock(self->ResultsMutex);
        auto& pair = self->Results[work.Key];
        if (pair.first < work.TimeStamp)
        {
          pair = std::make_pair(work.TimeStamp, result);
          lock.unlock();
          self->ResultsCondition.notify_all();
        }
      }
    }

    vtkLogF(TRACE, "exiting worker thread");
  }

public:
  vtkWorkQueue(int numThreads)
    : Terminate(false)
  {
    assert(numThreads >= 0);
    for (int cc = 0; cc < numThreads; ++cc)
    {
      this->ThreadPool.emplace_back(&vtkWorkQueue::DoWork, cc, this);
    }
  }
  ~vtkWorkQueue()
  {
    this->Terminate = true;
    this->QueueCondition.notify_all();
    for (auto& thread : this->ThreadPool)
    {
      thread.join();
    }
  }

  bool IsValid() const { return !this->ThreadPool.empty(); }

  void PushBack(vtkWork&& work)
  {
    if (!this->IsValid())
    {
      vtkLogF(ERROR, "Queue is invalid! Can't push work!");
      return;
    }

    auto key = work.Key;
    work.TimeStamp = ++this->LastTimeStamp[key];
    {
      std::unique_lock<std::mutex> lock(this->QueueMutex);
      this->Queue.emplace(std::move(work));
    }
    this->QueueCondition.notify_one();
  }

  bool GetResult(vtkTypeUInt32 key, vtkSmartPointer<vtkUnsignedCharArray>& data) const
  {
    std::unique_lock<std::mutex> lock(this->ResultsMutex);
    auto iter = this->Results.find(key);
    if (iter == this->Results.end())
    {
      return false;
    }

    const auto& resultsPair = iter->second;
    data = resultsPair.second;
    // return true if this is the latest result for this key.
    return (resultsPair.first == this->LastTimeStamp.at(key));
  }

  void Flush(vtkTypeUInt32 key)
  {
    auto tsIter = this->LastTimeStamp.find(key);
    if (tsIter == this->LastTimeStamp.end())
    {
      return;
    }
    const auto& ts = tsIter->second;
    std::unique_lock<std::mutex> lock(this->ResultsMutex);
    this->ResultsCondition.wait(lock, [this, &ts, &key]() {
      try
      {
        return ts == this->Results[key].first;
      }
      catch (std::out_of_range&)
      {
        // result not available yet; keep waiting;
        return false;
      }
    });
  }
};
VTK_ABI_NAMESPACE_END
} // namespace detail

VTK_ABI_NAMESPACE_BEGIN
//****************************************************************************
class vtkDataEncoder::vtkInternals
{
public:
  detail::vtkWorkQueue Queue;
  vtkNew<vtkUnsignedCharArray> LastBase64Image;

  vtkInternals(int numThreads)
    : Queue(numThreads)
  {
  }

  // Once an imagedata has been written to memory as a jpg or png, this
  // convenience function can encode that image as a Base64 string.
  const char* GetBase64EncodedImage(vtkUnsignedCharArray* encodedInputImage)
  {
    this->LastBase64Image->SetNumberOfComponents(1);
    this->LastBase64Image->SetNumberOfTuples(
      std::ceil(1.5 * encodedInputImage->GetNumberOfTuples()));
    unsigned long size = vtkBase64Utilities::Encode(encodedInputImage->GetPointer(0),
      encodedInputImage->GetNumberOfTuples(), this->LastBase64Image->GetPointer(0), /*mark_end=*/0);

    this->LastBase64Image->SetNumberOfTuples(static_cast<vtkIdType>(size) + 1);
    this->LastBase64Image->SetValue(size, 0);

    return reinterpret_cast<char*>(this->LastBase64Image->GetPointer(0));
  }
};

vtkStandardNewMacro(vtkDataEncoder);
//------------------------------------------------------------------------------
vtkDataEncoder::vtkDataEncoder()
  : MaxThreads(3)
  , Internals(new vtkInternals(this->MaxThreads))
{
}

//------------------------------------------------------------------------------
vtkDataEncoder::~vtkDataEncoder() = default;

//------------------------------------------------------------------------------
void vtkDataEncoder::SetMaxThreads(vtkTypeUInt32 maxThreads)
{
  if (maxThreads < MAX_NUMBER_OF_THREADS_IN_POOL && maxThreads > 0)
  {
    this->MaxThreads = maxThreads;
  }
}

//------------------------------------------------------------------------------
void vtkDataEncoder::Initialize()
{
  this->Internals.reset(new vtkDataEncoder::vtkInternals(this->MaxThreads));
}

//------------------------------------------------------------------------------
void vtkDataEncoder::Push(vtkTypeUInt32 key, vtkImageData* data, int quality, int encoding)
{
  auto& internals = (*this->Internals);
  internals.Queue.PushBack(detail::vtkWork(key, data, quality, encoding));
}

//------------------------------------------------------------------------------
bool vtkDataEncoder::GetLatestOutput(vtkTypeUInt32 key, vtkSmartPointer<vtkUnsignedCharArray>& data)
{
  auto& internals = (*this->Internals);
  return internals.Queue.GetResult(key, data);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkDataEncoder::Flush(vtkTypeUInt32 key)
{
  auto& internals = (*this->Internals);
  internals.Queue.Flush(key);
}

//------------------------------------------------------------------------------
void vtkDataEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkDataEncoder::Finalize()
{
  this->Internals.reset(new vtkDataEncoder::vtkInternals(0));
}
VTK_ABI_NAMESPACE_END
