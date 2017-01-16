/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkOpenGLResourceFreeCallback_h
#define vtkOpenGLResourceFreeCallback_h

// Description:
// Provide a mechanism for making sure graphics resources are
// freed properly.

class vtkOpenGLRenderWindow;
class vtkWindow;

class vtkGenericOpenGLResourceFreeCallback
{
  public:
    vtkGenericOpenGLResourceFreeCallback() {
        this->VTKWindow = NULL; this->Releasing = false; }
    virtual ~vtkGenericOpenGLResourceFreeCallback() { }

    // Called when the event is invoked
    virtual void Release() = 0;

    virtual void RegisterGraphicsResources(vtkOpenGLRenderWindow *rw) =  0;

    bool IsReleasing() {
      return this->Releasing; }

  protected:
    vtkOpenGLRenderWindow *VTKWindow;
    bool Releasing;
};

// Description:
// Templated member callback.
template <class T>
class vtkOpenGLResourceFreeCallback : public vtkGenericOpenGLResourceFreeCallback
{
public:
  vtkOpenGLResourceFreeCallback(T* handler, void (T::*method)(vtkWindow *))
  {
    this->Handler = handler;
    this->Method = method;
  }

  ~vtkOpenGLResourceFreeCallback() VTK_OVERRIDE { }

  void RegisterGraphicsResources(vtkOpenGLRenderWindow *rw) VTK_OVERRIDE {
    if (this->VTKWindow == rw)
    {
      return;
    }
    if (this->VTKWindow)
    {
      this->Release();
    }
    this->VTKWindow = rw;
    if (this->VTKWindow)
    {
      this->VTKWindow->RegisterGraphicsResources(this);
    }
  }

  // Called when the event is invoked
  void Release() VTK_OVERRIDE
  {
    if (this->VTKWindow && this->Handler && !this->Releasing)
    {
      this->Releasing = true;
      this->VTKWindow->PushContext();
      (this->Handler->*this->Method)(this->VTKWindow);
      this->VTKWindow->UnregisterGraphicsResources(this);
      this->VTKWindow->PopContext();
      this->VTKWindow = NULL;
      this->Releasing = false;
    }
  }
protected:
  T* Handler;
  void (T::*Method)(vtkWindow *);
};

#endif
// VTK-HeaderTest-Exclude: vtkOpenGLResourceFreeCallback.h
