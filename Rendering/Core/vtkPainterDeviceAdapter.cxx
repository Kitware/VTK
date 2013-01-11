/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterDeviceAdapter.cxx

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

#include "vtkPainterDeviceAdapter.h"

#include "vtkObjectFactory.h"

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkPainterDeviceAdapter)

//-----------------------------------------------------------------------------
vtkPainterDeviceAdapter::vtkPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
vtkPainterDeviceAdapter::~vtkPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
void vtkPainterDeviceAdapter::SendMaterialProperties(int,
                                                     int,
                                                     const void*,
                                                     const void*,
                                                     const void*,
                                                     const void*)
{
  // should be implemented by subclasses
}

//-----------------------------------------------------------------------------
void vtkPainterDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
