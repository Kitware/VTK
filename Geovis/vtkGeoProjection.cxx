/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoProjection.cxx

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

#include "vtkGeoProjection.h"

#include "vtkObjectFactory.h"

#include <vtksys/ios/sstream>

#include "vtk_libproj4.h"

vtkStandardNewMacro(vtkGeoProjection);

static int vtkGeoProjectionNumProj = -1;

int vtkGeoProjection::GetNumberOfProjections()
{
  if ( vtkGeoProjectionNumProj < 0 )
    {
    vtkGeoProjectionNumProj = 0;
    for ( const PROJ_LIST* pj = proj_list; pj && pj->id; ++ pj )
      ++ vtkGeoProjectionNumProj;
    }
  return vtkGeoProjectionNumProj;
}

const char* vtkGeoProjection::GetProjectionName( int projection )
{
  if ( projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections() )
    return 0;

  return proj_list[projection].id;
}

const char* vtkGeoProjection::GetProjectionDescription( int projection )
{
  if ( projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections() )
    return 0;

  return proj_list[projection].descr[0];
}

vtkGeoProjection::vtkGeoProjection()
{
  this->Name = 0;
  this->SetName( "latlong" );
  this->CentralMeridian = 0.;
  this->Projection = 0;
}

vtkGeoProjection::~vtkGeoProjection()
{
  this->SetName( 0 );
  if ( this->Projection )
    {
    proj_free( this->Projection );
    }
}

void vtkGeoProjection::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Name: " << this->Name << "\n";
  os << indent << "CentralMeridian: " << this->CentralMeridian << "\n";
  os << indent << "Projection: " << this->Projection << "\n";
}

int vtkGeoProjection::GetIndex()
{
  this->UpdateProjection();
  if ( ! this->Projection )
    return -1;
  int i = 0;
  for ( const PROJ_LIST* proj = proj_list; proj && proj->id; ++ proj, ++ i )
    {
    if ( ! strcmp( proj->id, this->Name ) )
      {
      return i;
      }
    }
  return -1;
}

const char* vtkGeoProjection::GetDescription()
{
  this->UpdateProjection();
  if ( ! this->Projection )
    {
    return 0;
    }
  return this->Projection->descr;
}

PROJ* vtkGeoProjection::GetProjection()
{
  this->UpdateProjection();
  return this->Projection;
}


int vtkGeoProjection::UpdateProjection()
{
  if ( this->GetMTime() <= this->ProjectionMTime )
    {
    return 0;
    }

  if ( this->Projection )
    {
    proj_free( this->Projection );
    this->Projection = 0;
    }

  if ( ! this->Name || ! strlen( this->Name ) )
    {
    return 1;
    }

  if ( ! strcmp ( this->Name, "latlong" ) )
    {
    // latlong is "null" projection.
    return 1;
    }

  const char* pjArgs[3];
  vtkstd::string projSpec( "+proj=" );
  projSpec += this->Name;
  vtkstd::string ellpsSpec( "+ellps=clrk66" );
  vtkstd::string meridSpec;
  vtksys_ios::ostringstream os;
  os << "+lon_0=" << this->CentralMeridian;
  meridSpec = os.str();
  pjArgs[0] = projSpec.c_str();
  pjArgs[1] = ellpsSpec.c_str();
  pjArgs[2] = meridSpec.c_str();

  this->Projection = proj_init( 3, const_cast<char**>( pjArgs ) );
  if ( this->Projection )
    {
    return 1;
    }
  return 0;
}

