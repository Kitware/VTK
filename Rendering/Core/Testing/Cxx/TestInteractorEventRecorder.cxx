// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkInteractorEventRecorder.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

namespace
{

constexpr char TestOldRepeatCountWorkAroundLog[] = "# StreamVersion 1.2\n"
                                                   "LeftButtonPressEvent 153 255 0 0 1 0 0\n";

bool TestOldRepeatCountWorkAround()
{
  vtkNew<vtkRenderWindow> win;
  win->OffScreenRenderingOn();
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestOldRepeatCountWorkAroundLog);
  recorder->Play();

  if (iren->GetRepeatCount() != 1)
  {
    vtkLog(ERROR, "Unexpected RepeatCount without work around: " << iren->GetRepeatCount());
    return false;
  }

  recorder->OldRepeatCountWorkAroundOn();
  recorder->Play();

  if (iren->GetRepeatCount() != 0)
  {
    vtkLog(ERROR, "Unexpected RepeatCount with work around: " << iren->GetRepeatCount());
    return false;
  }

  return true;
}
}

int TestInteractorEventRecorder(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool ret = ::TestOldRepeatCountWorkAround();
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
