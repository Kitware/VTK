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

#include "vtkAMRFlashReaderInternal.h"

#include <cassert>

vtkStandardNewMacro( vtkAMRFlashParticlesReader );

vtkAMRFlashParticlesReader::vtkAMRFlashParticlesReader()
{
  this->Internal = new vtkFlashReaderInternal();
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMRFlashParticlesReader::~vtkAMRFlashParticlesReader()
{
  if( this->Internal != NULL )
    delete this->Internal;
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
  vtkPolyData* particles = vtkPolyData::New();
  // TODO: implement this
  return( particles );
}

//------------------------------------------------------------------------------
void vtkAMRFlashParticlesReader::SetupParticleDataSelections()
{
  // TODO: implement this
}
