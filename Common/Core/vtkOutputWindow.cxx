/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutputWindow.h"
#include "vtkToolkits.h"
#if defined(_WIN32) && !defined(VTK_USE_X)
#include "vtkWin32OutputWindow.h"
#endif
#if defined(ANDROID)
#include "vtkAndroidOutputWindow.h"
#endif

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include <sstream>

namespace
{
// helps in set and restore value when an instance goes in
// and out of scope respectively.
template <class T>
class vtkScopedSet
{
  T* Ptr;
  T OldVal;

public:
  vtkScopedSet(T* ptr, const T& newval)
    : Ptr(ptr)
    , OldVal(*ptr)
  {
    *this->Ptr = newval;
  }
  ~vtkScopedSet() { *this->Ptr = this->OldVal; }
};
}

//----------------------------------------------------------------------------
vtkOutputWindow* vtkOutputWindow::Instance = nullptr;
static unsigned int vtkOutputWindowCleanupCounter = 0;

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
  vtkLogger::Log(vtkLogger::VERBOSITY_ERROR, fname, lineno, message);

  std::ostringstream vtkmsg;
  vtkmsg << "ERROR: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
  if (sourceObj && sourceObj->HasObserver(vtkCommand::ErrorEvent))
  {
    sourceObj->InvokeEvent(vtkCommand::ErrorEvent, const_cast<char*>(vtkmsg.str().c_str()));
  }
  else if (auto win = vtkOutputWindow::GetInstance())
  {
    vtkOutputWindowPrivateAccessor helper_raii(win);
    win->DisplayErrorText(vtkmsg.str().c_str());
  }
}

void vtkOutputWindowDisplayWarningText(
  const char* fname, int lineno, const char* message, vtkObject* sourceObj)
{
  vtkLogger::Log(vtkLogger::VERBOSITY_WARNING, fname, lineno, message);

  std::ostringstream vtkmsg;
  vtkmsg << "Warning: In " << fname << ", line " << lineno << "\n" << message << "\n\n";
  if (sourceObj && sourceObj->HasObserver(vtkCommand::WarningEvent))
  {
    sourceObj->InvokeEvent(vtkCommand::WarningEvent, const_cast<char*>(vtkmsg.str().c_str()));
  }
  else if (auto win = vtkOutputWindow::GetInstance())
  {
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

vtkOutputWindowCleanup::vtkOutputWindowCleanup()
{
  ++vtkOutputWindowCleanupCounter;
}

vtkOutputWindowCleanup::~vtkOutputWindowCleanup()
{
  if (--vtkOutputWindowCleanupCounter == 0)
  {
    // Destroy any remaining output window.
    vtkOutputWindow::SetInstance(nullptr);
  }
}

vtkObjectFactoryNewMacro(vtkOutputWindow);
vtkOutputWindow::vtkOutputWindow()
{
  this->PromptUser = false;
  this->CurrentMessageType = MESSAGE_TYPE_TEXT;
  this->DisplayMode = vtkOutputWindow::DEFAULT;
  this->InStandardMacros = false;
}

vtkOutputWindow::~vtkOutputWindow() = default;

void vtkOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "vtkOutputWindow Single instance = " << (void*)vtkOutputWindow::Instance << endl;
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
      this->PromptUser = 0;
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
  if (!vtkOutputWindow::Instance)
  {
    // Try the factory first
    vtkOutputWindow::Instance =
      (vtkOutputWindow*)vtkObjectFactory::CreateInstance("vtkOutputWindow");
    // if the factory did not provide one, then create it here
    if (!vtkOutputWindow::Instance)
    {
#if defined(_WIN32) && !defined(VTK_USE_X)
      vtkOutputWindow::Instance = vtkWin32OutputWindow::New();
#elif defined(ANDROID)
      vtkOutputWindow::Instance = vtkAndroidOutputWindow::New();
#else
      vtkOutputWindow::Instance = vtkOutputWindow::New();
#endif
    }
  }
  // return the instance
  return vtkOutputWindow::Instance;
}

void vtkOutputWindow::SetInstance(vtkOutputWindow* instance)
{
  if (vtkOutputWindow::Instance == instance)
  {
    return;
  }
  // preferably this will be nullptr
  if (vtkOutputWindow::Instance)
  {
    vtkOutputWindow::Instance->Delete();
  }
  vtkOutputWindow::Instance = instance;
  if (!instance)
  {
    return;
  }
  // user will call ->Delete() after setting instance
  instance->Register(nullptr);
}

#if !defined(VTK_LEGACY_REMOVE)
void vtkOutputWindow::SetUseStdErrorForAllMessages(bool val)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkOutputWindow::SetUseStdErrorForAllMessages, "VTK 9.0", vtkOutputWindow::SetDisplayMode);
  this->SetDisplayMode(val ? ALWAYS_STDERR : DEFAULT);
}

bool vtkOutputWindow::GetUseStdErrorForAllMessages()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkOutputWindow::GetUseStdErrorForAllMessages, "VTK 9.0", vtkOutputWindow::GetDisplayMode);
  return this->DisplayMode == ALWAYS_STDERR;
}

void vtkOutputWindow::UseStdErrorForAllMessagesOn()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkOutputWindow::UseStdErrorForAllMessagesOn, "VTK 9.0", vtkOutputWindow::SetDisplayMode);
  this->SetDisplayMode(ALWAYS_STDERR);
}

void vtkOutputWindow::UseStdErrorForAllMessagesOff()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkOutputWindow::UseStdErrorForAllMessagesOff, "VTK 9.0", vtkOutputWindow::SetDisplayMode);
  this->SetDisplayMode(DEFAULT);
}
#endif
