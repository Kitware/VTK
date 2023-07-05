// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOutputWindow.h"
#if defined(_WIN32) && !defined(VTK_USE_X)
#include "vtkWin32OutputWindow.h"
#endif
#if defined(__ANDROID__) || defined(ANDROID)
#include "vtkAndroidOutputWindow.h"
#endif

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <mutex>
#include <sstream>
#include <thread>

namespace
{
// helps in set and restore value when an instance goes in
// and out of scope respectively.
template <class T>
class vtkScopedSet
{
  std::atomic<T>* Ptr;
  T OldVal;

public:
  vtkScopedSet(std::atomic<T>* ptr, const T& newval)
    : Ptr(ptr)
    , OldVal(*ptr)
  {
    *this->Ptr = newval;
  }
  ~vtkScopedSet() { *this->Ptr = this->OldVal; }
};
}

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
std::mutex InstanceLock; // XXX(c++17): use a `shared_mutex`
vtkSmartPointer<vtkOutputWindow> vtkOutputWindowGlobalInstance;

// helps accessing private members in vtkOutputWindow.
class vtkOutputWindowPrivateAccessor
{
  vtkOutputWindow* Instance;

public:
  vtkOutputWindowPrivateAccessor(vtkOutputWindow* self)
    : Instance(self)
  {
    ++self->InStandardMacros;
  }
  ~vtkOutputWindowPrivateAccessor() { --(this->Instance->InStandardMacros); }
};

void vtkOutputWindowDisplayText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayText(message);
}

void vtkOutputWindowDisplayErrorText(const char* message)
{
  vtkLogF(ERROR, "%s", message);
  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayErrorText(message);
  }
}

void vtkOutputWindowDisplayWarningText(const char* message)
{
  vtkLogF(WARNING, "%s", message);
  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayWarningText(message);
  }
}

void vtkOutputWindowDisplayGenericWarningText(const char* message)
{
  vtkLogF(WARNING, "%s", message);
  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayGenericWarningText(message);
  }
}

void vtkOutputWindowDisplayDebugText(const char* message)
{
  vtkLogF(INFO, "%s", message);
  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayDebugText(message);
  }
}

void vtkOutputWindowDisplayErrorText(
  const char* fname, int lineno, const char* message, vtkObject* sourceObj)
{
  std::ostringstream vtkmsg;
  vtkmsg << "ERROR: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
  if (sourceObj && sourceObj->HasObserver(vtkCommand::ErrorEvent))
  {
    sourceObj->InvokeEvent(vtkCommand::ErrorEvent, const_cast<char*>(vtkmsg.str().c_str()));
  }
  else if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkLogger::Log(vtkLogger::VERBOSITY_ERROR, fname, lineno, message);
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayErrorText(vtkmsg.str().c_str());
  }
}

void vtkOutputWindowDisplayWarningText(
  const char* fname, int lineno, const char* message, vtkObject* sourceObj)
{
  std::ostringstream vtkmsg;
  vtkmsg << "Warning: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
  if (sourceObj && sourceObj->HasObserver(vtkCommand::WarningEvent))
  {
    sourceObj->InvokeEvent(vtkCommand::WarningEvent, const_cast<char*>(vtkmsg.str().c_str()));
  }
  else if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkLogger::Log(vtkLogger::VERBOSITY_WARNING, fname, lineno, message);
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayWarningText(vtkmsg.str().c_str());
  }
}

void vtkOutputWindowDisplayGenericWarningText(const char* fname, int lineno, const char* message)
{
  vtkLogger::Log(vtkLogger::VERBOSITY_WARNING, fname, lineno, message);

  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    std::ostringstream vtkmsg;
    vtkmsg << "Generic Warning: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
    win->DisplayGenericWarningText(vtkmsg.str().c_str());
  }
}

void vtkOutputWindowDisplayDebugText(
  const char* fname, int lineno, const char* message, vtkObject* vtkNotUsed(sourceObj))
{
  vtkLogger::Log(vtkLogger::VERBOSITY_INFO, fname, lineno, message);

  if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    std::ostringstream vtkmsg;
    vtkmsg << "Debug: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
    win->DisplayDebugText(vtkmsg.str().c_str());
  }
}

vtkObjectFactoryNewMacro(vtkOutputWindow);
vtkOutputWindow::vtkOutputWindow()
{
  this->PromptUser = false;
  this->CurrentMessageType = MESSAGE_TYPE_TEXT;
  this->DisplayMode = vtkOutputWindow::DEFAULT;
  this->InStandardMacros = 0;
}

vtkOutputWindow::~vtkOutputWindow() = default;

void vtkOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "vtkOutputWindow Single instance = " << (void*)vtkOutputWindowGlobalInstance
     << endl;
  os << indent << "Prompt User: " << (this->PromptUser ? "On\n" : "Off\n");
  os << indent << "DisplayMode: ";
  switch (this->DisplayMode)
  {
    case DEFAULT:
      os << "Default\n";
      break;
    case NEVER:
      os << "Never\n";
      break;
    case ALWAYS:
      os << "Always\n";
      break;
    case ALWAYS_STDERR:
      os << "AlwaysStderr\n";
      break;
  }
}

