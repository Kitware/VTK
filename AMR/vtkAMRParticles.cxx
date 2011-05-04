/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRParticles.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRParticles.h"
#include "vtkAMRParticleType.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRParticles);

vtkAMRParticles::vtkAMRParticles()
{
  this->Points = vtkPoints::New();
  this->Points->SetDataTypeToDouble();

  this->ParticleData = vtkPointData::New();
  this->ParticleType = VTK_GENERIC_PARTICLE;
}

//------------------------------------------------------------------------------
vtkAMRParticles::~vtkAMRParticles()
{
  if( this->Points != NULL )
   this->Points->Delete();

  if( this->ParticleData != NULL )
    this->ParticleData->Delete();

  this->Points       = NULL;
  this->ParticleData = NULL;
}

//------------------------------------------------------------------------------
void vtkAMRParticles::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
vtkIdType vtkAMRParticles::GetNumberOfParticles()
{
  return( this->Points->GetNumberOfPoints() );
}

//------------------------------------------------------------------------------
void vtkAMRParticles::SetNumberOfParticles( vtkIdType N )
{
  this->Points->SetNumberOfPoints( N );
}

//------------------------------------------------------------------------------
void vtkAMRParticles::SetParticle( const vtkIdType idx, double x[3] )
{
  assert( "pre: particle index out-of-bounds" &&
          (idx>=0) && (idx<this->GetNumberOfParticles()) );

  this->Points->SetPoint( idx, x );
}

//------------------------------------------------------------------------------
void vtkAMRParticles::SetParticle( const vtkIdType idx,
   const double x, const double y, const double z )
{
  assert( "pre: particle index out-of-bounds" &&
          (idx>=0) && (idx<this->GetNumberOfParticles()) );

  this->Points->SetPoint( idx, x, y, z );
}

//------------------------------------------------------------------------------
void vtkAMRParticles::GetParticle(
    const vtkIdType idx, double x[3] )
{
  assert( "pre: particle index out-of-bounds" &&
          (idx>=0) && (idx<this->GetNumberOfParticles()) );

  this->Points->GetPoint( idx, x );
}

//------------------------------------------------------------------------------
void vtkAMRParticles::GetParticle(
    const vtkIdType idx, double &x, double &y, double &z )
{
  assert( "pre: particle index out-of-bounds" &&
          (idx>=0) && (idx<this->GetNumberOfParticles()) );

  x = this->Points->GetPoint( idx )[ 0 ];
  y = this->Points->GetPoint( idx )[ 1 ];
  z = this->Points->GetPoint( idx )[ 2 ];
}

//------------------------------------------------------------------------------
vtkPointData* vtkAMRParticles::GetParticleData()
{
  return( this->ParticleData );
}
