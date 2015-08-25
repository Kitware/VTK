/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPassThroughEdgeStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkPassThroughEdgeStrategy.h"

#include "vtkCellArray.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <utility>
#include <vector>
#include <map>

vtkStandardNewMacro(vtkPassThroughEdgeStrategy);

vtkPassThroughEdgeStrategy::vtkPassThroughEdgeStrategy()
{
}

vtkPassThroughEdgeStrategy::~vtkPassThroughEdgeStrategy()
{
}

void vtkPassThroughEdgeStrategy::Layout()
{
}

void vtkPassThroughEdgeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
