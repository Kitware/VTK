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
#if defined( _WIN32 ) && !defined( VTK_USE_X )
#include "vtkWin32OutputWindow.h"
#endif
#if defined (ANDROID)
#include "vtkAndroidOutputWindow.h"
#endif
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkOutputWindow* vtkOutputWindow::Instance = 0;
static unsigned int vtkOutputWindowCleanupCounter = 0;

void vtkOutputWindowDisplayText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayText(message);
}

void vtkOutputWindowDisplayErrorText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayErrorText(message);
}

void vtkOutputWindowDisplayWarningText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayWarningText(message);
}

void vtkOutputWindowDisplayGenericWarningText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayGenericWarningText(message);
}

void vtkOutputWindowDisplayDebugText(const char* message)
{
  vtkOutputWindow::GetInstance()->DisplayDebugText(message);
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
    vtkOutputWindow::SetInstance(0);
  }
}

vtkOutputWindow::vtkOutputWindow()
{
  this->PromptUser = 0;
}

vtkOutputWindow::~vtkOutputWindow()
{
}

void vtkOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "vtkOutputWindow Single instance = "
     << (void*)vtkOutputWindow::Instance << endl;
  os << indent << "Prompt User: "
     << (this->PromptUser ? "On\n" : "Off\n");
}


// default implementation outputs to cerr only
void vtkOutputWindow::DisplayText(const char* txt)
{
  cerr << txt;
  if (this->PromptUser)
  {
    char c = 'n';
    cerr << "\nDo you want to suppress any further messages (y,n,q)?."
              << endl;
    cin >> c;
    if (c == 'y')
    {
      vtkObject::GlobalWarningDisplayOff();
    }
    if(c == 'q')
    {
      this->PromptUser = 0;
    }
  }
  this->InvokeEvent(vtkCommand::MessageEvent, (void*)txt);
}

void vtkOutputWindow::DisplayErrorText(const char* txt)
{
  this->DisplayText(txt);
  this->InvokeEvent(vtkCommand::ErrorEvent, (void*)txt);
}

void vtkOutputWindow::DisplayWarningText(const char* txt)
{
  this->DisplayText(txt);
  this->InvokeEvent(vtkCommand::WarningEvent,(void*) txt);
}

void vtkOutputWindow::DisplayGenericWarningText(const char* txt)
{
  this->DisplayText(txt);
}

void vtkOutputWindow::DisplayDebugText(const char* txt)
{
  this->DisplayText(txt);
}

// Up the reference count so it behaves like New
vtkOutputWindow* vtkOutputWindow::New()
{
  vtkOutputWindow* ret = vtkOutputWindow::GetInstance();
  ret->Register(NULL);
  return ret;
}


// Return the single instance of the vtkOutputWindow
vtkOutputWindow* vtkOutputWindow::GetInstance()
{
  if(!vtkOutputWindow::Instance)
  {
    // Try the factory first
    vtkOutputWindow::Instance = (vtkOutputWindow*)
      vtkObjectFactory::CreateInstance("vtkOutputWindow");
    // if the factory did not provide one, then create it here
    if(!vtkOutputWindow::Instance)
    {
#if defined( _WIN32 ) && !defined( VTK_USE_X )
      vtkOutputWindow::Instance = vtkWin32OutputWindow::New();
#else
#if defined( ANDROID )
      vtkOutputWindow::Instance = vtkAndroidOutputWindow::New();
#else
      vtkOutputWindow::Instance = new vtkOutputWindow;
      vtkOutputWindow::Instance->InitializeObjectBase();
#endif
#endif
    }
  }
  // return the instance
  return vtkOutputWindow::Instance;
}

void vtkOutputWindow::SetInstance(vtkOutputWindow* instance)
{
  if (vtkOutputWindow::Instance==instance)
  {
    return;
  }
  // preferably this will be NULL
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
  instance->Register(NULL);
}


