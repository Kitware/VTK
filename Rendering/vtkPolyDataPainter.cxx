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

#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkDebugLeaks.h"
#include "vtkGarbageCollector.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPolyDataPainter, "1.1.2.1");
vtkCxxSetObjectMacro(vtkPolyDataPainter, PolyData, vtkPolyData);
vtkInformationKeyMacro(vtkPolyDataPainter, BUILD_NORMALS, Integer);
//-----------------------------------------------------------------------------
vtkPolyDataPainter::vtkPolyDataPainter()
{
  this->PolyData = NULL;
  this->BuildNormals = 1;
}

//-----------------------------------------------------------------------------
vtkPolyDataPainter::~vtkPolyDataPainter()
{
  this->SetPolyData(NULL);
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::Render(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags)
{
  if (!this->PolyData)
    {
    vtkErrorMacro("No input!");
    return;
    }
  this->Superclass::Render(renderer, actor, typeflags);
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::PassInformation(vtkPainter* toPainter)
{
  vtkPolyDataPainter* pdp = vtkPolyDataPainter::SafeDownCast(toPainter);
  if (pdp)
    {
    // Is the input to the toPainter correct?
    vtkPolyData* output = this->GetOutputData();
    if (output != pdp->GetPolyData())
      {
      // only pass the data when they differ.
      pdp->SetPolyData(output); 
      }
    }
  this->Superclass::PassInformation(toPainter);
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
void vtkPolyDataPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->PolyData, "Input PolyData");
}

//-----------------------------------------------------------------------------
void vtkPolyDataPainter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PolyData: (" << this->PolyData << ")" << endl;
  os << indent << "BuildNormals: " << this->BuildNormals << endl;
}

