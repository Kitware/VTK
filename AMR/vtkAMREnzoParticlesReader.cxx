/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoParticlesReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMREnzoParticlesReader.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

#include <cassert>

//#include "vtkAMREnzoReaderInternal.hpp"

#define H5_USE_16_API
#include <hdf5.h>      // for the HDF data loading engine


vtkStandardNewMacro(vtkAMREnzoParticlesReader);

//------------------------------------------------------------------------------
vtkAMREnzoParticlesReader::vtkAMREnzoParticlesReader()
{
//  this->Internal = new vtkEnzoReaderInternal( NULL );
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMREnzoParticlesReader::~vtkAMREnzoParticlesReader()
{
//  if( this->Internal != NULL )
//    delete this->Internal;
//  this->Internal = NULL;
}

//------------------------------------------------------------------------------
void vtkAMREnzoParticlesReader::PrintSelf(
    std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMREnzoParticlesReader::ReadMetaData()
{
//  if( this->Initialized )
//    return;
//
//  this->Internal->SetFileName( this->FileName );
//  this->Internal->ReadMetaData();
//  this->NumberOfBlocks = this->Internal->NumberOfBlocks;
//  this->Initialized    = true;
}

//------------------------------------------------------------------------------
vtkPolyData* vtkAMREnzoParticlesReader::ReadParticles(const int blkidx)
{
  vtkPolyData *particles = vtkPolyData::New();
//  vtkPoints   *positions = vtkPoints::New();
//  vtkPointData *pdata    = particles->GetPointData();
//
//  assert( "pre: particles dataset should not be NULL" && (particles != NULL) );
//
//  // this->Internal->Blocks includes a pseudo block -- the roo as block #0
//  int iBlockIdx    = blkidx+1;
//  int NumParticles = this->Internal->Blocks[ iBlockIdx ].NumberOfParticles;
//
//  if( NumParticles > 0 )
//    {
//      vtkstd::string pfile = this->Internal->Blocks[iBlockIdx].ParticleFileName;
//      if( pfile == "" )
//        {
//          vtkErrorMacro( "Empty paricles file!" );
//        }
//    }
//
//  // TODO: Implement this
//
//  particles->SetPoints( positions );
//  positions->Delete();
  return( particles );
}
