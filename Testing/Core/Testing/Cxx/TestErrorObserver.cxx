// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArchiver.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkRandomPool.h"
#include "vtkTestErrorObserver.h"

int TestErrorCheck()
{
  vtkNew<vtkTest::ErrorObserver> observer;
  vtkNew<vtkArchiver> archiver;
  archiver->AddObserver(vtkCommand::ErrorEvent, observer);
  archiver->OpenArchive();
  return observer->CheckErrorMessage("Please specify ArchiveName to use");
}

int TestWarningCheck()
{
  vtkNew<vtkTest::ErrorObserver> observer;
  vtkNew<vtkRandomPool> pool;
  pool->AddObserver(vtkCommand::WarningEvent, observer);
  pool->PopulateDataArray(nullptr, 0., 1.);
  return observer->CheckWarningMessage("Bad request");
}

int TestErrorObserver(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int ret = 0;
  ret |= TestErrorCheck();
  ret |= TestWarningCheck();
  return ret;
}
