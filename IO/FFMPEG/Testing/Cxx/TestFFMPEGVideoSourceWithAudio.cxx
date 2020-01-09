/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFFMPEGVideoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"

#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"

#include "vtkLookupTable.h"

#include "vtkFFMPEGVideoSource.h"

// An example of decoding and playing audio
// Note that the tractor test video has no audio so
// not the best example :-)
#ifdef _WIN32

#include "vtkWindows.h"
#include <xaudio2.h>

namespace
{
template <typename dtype>
void copyRealData(BYTE* dest, dtype** inp, int numChannels, int numSamples, bool packed)
{
  float* fdest = reinterpret_cast<float*>(dest);
  if (packed)
  {
    dtype* iptr = inp[0];
    for (int i = 0; i < numChannels * numSamples; ++i)
    {
      fdest[i] = iptr[i];
    }
  }
  else
  {
    for (int c = 0; c < numChannels; ++c)
    {
      float* cdest = fdest + c;
      dtype* iptr = inp[c];
      for (int i = 0; i < numSamples; ++i)
      {
        *cdest = iptr[i];
        cdest += numChannels;
      }
    }
  }
}
}

struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
  HANDLE hBufferEndEvent;
  StreamingVoiceContext()
    : hBufferEndEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr))
  {
  }
  ~StreamingVoiceContext() { CloseHandle(hBufferEndEvent); }
  void OnBufferEnd(void*) { SetEvent(hBufferEndEvent); }

  // Unused methods are stubs
  void OnVoiceProcessingPassEnd() {}
  void OnVoiceProcessingPassStart(UINT32) {}
  void OnBufferStart(void*) {}
  void OnLoopEnd(void*) {}
  void OnVoiceError(void*, HRESULT) {}
  void OnStreamEnd() {}
};

