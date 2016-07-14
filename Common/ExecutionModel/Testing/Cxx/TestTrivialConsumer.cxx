/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTrivialConsumer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkSphereSource.h"
#include "vtkTrivialConsumer.h"

int TestTrivialConsumer(int, char*[])
{
  vtkNew<vtkSphereSource> spheres;
  vtkNew<vtkTrivialConsumer> consumer;

  consumer->SetInputConnection(spheres->GetOutputPort());
  consumer->Update();

  return 0;
}
