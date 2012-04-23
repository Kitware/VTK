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
// \code
#include "vtkTestingRenderingModule.h" // For export macro
//   #include "vtkTestingObjectFactory.h"
//   vtkTestingObjectFactory* factory = vtkTestingObjectFactory::New();
//   vtkObjectFactory::RegisterFactory(factory);
// \endcode

#include "vtkObjectFactory.h"

#include "vtkTesting.h"            // Required for testing framework
#include "vtkTestDriver.h"         // Required for testing framework
#include "vtkTestingInteractor.h"  // Required for testing framework
#include "vtkSmartPointer.h"       // Required for testing framework


class VTKTESTINGRENDERING_EXPORT vtkTestingObjectFactory : public vtkObjectFactory
{
public:
  static vtkTestingObjectFactory* New();
  vtkTypeMacro(vtkTestingObjectFactory,vtkObjectFactory);
  virtual const char* GetVTKSourceVersion();
  const char* GetDescription() { return "Factory for overrides during testing"; }
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  // Description:
  // Register objects that override vtk objects whem they are run as tests.
  vtkTestingObjectFactory();

private:
  vtkTestingObjectFactory(const vtkTestingObjectFactory&); // Not implemented
  void operator=(const vtkTestingObjectFactory&);          // Not implemented
};
#endif
