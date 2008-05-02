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
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkStdString.h"
#include "vtkVariant.h"

//----------------------------------------------------------------------------
// class vtkDistributedGraphHelper
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkDistributedGraphHelper, "1.1.2.3");

//----------------------------------------------------------------------------
vtkDistributedGraphHelper::vtkDistributedGraphHelper() 
{ 
  this->Graph = 0;
  this->VertexDistribution = 0;
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

//----------------------------------------------------------------------------
void 
vtkDistributedGraphHelper::
SetVertexPedigreeIdDistribution(vtkVertexPedigreeIdDistribution Func, 
                                void *userData)
{
  this->VertexDistribution = Func;
  this->VertexDistributionUserData = userData;
}

//----------------------------------------------------------------------------
vtkIdType 
vtkDistributedGraphHelper::
GetVertexOwnerByPedigreeId(const vtkVariant& pedigreeId)
{
  vtkIdType numProcs 
    = this->Graph->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());
  if (this->VertexDistribution)
    {
    return (this->VertexDistribution(pedigreeId, 
                                     this->VertexDistributionUserData)
            % numProcs);
    }

  // Hash the variant in a very lame way.
  double numericValue;
  vtkStdString stringValue;
  const unsigned char *charsStart, *charsEnd;
  if (pedigreeId.IsNumeric())
    {
    // Convert every numeric value into a double.
    numericValue = pedigreeId.ToDouble();

    // Hash the characters in the double. 
    charsStart = reinterpret_cast<const unsigned char*>(&numericValue);
    charsEnd = charsStart + sizeof(double);
    }
  else if (pedigreeId.GetType() == VTK_STRING)
    {
    stringValue = pedigreeId.ToString();
    charsStart = reinterpret_cast<const unsigned char*>(stringValue.c_str());
    charsEnd = charsStart + stringValue.size();
    }
  else
    {
    vtkErrorMacro("Cannot hash vertex pedigree ID of type " 
                  << pedigreeId.GetType());
    return 0;
    }

  unsigned long hash = 5381;
  for (; charsStart != charsEnd; ++charsStart)
    {
    hash = ((hash << 5) + hash) ^ *charsStart;
    }

  return hash % numProcs;
}
