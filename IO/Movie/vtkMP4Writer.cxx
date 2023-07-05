// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMP4Writer.h"
#include "vtkWindows.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkErrorCode.h"

#include <mfidl.h> // must be included first

#include <mfapi.h>
#include <mferror.h>
#include <mfreadwrite.h>

VTK_ABI_NAMESPACE_BEGIN
class vtkMP4Writer::vtkMP4WriterInternals
{
public:
  // Format constants
  UINT32 VideoWidth = 0;
  UINT32 VideoHeight = 0;
  UINT32 VideoFPS = 10;
  UINT64 VideoFrameDuration = 10 * 1000 * 1000 / this->VideoFPS; // 100s of nanoseconds
  UINT32 VideoBitRate = 800000;
  const GUID VideoEncodingFormat = MFVideoFormat_H264;
  const GUID VideoInputFormat = MFVideoFormat_RGB32;

  // Buffer to hold the video frame data.
  DWORD* VideoFrameBuffer = nullptr;

  IMFSinkWriter* SinkWriter = nullptr;
  DWORD Stream = 0;
  LONGLONG TimeStamp = 0;

  template <class T>
  void SafeRelease(T** ppT)
  {
    if (*ppT)
    {
      (*ppT)->Release();
      *ppT = nullptr;
    }
  }

  HRESULT InitializeSinkWriter(IMFSinkWriter** ppWriter, DWORD* pStreamIndex, const char* fileName)
  {
    *ppWriter = nullptr;
    *pStreamIndex = 0;

    IMFSinkWriter* pSinkWriter = nullptr;
    IMFMediaType* pMediaTypeOut = nullptr;
    IMFMediaType* pMediaTypeIn = nullptr;
    DWORD streamIndex = 0;

    IMFAttributes* writerAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&writerAttributes, 10);
    if (SUCCEEDED(hr))
    {
      hr = writerAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);
    }

    if (SUCCEEDED(hr))
    {
      wchar_t* wFileName = new wchar_t[mbstowcs(nullptr, fileName, 32000) + 1];
      mbstowcs(wFileName, fileName, 32000);
      hr = MFCreateSinkWriterFromURL(wFileName, nullptr, writerAttributes, &pSinkWriter);
      SafeRelease(&writerAttributes);
      delete[] wFileName;
    }

    // Set the output media type.
    if (SUCCEEDED(hr))
    {
      hr = MFCreateMediaType(&pMediaTypeOut);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, this->VideoEncodingFormat);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, this->VideoBitRate);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, this->VideoWidth, this->VideoHeight);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, this->VideoFPS, 1);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED(hr))
    {
      hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);
    }

    // Set the input media type.
    if (SUCCEEDED(hr))
    {
      hr = MFCreateMediaType(&pMediaTypeIn);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, this->VideoInputFormat);
    }
    if (SUCCEEDED(hr))
    {
      hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, this->VideoWidth, this->VideoHeight);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, this->VideoFPS, 1);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }
    if (SUCCEEDED(hr))
    {
      hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, nullptr);
    }

    // Tell the sink writer to start accepting data.
    if (SUCCEEDED(hr))
    {
      hr = pSinkWriter->BeginWriting();
    }

    // Return the pointer to the caller.
    if (SUCCEEDED(hr))
    {
      *ppWriter = pSinkWriter;
      (*ppWriter)->AddRef();
      *pStreamIndex = streamIndex;
    }

    SafeRelease(&pSinkWriter);
    SafeRelease(&pMediaTypeOut);
    SafeRelease(&pMediaTypeIn);
    return hr;
  }

  HRESULT WriteFrame(IMFSinkWriter* pWriter, DWORD streamIndex, const LONGLONG& frameTime)
  {
    IMFSample* pSample = nullptr;
    IMFMediaBuffer* pBuffer = nullptr;

    const LONG cbWidth = 4 * this->VideoWidth;
    const DWORD cbBuffer = cbWidth * this->VideoHeight;

    BYTE* pData = nullptr;

    // Create a new memory buffer.
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Lock the buffer and copy the video frame to the buffer.
    if (SUCCEEDED(hr))
    {
      hr = pBuffer->Lock(&pData, nullptr, nullptr);
    }
    if (SUCCEEDED(hr))
    {
      hr = MFCopyImage(pData,          // Destination buffer.
        cbWidth,                       // Destination stride.
        (BYTE*)this->VideoFrameBuffer, // First row in source image.
        cbWidth,                       // Source stride.
        cbWidth,                       // Image width in bytes.
        this->VideoHeight              // Image height in pixels.
      );
    }
    if (pBuffer)
    {
      pBuffer->Unlock();
    }

    // Set the data length of the buffer.
    if (SUCCEEDED(hr))
    {
      hr = pBuffer->SetCurrentLength(cbBuffer);
    }

    // Create a media sample and add the buffer to the sample.
    if (SUCCEEDED(hr))
    {
      hr = MFCreateSample(&pSample);
    }
    if (SUCCEEDED(hr))
    {
      hr = pSample->AddBuffer(pBuffer);
    }

    // Set the time stamp and the duration.
    if (SUCCEEDED(hr))
    {
      hr = pSample->SetSampleTime(frameTime);
    }
    if (SUCCEEDED(hr))
    {
      hr = pSample->SetSampleDuration(this->VideoFrameDuration);
    }

    // Send the sample to the Sink Writer.
    if (SUCCEEDED(hr))
    {
      hr = pWriter->WriteSample(streamIndex, pSample);
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
  }
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMP4Writer);

