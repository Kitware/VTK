/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PipelineParallelism.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiProcessController.h"

void pipe1(vtkMultiProcessController* controller, void* arg);
void pipe2(vtkMultiProcessController* controller, void* arg);
