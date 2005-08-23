// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPainterDeviceAdapter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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

#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkPainterDeviceAdapter);
vtkCxxRevisionMacro(vtkPainterDeviceAdapter, "1.1.2.1");

//-----------------------------------------------------------------------------
vtkPainterDeviceAdapter::vtkPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
vtkPainterDeviceAdapter::~vtkPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
vtkPainterDeviceAdapter* vtkPainterDeviceAdapter::New()
{
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkPainterDeviceAdapter");
  return (vtkPainterDeviceAdapter*)ret;
}

//-----------------------------------------------------------------------------
void vtkPainterDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