//------------------------------------------------------------------------------
vtkMP4Writer::vtkMP4Writer()
{
  this->BitRate = 800000;
  this->Rate = 10;

  this->Internals = new vtkMP4WriterInternals();
}

//------------------------------------------------------------------------------
vtkMP4Writer::~vtkMP4Writer()
{
  if (this->Writing)
  {
    this->End();
  }
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkMP4Writer::Start()
{
  if (this->Writing)
  {
    vtkWarningMacro(
      "Start() called while already writing a file. Call End() before writing a new video file.");
    return;
  }

  // Error checking
  this->SetErrorCode(1);
  if (this->GetInput() == nullptr)
  {
    vtkErrorMacro(<< "Write:Please specify an input!");
    this->SetErrorCode(vtkGenericMovieWriter::NoInputError);
    return;
  }

  // Fill in image information.
  this->GetInputAlgorithm(0, 0)->UpdateInformation();
  int wExtent[6];
  this->GetInputInformation(0, 0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExtent);
  this->Internals->VideoWidth = wExtent[1] - wExtent[0] + 1;
  this->Internals->VideoHeight = wExtent[3] - wExtent[2] + 1;
  UINT32 numPixels = this->Internals->VideoWidth * this->Internals->VideoHeight;
  this->Internals->VideoBitRate = this->BitRate;
  this->Internals->VideoFPS = this->Rate;
  this->Internals->VideoFrameDuration = 10 * 1000 * 1000 / this->Rate; // in 100s of nanoseconds
  this->Internals->VideoFrameBuffer = new DWORD[numPixels];

  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr))
  {
    hr = MFStartup(MF_VERSION);
    if (SUCCEEDED(hr))
    {
      hr = this->Internals->InitializeSinkWriter(
        &this->Internals->SinkWriter, &this->Internals->Stream, this->FileName);
      this->SetErrorCode(0);
    }
  }

  if (FAILED(hr))
  {
    this->SetErrorCode(vtkGenericMovieWriter::InitError);
    vtkErrorMacro("Could not initialize writer");
    return;
  }

  this->Writing = true;
}

//------------------------------------------------------------------------------
void vtkMP4Writer::Write()
{
  if (!this->Writing)
  {
    vtkErrorMacro("Start() must be called before calling Write()");
    return;
  }
  if (this->Internals->SinkWriter == nullptr)
  {
    vtkErrorMacro("No writer created for file");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
  }

  // get the data
  vtkImageData* input = this->GetImageDataInput(0);
  this->GetInputAlgorithm(0, 0)->UpdateWholeExtent();

  // get the pointer to the data
  unsigned char* ptr = (unsigned char*)(input->GetScalarPointer());
  unsigned char* dest = (unsigned char*)(this->Internals->VideoFrameBuffer);

  // Set all pixels to green
  for (DWORD j = 0; j < this->Internals->VideoHeight; ++j)
  {
    for (DWORD i = 0; i < this->Internals->VideoWidth; ++i)
    {
      *dest++ = ptr[2];
      *dest++ = ptr[1];
      *dest++ = ptr[0];
      *dest++ = 0;
      ptr += 3;
    }
  }

  HRESULT hr = this->Internals->WriteFrame(
    this->Internals->SinkWriter, this->Internals->Stream, this->Internals->TimeStamp);
  if (FAILED(hr))
  {
    vtkErrorMacro("Failed to write frame to MP4 file");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
  }
  this->Internals->TimeStamp += this->Internals->VideoFrameDuration;
}

//------------------------------------------------------------------------------
void vtkMP4Writer::End()
{
  if (this->Writing)
  {
    if (this->Internals->SinkWriter)
    {
      HRESULT hr = this->Internals->SinkWriter->Finalize();
      if (FAILED(hr))
      {
        vtkErrorMacro("Failed to write MP4 file");
        return;
      }
      this->Internals->SafeRelease(&this->Internals->SinkWriter);
    }
    MFShutdown();
    CoUninitialize();

    delete[] this->Internals->VideoFrameBuffer;
    this->Writing = false;
  }
}

//------------------------------------------------------------------------------
void vtkMP4Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Rate: " << this->Rate << endl;
  os << indent << "BitRate: " << this->BitRate << endl;
}
VTK_ABI_NAMESPACE_END
