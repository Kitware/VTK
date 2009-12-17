/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestingObjectFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/
#ifndef __vtkTestingObjectFactory_h
#define __vtkTestingObjectFactory_h

// .NAME vtkTestingObjectFactory - Object overrides used during testing
// .SECTION Description
// Some vtk examples and tests need to perform differently when they
// are run as tests versus when they are run as individual
// programs. Many tests/examples are interactive and eventually call
// vtkRenderWindowInteration::Start() to initialie the
// interaction. But, when run as tests, these programs should
// exit. This factory overrides vtkRenderWindowInteractor so that the
// Start() method just returns.
// To use this factory:
//   #include "vtkTestingObjectFactory.h"
//   vtkTestingObjectFactory* factory = vtkTestingObjectFactory::New();
//   vtkObjectFactory::RegisterFactory(factory);
//
#include "vtkObject.h"
#include "vtkVersion.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkTestDriver.h"

class VTK_EXPORT vtkTestingObjectFactory : public vtkObjectFactory
{
public:
  static vtkTestingObjectFactory* New() { return new vtkTestingObjectFactory;}
  vtkTypeRevisionMacro(vtkTestingObjectFactory,vtkObjectFactory);
  virtual const char* GetVTKSourceVersion() { return VTK_SOURCE_VERSION; }
  const char* GetDescription() { return "Factory for overrides during testing"; }

protected:
  // Description:
  // Register objects that override vtk objects whem they are run as tests.
  vtkTestingObjectFactory();

private:
  vtkTestingObjectFactory(const vtkTestingObjectFactory&); // Not implemented
  void operator=(const vtkTestingObjectFactory&);          // Not implemented
};


// .NAME vtkTestingInteractor - A RenderWindowInteractor for testing
// .SECTION Description
// Provides a Start() method that just returns. This permits programs
// run as tests to exit gracefully during the test run.
class vtkTestingInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkTestingInteractor* New() { return new vtkTestingInteractor; }
  vtkTypeRevisionMacro(vtkTestingInteractor,vtkRenderWindowInteractor);
  virtual void Start()
    {
    cout << "Start: This is the testing interactor. Start() does nothing"
         << endl;
    }

protected:
  vtkTestingInteractor()
    {
    cout << "New: Constructing the testing interactor" << endl;
    }

private:
  vtkTestingInteractor(const vtkTestingInteractor&); // Not implemented
  void operator=(const vtkTestingInteractor&);       // Not implemented
};

#endif
