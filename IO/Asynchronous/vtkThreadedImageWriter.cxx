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
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkJPEGWriter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPNMWriter.h"
#include "vtkPointData.h"
#include "vtkTIFFWriter.h"
#include "vtkThreadedTaskQueue.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkZLibDataCompressor.h"
#include "vtksys/FStream.hxx"

#include <cassert>
#include <cmath>
#include <iostream>

#include <vtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 32
//****************************************************************************
namespace
{
static void EncodeAndWrite(const vtkSmartPointer<vtkImageData>& image, const std::string& fileName)
{
  vtkLogF(TRACE, "encoding: %s", fileName.c_str());
  assert(image != nullptr);

  std::size_t pos = fileName.rfind(".");
  std::string extension = fileName.substr(pos + 1);

  if (extension == "Z")
  {
    vtkNew<vtkZLibDataCompressor> zLib;
    float* zBuf = static_cast<vtkFloatArray*>(image->GetPointData()->GetScalars())->GetPointer(0);
    size_t bufSize = image->GetNumberOfPoints() * sizeof(float);
    unsigned char* cBuffer = new unsigned char[bufSize];
    size_t compressSize = zLib->Compress((unsigned char*)zBuf, bufSize, cBuffer, bufSize);
    vtksys::ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
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
    vtksys::ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
    fileHandler.write(scalarPtr, numberOfScalars * scalarSize);
  }
}
}

//****************************************************************************
class vtkThreadedImageWriter::vtkInternals
{
private:
  using TaskQueueType = vtkThreadedTaskQueue<void, vtkSmartPointer<vtkImageData>, std::string>;
  std::unique_ptr<TaskQueueType> Queue;

public:
  vtkInternals()
    : Queue(nullptr)
  {
  }

  ~vtkInternals() { this->TerminateAllWorkers(); }

  void TerminateAllWorkers()
  {
    if (this->Queue)
    {
      this->Queue->Flush();
    }
    this->Queue.reset(nullptr);
  }

  void SpawnWorkers(vtkTypeUInt32 numberOfThreads)
  {
    this->Queue.reset(new TaskQueueType(::EncodeAndWrite,
      /*strict_ordering=*/true,
      /*buffer_size=*/-1,
      /*max_concurrent_tasks=*/static_cast<int>(numberOfThreads)));
  }

  void PushImageToQueue(vtkSmartPointer<vtkImageData>&& data, std::string&& filename)
  {
    this->Queue->Push(std::move(data), std::move(filename));
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
void vtkThreadedImageWriter::EncodeAndWrite(vtkImageData* image, const char* fileName)
{
  // Error checking
  if (image == nullptr)
  {
    vtkErrorMacro(<< "Write:Please specify an input!");
    return;
  }

  // we make a shallow copy so that the caller doesn't have to take too much
  // care when modifying image besides the standard requirements for the case
  // where the image is propagated in the pipeline.
  vtkSmartPointer<vtkImageData> img;
  img.TakeReference(image->NewInstance());
  img->ShallowCopy(image);
  this->Internals->PushImageToQueue(std::move(img), std::string(fileName));
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
