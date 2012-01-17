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
#include <string>
#include <map>
#include <vector>

#include "vtk_libproj4.h"

vtkStandardNewMacro(vtkGeoProjection);

static int vtkGeoProjectionNumProj = -1;

//-----------------------------------------------------------------------------
class vtkGeoProjection::vtkInternals
{
public:
  const char* GetKeyAt(int index)
  {
    if (static_cast<int>(this->OptionalParameters.size()) > index)
      {
      std::map< std::string, std::string >::iterator iter =
          this->OptionalParameters.begin();
      int nbIter = index;
      while(nbIter > 0)
        {
        nbIter--;
        iter++;
        }
      return iter->first.c_str();
      }
    return NULL;
  }

  const char* GetValueAt(int index)
  {
    if (static_cast<int>(this->OptionalParameters.size()) > index)
      {
      std::map< std::string, std::string >::iterator iter =
          this->OptionalParameters.begin();
      int nbIter = index;
      while(nbIter > 0)
        {
        nbIter--;
        iter++;
        }
      return iter->second.c_str();
      }
    return NULL;
  }

  std::map< std::string, std::string > OptionalParameters;
};

//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
const char* vtkGeoProjection::GetProjectionName( int projection )
{
  if ( projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections() )
    return 0;

  return proj_list[projection].id;
}
//-----------------------------------------------------------------------------
const char* vtkGeoProjection::GetProjectionDescription( int projection )
{
  if ( projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections() )
    return 0;

  return proj_list[projection].descr[0];
}
//-----------------------------------------------------------------------------
vtkGeoProjection::vtkGeoProjection()
{
  this->Name = 0;
  this->SetName( "latlong" );
  this->CentralMeridian = 0.;
  this->Projection = 0;
  this->Internals = new vtkInternals();
}
//-----------------------------------------------------------------------------
vtkGeoProjection::~vtkGeoProjection()
{
  this->SetName( 0 );
  if ( this->Projection )
    {
    proj_free( this->Projection );
    }
  delete this->Internals;
  this->Internals = NULL;
}
//-----------------------------------------------------------------------------
void vtkGeoProjection::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Name: " << this->Name << "\n";
  os << indent << "CentralMeridian: " << this->CentralMeridian << "\n";
  os << indent << "Projection: " << this->Projection << "\n";
  os << indent << "Optional parameters:\n";
  for(int i=0;i<this->GetNumberOfOptionalParameters();i++)
    {
    os << indent << " - " << this->GetOptionalParameterKey(i) << " = "
       << this->GetOptionalParameterValue(i) << "\n";
    }
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
const char* vtkGeoProjection::GetDescription()
{
  this->UpdateProjection();
  if ( ! this->Projection )
    {
    return 0;
    }
  return this->Projection->descr;
}
//-----------------------------------------------------------------------------
PROJ* vtkGeoProjection::GetProjection()
{
  this->UpdateProjection();
  return this->Projection;
}

//-----------------------------------------------------------------------------
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

  int argSize = 3 + this->GetNumberOfOptionalParameters();
  const char** pjArgs = new const char*[argSize];
  std::string projSpec( "+proj=" );
  projSpec += this->Name;
  std::string ellpsSpec( "+ellps=clrk66" );
  std::string meridSpec;
  vtksys_ios::ostringstream os;
  os << "+lon_0=" << this->CentralMeridian;
  meridSpec = os.str();
  pjArgs[0] = projSpec.c_str();
  pjArgs[1] = ellpsSpec.c_str();
  pjArgs[2] = meridSpec.c_str();

  // Add optional parameters
  std::vector<std::string> stringHolder; // Keep string ref in memory
  for(int i=0; i < this->GetNumberOfOptionalParameters(); i++)
    {
    vtksys_ios::ostringstream param;
    param << "+" << this->GetOptionalParameterKey(i);
    param << "=" << this->GetOptionalParameterValue(i);
    stringHolder.push_back(param.str());
    pjArgs[3+i] = stringHolder[i].c_str();
    }

  this->Projection = proj_init( argSize, const_cast<char**>( pjArgs ) );
  delete[] pjArgs;
  if ( this->Projection )
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGeoProjection::SetOptionalParameter(const char* key, const char* value)
{
  if(key != NULL && value != NULL)
    {
    this->Internals->OptionalParameters[key] = value;
    }
  else
    {
    vtkErrorMacro("Invalid Optional Parameter Key/Value pair. None can be NULL");
    }
  this->UpdateProjection();
}
//-----------------------------------------------------------------------------
void vtkGeoProjection::RemoveOptionalParameter(const char* key)
{
  this->Internals->OptionalParameters.erase(key);
  this->UpdateProjection();
}
//-----------------------------------------------------------------------------
int vtkGeoProjection::GetNumberOfOptionalParameters()
{
  return static_cast<int>(this->Internals->OptionalParameters.size());
}
//-----------------------------------------------------------------------------
const char* vtkGeoProjection::GetOptionalParameterKey(int index)
{
  return this->Internals->GetKeyAt(index);
}
//-----------------------------------------------------------------------------
const char* vtkGeoProjection::GetOptionalParameterValue(int index)
{
  return this->Internals->GetValueAt(index);
}
//-----------------------------------------------------------------------------
void vtkGeoProjection::ClearOptionalParameters()
{
  this->Internals->OptionalParameters.clear();
  this->UpdateProjection();
}
