/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutputWindow.h"
#ifdef _WIN32
#include "vtkWin32OutputWindow.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkDebugLeaks.h"

vtkCxxRevisionMacro(vtkOutputWindow, "1.25");

vtkOutputWindow* vtkOutputWindow::Instance = 0;
vtkOutputWindowSmartPointer vtkOutputWindow::SmartPointer(NULL);


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

vtkOutputWindowSmartPointer::~vtkOutputWindowSmartPointer()
{
  if (Pointer)
    {
    Pointer->Delete();
    }
}

vtkOutputWindow::vtkOutputWindow()
{
  this->PromptUser = 0;
}

vtkOutputWindow::~vtkOutputWindow()
{
  vtkOutputWindow::Instance = 0;
}

void vtkOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

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
}

void vtkOutputWindow::DisplayErrorText(const char* txt)
{
  this->DisplayText(txt);
}

void vtkOutputWindow::DisplayWarningText(const char* txt)
{
  this->DisplayText(txt);
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
      // if the factory failed to create the object,
      // then destroy it now, as vtkDebugLeaks::ConstructClass was called
      // with vtkclassname, and not the real name of the class
#ifdef _WIN32    
#ifdef VTK_DEBUG_LEAKS
      vtkDebugLeaks::DestructClass("vtkOutputWindow");
#endif
      vtkOutputWindow::Instance = vtkWin32OutputWindow::New();
#else
      vtkOutputWindow::Instance = new vtkOutputWindow;
#endif
      }
    // set the smart pointer to the instance, so
    // it will be UnRegister'ed at exit of the program
    vtkOutputWindow::SmartPointer.SetPointer(vtkOutputWindow::Instance );
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
  vtkOutputWindow::SmartPointer.SetPointer( instance );
  // preferably this will be NULL
  if (vtkOutputWindow::Instance)
    {
    vtkOutputWindow::Instance->Delete();;
    }
  vtkOutputWindow::Instance = instance;
  if (!instance)
    {
    return;
    }
  // Should be safe to send a message now as instance is set
  if (instance->GetReferenceCount()!=1)
    {
    vtkGenericWarningMacro(<<"OutputWindow should have reference count = 1");
    }
  // user will call ->Delete() after setting instance
  instance->Register(NULL);
}


