/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCellTessellator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkCellArray.h"
#include "vtkGenericEdgeTable.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericCellIterator.h"

#include <vtkstd/queue>
#include <vtkstd/stack>
#include <assert.h>

#include "vtkMath.h"

vtkCxxRevisionMacro(vtkGenericCellTessellator, "1.7");
vtkCxxSetObjectMacro(vtkGenericCellTessellator, ErrorMetric, vtkGenericSubdivisionErrorMetric);

//-----------------------------------------------------------------------------
// Create the tessellator helper with a default of 0.25 for threshold
//
vtkGenericCellTessellator::vtkGenericCellTessellator()
{
  this->ErrorMetric = vtkGenericSubdivisionErrorMetric::New();
}

//-----------------------------------------------------------------------------
vtkGenericCellTessellator::~vtkGenericCellTessellator()
{
  this->SetErrorMetric( 0 );
}


//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ErrorMetric: " 
     << this->ErrorMetric << endl;
}
