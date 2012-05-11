/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedAAHexahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// High quality volume renderer for axis-aligned hexahedra
// Implementation by Stephane Marchesin (stephane.marchesin@gmail.com)
// CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France
// BP12, F-91297 Arpajon, France.
//
// This file implements the paper
// "High-Quality, Semi-Analytical Volume Rendering for AMR Data",
// Stephane Marchesin and Guillaume Colin de Verdiere, IEEE Vis 2009.


#include "vtkProjectedAAHexahedraMapper.h"

#include "vtkCellCenterDepthSort.h"
#include "vtkGarbageCollector.h"
#include "vtkObjectFactory.h"

// ----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkProjectedAAHexahedraMapper)

// ----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkProjectedAAHexahedraMapper,
                     VisibilitySort, vtkVisibilitySort);

// ----------------------------------------------------------------------------
vtkProjectedAAHexahedraMapper::vtkProjectedAAHexahedraMapper()
{
  this->VisibilitySort = vtkCellCenterDepthSort::New();
}

// ----------------------------------------------------------------------------
vtkProjectedAAHexahedraMapper::~vtkProjectedAAHexahedraMapper()
{
  this->SetVisibilitySort(NULL);
}

// ----------------------------------------------------------------------------
void vtkProjectedAAHexahedraMapper::PrintSelf(ostream &os,
                                                    vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VisibilitySort: " << this->VisibilitySort << endl;
}

// ----------------------------------------------------------------------------
void vtkProjectedAAHexahedraMapper::ReportReferences(
  vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->VisibilitySort, "VisibilitySort");
}
