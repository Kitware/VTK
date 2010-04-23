/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParallelFactory - 
// .SECTION Description

#ifndef __vtkParallelFactory_h
#define __vtkParallelFactory_h

#include "vtkObjectFactory.h"

class VTK_PARALLEL_EXPORT vtkParallelFactory : public vtkObjectFactory
{
public: 
// Methods from vtkObject
  vtkTypeMacro(vtkParallelFactory,vtkObjectFactory);
  static vtkParallelFactory *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual const char* GetVTKSourceVersion();
  virtual const char* GetDescription();
protected:
  vtkParallelFactory();
  ~vtkParallelFactory() { }
private:
  vtkParallelFactory(const vtkParallelFactory&);  // Not implemented.
  void operator=(const vtkParallelFactory&);  // Not implemented.
};

extern "C" VTK_PARALLEL_EXPORT vtkObjectFactory* vtkLoad();
#endif
