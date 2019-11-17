/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMotionFXCFGReaderCommon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef TestMotionFXCFGReaderCommon_h
#define TestMotionFXCFGReaderCommon_h

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCompositePolyDataMapper2.h>
#include <vtkInformation.h>
#include <vtkMotionFXCFGReader.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace impl
{

struct ClientData
{
  vtkSmartPointer<vtkRenderWindow> Window;
  vtkSmartPointer<vtkMotionFXCFGReader> Reader;
  vtkSmartPointer<vtkCompositePolyDataMapper2> Mapper;
  std::vector<double> TimeSteps;
  int CurrentIndex;

  void GoToNext()
  {
    cout << "Go to next" << endl;
    this->CurrentIndex =
      std::min(static_cast<int>(this->TimeSteps.size()) - 1, this->CurrentIndex + 1);
    this->Render();
  }

  void GoToPrev()
  {
    cout << "Go to prev" << endl;
    this->CurrentIndex = std::max(0, this->CurrentIndex - 1);
    this->Render();
  }

  void Play()
  {
    cout << "Playing";
    for (size_t cc = 0; cc < this->TimeSteps.size(); ++cc)
    {
      cout << ".";
      cout.flush();
      this->CurrentIndex = static_cast<int>(cc);
      this->Render();
    }
    cout << endl;
  }

  void Render()
  {
    assert(
      this->CurrentIndex >= 0 && this->CurrentIndex < static_cast<int>(this->TimeSteps.size()));
    this->Reader->UpdateTimeStep(this->TimeSteps[this->CurrentIndex]);
    this->Mapper->SetInputDataObject(this->Reader->GetOutputDataObject(0));
    this->Window->Render();
  }
};

static void CharEventCallback(vtkObject* caller, unsigned long, void* clientdata, void*)
{
  ClientData& data = *reinterpret_cast<ClientData*>(clientdata);
  auto iren = vtkRenderWindowInteractor::SafeDownCast(caller);
  switch (iren->GetKeyCode())
  {
    case 'x':
    case 'X':
      data.GoToNext();
      break;

    case 'z':
    case 'Z':
      data.GoToPrev();
      break;

    case 'c':
    case 'C':
      data.Play();
      break;
  }
}

template <typename InitializationCallback>
int Test(int argc, char* argv[], const char* dfile, const InitializationCallback& initCallback)
{
  vtkNew<vtkMotionFXCFGReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, dfile);
  reader->SetFileName(fname);
  delete[] fname;

  reader->SetTimeResolution(100);
  reader->UpdateInformation();

  using SDDP = vtkStreamingDemandDrivenPipeline;
  vtkInformation* outInfo = reader->GetOutputInformation(0);
  const int numTimeSteps = outInfo->Length(SDDP::TIME_STEPS());

  if (numTimeSteps != 100)
  {
    cerr << "ERROR: missing timesteps. Potential issue reading the CFG file." << endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkRenderWindow> renWin;

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  initCallback(renWin, renderer, reader);

  std::vector<double> ts(numTimeSteps);
  outInfo->Get(SDDP::TIME_STEPS(), &ts[0]);

  // for baseline comparison, we'll jump to the middle of the
  // time sequence and do a capture.
  reader->UpdateTimeStep(ts[numTimeSteps / 2]);
  mapper->SetInputDataObject(reader->GetOutputDataObject(0));
  renWin->Render();

  const int retVal = vtkTesting::Test(argc, argv, renWin, 10);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    ClientData data;
    data.Window = renWin;
    data.Reader = reader;
    data.Mapper = mapper;
    data.TimeSteps = ts;
    data.CurrentIndex = numTimeSteps / 2;

    vtkNew<vtkCallbackCommand> observer;
    observer->SetClientData(&data);
    observer->SetCallback(&CharEventCallback);
    iren->AddObserver(vtkCommand::CharEvent, observer);

    cout << "Entering interactive mode......" << endl
         << "Supported operations:" << endl
         << "   'z' or 'Z' : go to next time step" << endl
         << "   'x' or 'X' : go to previous time step" << endl
         << "   'c' or 'C' : play animation from start to end" << endl
         << "   'q' or 'Q' : quit" << endl;
    iren->Start();
    return EXIT_SUCCESS;
  }
  else if (retVal == vtkTesting::NOT_RUN)
  {
    return VTK_SKIP_RETURN_CODE;
  }
  else if (retVal == vtkTesting::PASSED)
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
}

#endif
