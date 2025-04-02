// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAndroidOutputWindow.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include <sstream>

#include <android/log.h>

#include <mutex>

static std::mutex vtkAndroidOutputWindowMutex;

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAndroidOutputWindow);

//------------------------------------------------------------------------------
vtkAndroidOutputWindow::vtkAndroidOutputWindow() {}

//------------------------------------------------------------------------------
vtkAndroidOutputWindow::~vtkAndroidOutputWindow() {}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::DisplayErrorText(const char* someText)
{
  if (!someText)
  {
    return;
  }

  std::istringstream stream(someText);
  std::string line;
  while (std::getline(stream, line))
  {
    __android_log_print(ANDROID_LOG_ERROR, "VTK", "%s", line.c_str());
  }
  this->InvokeEvent(vtkCommand::ErrorEvent, (void*)someText);
}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::DisplayWarningText(const char* someText)
{
  std::lock_guard<std::mutex> lock(vtkAndroidOutputWindowMutex);
  if (!someText)
  {
    return;
  }

  std::istringstream stream(someText);
  std::string line;
  while (std::getline(stream, line))
  {
    __android_log_print(ANDROID_LOG_WARN, "VTK", "%s", line.c_str());
  }
  this->InvokeEvent(vtkCommand::WarningEvent, (void*)someText);
}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::DisplayGenericWarningText(const char* someText)
{
  std::lock_guard<std::mutex> lock(vtkAndroidOutputWindowMutex);
  if (!someText)
  {
    return;
  }

  std::istringstream stream(someText);
  std::string line;
  while (std::getline(stream, line))
  {
    __android_log_print(ANDROID_LOG_WARN, "VTK", "%s", line.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::DisplayDebugText(const char* someText)
{
  std::lock_guard<std::mutex> lock(vtkAndroidOutputWindowMutex);
  if (!someText)
  {
    return;
  }

  std::istringstream stream(someText);
  std::string line;
  while (std::getline(stream, line))
  {
    __android_log_print(ANDROID_LOG_DEBUG, "VTK", "%s", line.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::DisplayText(const char* someText)
{
  std::lock_guard<std::mutex> lock(vtkAndroidOutputWindowMutex);
  if (!someText)
  {
    return;
  }

  std::istringstream stream(someText);
  std::string line;
  while (std::getline(stream, line))
  {
    __android_log_print(ANDROID_LOG_INFO, "VTK", "%s", line.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkAndroidOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
