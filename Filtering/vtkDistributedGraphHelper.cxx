/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedGraphHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* 
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
// .NAME vtkDistributedGraphHelper.cxx - distributed graph helper for vtkGraph
//
// .SECTION Description
// Attach a subclass of this helper to a vtkGraph to turn it into a distributed graph
#include "vtkDistributedGraphHelper.h"

//----------------------------------------------------------------------------
// class vtkDistributedGraphHelper
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkDistributedGraphHelper, "1.1.2.1");

//----------------------------------------------------------------------------
vtkDistributedGraphHelper::vtkDistributedGraphHelper() 
{ 
  this->Graph = 0;
}

//----------------------------------------------------------------------------
vtkDistributedGraphHelper::~vtkDistributedGraphHelper() 
{
}

//----------------------------------------------------------------------------
void vtkDistributedGraphHelper::AttachToGraph(vtkGraph *graph)
{
  this->Graph = graph;

}
