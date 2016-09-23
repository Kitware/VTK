/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkPolyDataPainter.h"

#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkInformationKeyMacro(vtkPolyDataPainter, BUILD_NORMALS, Integer);
vtkInformationKeyMacro(vtkPolyDataPainter, DATA_ARRAY_TO_VERTEX_ATTRIBUTE, ObjectBase);
vtkInformationKeyMacro(vtkPolyDataPainter, DISABLE_SCALAR_COLOR, Integer);
vtkInformationKeyMacro(vtkPolyDataPainter, SHADER_DEVICE_ADAPTOR, ObjectBase)

//-----------------------------------------------------------------------------
vtkPolyDataPainter::vtkPolyDataPainter()
{
  this->BuildNormals = 1;
}

//-----------------------------------------------------------------------------
vtkPolyDataPainter::~vtkPolyDataPainter()
{
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::Render(vtkRenderer* renderer, vtkActor* actor,
                                unsigned long typeflags,bool forceCompileOnly)
{
  if (!this->GetInputAsPolyData())
  {
    vtkErrorMacro("No polydata input!");
    return;
  }

  this->Superclass::Render(renderer, actor, typeflags,forceCompileOnly);
}

//-----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataPainter::GetInputAsPolyData()
{
  return vtkPolyData::SafeDownCast(this->GetInput());
}

//-----------------------------------------------------------------------------
vtkPolyData* vtkPolyDataPainter::GetOutputAsPolyData()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::ProcessInformation(vtkInformation* info)
{
  if (info->Has(vtkPolyDataPainter::BUILD_NORMALS()))
  {
    this->SetBuildNormals(info->Get(vtkPolyDataPainter::BUILD_NORMALS()));
  }
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BuildNormals: " << this->BuildNormals << endl;
}