vtkOutputWindow::StreamType vtkOutputWindow::GetDisplayStream(MessageTypes msgType) const
{
  switch (this->DisplayMode)
  {
    case DEFAULT:
      if (this->InStandardMacros && vtkLogger::IsEnabled())
      {
        return StreamType::Null;
      }
      VTK_FALLTHROUGH;

    case ALWAYS:
      switch (msgType)
      {
        case MESSAGE_TYPE_TEXT:
          return StreamType::StdOutput;

        default:
          return StreamType::StdError;
      }

    case ALWAYS_STDERR:
      return StreamType::StdError;

    case NEVER:
    default:
      return StreamType::Null;
  }
}

// default implementation outputs to cerr only
void vtkOutputWindow::DisplayText(const char* txt)
{
  // pick correct output channel to dump text on.
  const auto stream_type = this->GetDisplayStream(this->CurrentMessageType);
  switch (stream_type)
  {
    case StreamType::StdOutput:
      cout << txt;
      break;
    case StreamType::StdError:
      cerr << txt;
      break;
    case StreamType::Null:
      break;
  }

  if (this->PromptUser && this->CurrentMessageType != MESSAGE_TYPE_TEXT &&
    stream_type != StreamType::Null)
  {
    char c = 'n';
    cerr << "\nDo you want to suppress any further messages (y,n,q)?." << endl;
    cin >> c;
    if (c == 'y')
    {
      vtkObject::GlobalWarningDisplayOff();
    }
    if (c == 'q')
    {
      this->PromptUser = false;
    }
  }

  this->InvokeEvent(vtkCommand::MessageEvent, const_cast<char*>(txt));
  if (this->CurrentMessageType == MESSAGE_TYPE_TEXT)
  {
    this->InvokeEvent(vtkCommand::TextEvent, const_cast<char*>(txt));
  }
}

void vtkOutputWindow::DisplayErrorText(const char* txt)
{
  vtkScopedSet<MessageTypes> setter(&this->CurrentMessageType, MESSAGE_TYPE_ERROR);

  this->DisplayText(txt);
  this->InvokeEvent(vtkCommand::ErrorEvent, const_cast<char*>(txt));
}

void vtkOutputWindow::DisplayWarningText(const char* txt)
{
  vtkScopedSet<MessageTypes> setter(&this->CurrentMessageType, MESSAGE_TYPE_WARNING);

  this->DisplayText(txt);
  this->InvokeEvent(vtkCommand::WarningEvent, const_cast<char*>(txt));
}

void vtkOutputWindow::DisplayGenericWarningText(const char* txt)
{
  vtkScopedSet<MessageTypes> setter(&this->CurrentMessageType, MESSAGE_TYPE_GENERIC_WARNING);

  this->DisplayText(txt);
  this->InvokeEvent(vtkCommand::WarningEvent, const_cast<char*>(txt));
}

void vtkOutputWindow::DisplayDebugText(const char* txt)
{
  vtkScopedSet<MessageTypes> setter(&this->CurrentMessageType, MESSAGE_TYPE_DEBUG);

  this->DisplayText(txt);
}

// Return the single instance of the vtkOutputWindow
vtkOutputWindow* vtkOutputWindow::GetInstance()
{
  // Check if we have an instance already.
  {
    std::unique_lock<std::mutex> lock(InstanceLock);
    // std::shared_lock lock(InstanceLock); // XXX(c++17)
    (void)lock;

    if (vtkOutputWindowGlobalInstance)
    {
      return vtkOutputWindowGlobalInstance;
    }
  }

  {
    std::unique_lock<std::mutex> lock(InstanceLock);
    (void)lock;

    // Another thread may have raced us here; if it already exists, use it.
    if (vtkOutputWindowGlobalInstance)
    {
      return vtkOutputWindowGlobalInstance;
    }

    // Try the factory first
    vtkOutputWindowGlobalInstance.TakeReference(
      (vtkOutputWindow*)vtkObjectFactory::CreateInstance("vtkOutputWindow"));
    // if the factory did not provide one, then create it here
    if (!vtkOutputWindowGlobalInstance)
    {
#if defined(_WIN32) && !defined(VTK_USE_X)
      vtkOutputWindowGlobalInstance.TakeReference(vtkWin32OutputWindow::New());
#elif defined(ANDROID)
      vtkOutputWindowGlobalInstance.TakeReference(vtkAndroidOutputWindow::New());
#else
      vtkOutputWindowGlobalInstance.TakeReference(vtkOutputWindow::New());
#endif
    }
  }

  // return the instance
  return vtkOutputWindowGlobalInstance;
}

void vtkOutputWindow::SetInstance(vtkOutputWindow* instance)
{
  std::unique_lock<std::mutex> lock(InstanceLock);
  (void)lock;

  if (vtkOutputWindowGlobalInstance == instance)
  {
    return;
  }

  vtkOutputWindowGlobalInstance = vtk::MakeSmartPointer(instance);
}
VTK_ABI_NAMESPACE_END