void setupAudioPlayback(vtkFFMPEGVideoSource* video)
{
  IXAudio2* pXAudio2 = nullptr;
  HRESULT hr;
  if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
  {
    return;
  }

  IXAudio2MasteringVoice* pMasterVoice = nullptr;
  if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)))
  {
    return;
  }

  // XAUDIO2_DEBUG_CONFIGURATION debugc;
  // debugc.TraceMask = XAUDIO2_LOG_ERRORS;
  // debugc.BreakMask = XAUDIO2_LOG_ERRORS;
  // pXAudio2->SetDebugConfiguration(&debugc, nullptr);

  auto cbfunc = [pXAudio2](vtkFFMPEGVideoSourceAudioCallbackData& acbd) {
    HRESULT hr;
    static IXAudio2SourceVoice* pSourceVoice = nullptr;
    static int currentBufferIndex = 0;
    static StreamingVoiceContext aContext;
    const int STREAMING_BUFFER_SIZE = 400000; // try about 48000 * 2 channels * 4 bytes
    static BYTE audioBuffer[STREAMING_BUFFER_SIZE];
    static int maxBufferCount = 0;
    static int maxBufferSize = 0;

    int destBytesPerSample = 2;
    if (acbd.DataType == VTK_FLOAT || acbd.DataType == VTK_DOUBLE)
    {
      destBytesPerSample = 4;
    }

    // do we need to setup the voice?
    if (!pSourceVoice)
    {
      // always setup for 16 bit PCM
      WAVEFORMATEXTENSIBLE wfx = { 0 };
      if (acbd.DataType == VTK_FLOAT || acbd.DataType == VTK_DOUBLE)
      {
        wfx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
      }
      else
      {
        wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
      }
      wfx.Format.nChannels = acbd.NumberOfChannels;
      wfx.Format.nSamplesPerSec = acbd.SampleRate;
      wfx.Format.wBitsPerSample = 8 * destBytesPerSample;
      wfx.Format.nBlockAlign = acbd.NumberOfChannels * destBytesPerSample;
      wfx.Format.nAvgBytesPerSec = acbd.SampleRate * wfx.Format.nBlockAlign;
      wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
      wfx.Samples.wSamplesPerBlock = acbd.NumberOfSamples;
      if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx, 0,
                   XAUDIO2_DEFAULT_FREQ_RATIO, &aContext, nullptr, nullptr)))
      {
        return;
      }
      if (STREAMING_BUFFER_SIZE < wfx.Format.nBlockAlign * acbd.NumberOfSamples)
      {
        cerr << "buffer too small for audio data\n";
      }

      maxBufferSize = wfx.Format.nBlockAlign * acbd.NumberOfSamples;
      maxBufferCount = STREAMING_BUFFER_SIZE / maxBufferSize;

      hr = pSourceVoice->Start(0, 0);
    }

    if (maxBufferSize < destBytesPerSample * acbd.NumberOfSamples * acbd.NumberOfChannels)
    {
      cerr << "buffer too small for new audio data\n";
    }

    XAUDIO2_VOICE_STATE state;
    while (
      pSourceVoice->GetState(&state), static_cast<int>(state.BuffersQueued) >= maxBufferCount - 1)
    {
      cerr << "audio blocked waiting\n";
      WaitForSingleObject(aContext.hBufferEndEvent, INFINITE);
    }

    // copy the incoming data to a buffer
    BYTE* dptr = audioBuffer + maxBufferSize * currentBufferIndex;
    // no copy needed
    if (acbd.DataType == VTK_SHORT && acbd.Packed)
    {
      dptr = reinterpret_cast<BYTE*>(acbd.Data[0]);
    }
    if (acbd.DataType == VTK_FLOAT && acbd.Packed)
    {
      dptr = reinterpret_cast<BYTE*>(acbd.Data[0]);
    }
    if (acbd.DataType == VTK_FLOAT && !acbd.Packed)
    {
      copyRealData(dptr, reinterpret_cast<float**>(acbd.Data), acbd.NumberOfChannels,
        acbd.NumberOfSamples, acbd.Packed);
    }
    if (acbd.DataType == VTK_DOUBLE)
    {
      copyRealData(dptr, reinterpret_cast<double**>(acbd.Data), acbd.NumberOfChannels,
        acbd.NumberOfSamples, acbd.Packed);
    }

    XAUDIO2_BUFFER buf = { 0 };
    buf.AudioBytes = acbd.NumberOfSamples * destBytesPerSample * acbd.NumberOfChannels;
    buf.pAudioData = reinterpret_cast<BYTE*>(dptr);
    if (acbd.Caller->GetEndOfFile())
    {
      buf.Flags = XAUDIO2_END_OF_STREAM;
    }
    pSourceVoice->SubmitSourceBuffer(&buf);
    currentBufferIndex++;
    if (currentBufferIndex == maxBufferCount)
    {
      currentBufferIndex = 0;
    }

    // flush last buffers?
    // XAUDIO2_VOICE_STATE state;
    // while( pSourceVoice->GetState( &state ), state.BuffersQueued > 0 )
    // {
    //   WaitForSingleObjectEx( aContext.hBufferEndEvent, INFINITE, TRUE );
    // }
  };

  video->SetAudioCallback(cbfunc, nullptr);
}
#else
void setupAudioPlayback(vtkFFMPEGVideoSource*) {}
#endif

int TestFFMPEGVideoSourceWithAudio(int argc, char* argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer->SetBackground(0.2, 0.3, 0.4);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 450);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tracktor.webm");

  vtkNew<vtkFFMPEGVideoSource> video;
  video->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(video->GetOutputPort());
  actor->SetTexture(texture);

  vtkNew<vtkPlaneSource> plane;
  mapper->SetInputConnection(plane->GetOutputPort());
  actor->SetMapper(mapper);

  video->Initialize();
  int fsize[3];
  video->GetFrameSize(fsize);
  plane->SetOrigin(0, 0, 0);
  plane->SetPoint1(fsize[0], 0, 0);
  plane->SetPoint2(0, fsize[1], 0);
  renderWindow->Render();
  renderer->GetActiveCamera()->Zoom(2.0);

  setupAudioPlayback(video);
  video->SetDecodingThreads(4);
  video->Record();
  while (!video->GetEndOfFile())
  {
    renderWindow->Render();
  }

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
