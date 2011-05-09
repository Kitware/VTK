/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRFlashParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRFlashParticlesReader.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <cassert>

vtkStandardNewMacro( vtkAMRFlashParticlesReader );

vtkAMRFlashParticlesReader::vtkAMRFlashParticlesReader()
{
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMRFlashParticlesReader::~vtkAMRFlashParticlesReader()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRFlashParticlesReader::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRFlashParticlesReader::ReadMetaData()
{
  // TODO: implement this
}

//------------------------------------------------------------------------------
vtkPolyData* vtkAMRFlashParticlesReader::ReadParticles( const int blkidx )
{
  vtkPolytData* particles = vtkPolyData::New();
  // TODO: implement this
  return( particles );
}
