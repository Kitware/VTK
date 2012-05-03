/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDistributedStreamTracer - Distributed streamline generator
// .SECTION Description
// This filter is temporary to be compatabile with ParaView and will soon be removed
// .SECTION See Also
// vtkStreamTracer vtkPStreamTracer

#ifndef __vtkDistributedStreamTracer_h
#define __vtkDistributedStreamTracer_h

#include "vtkPStreamTracer.h"

#include "vtkFiltersParallelTracersModule.h" // For export macro

class  VTKFILTERSPARALLELTRACERS_EXPORT vtkDistributedStreamTracer : public vtkPStreamTracer
{
public:
  vtkTypeMacro(vtkDistributedStreamTracer,vtkPStreamTracer);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkDistributedStreamTracer *New();

protected:

  vtkDistributedStreamTracer();
  ~vtkDistributedStreamTracer();
};

#endif
