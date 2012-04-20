/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkPassThroughLayoutStrategy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPassThroughLayoutStrategy);

// ----------------------------------------------------------------------

vtkPassThroughLayoutStrategy::vtkPassThroughLayoutStrategy()
{
}

// ----------------------------------------------------------------------

vtkPassThroughLayoutStrategy::~vtkPassThroughLayoutStrategy()
{
}

// ----------------------------------------------------------------------
// Set the graph that will be laid out
void vtkPassThroughLayoutStrategy::Initialize()
{
}

// ----------------------------------------------------------------------

// Simple graph layout method
void vtkPassThroughLayoutStrategy::Layout()
{
}

void vtkPassThroughLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
