/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifdef _MSC_VER
#pragma warning(disable:4702)
#pragma warning(disable:4284)
#endif

#include "metaVesselTube.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

VesselTubePnt::
VesselTubePnt(int dim)
: TubePnt(dim)
{
}

VesselTubePnt::
~VesselTubePnt()
{
}

/** MetaVesselTube Constructors */
MetaVesselTube::
MetaVesselTube()
:MetaTube()
{
  if(META_DEBUG)
    {
    std::cout << "MetaVesselTube()" << std::endl;
    }
  this->Clear();
}


MetaVesselTube::
MetaVesselTube(const char *_headerName)
:MetaTube(_headerName)
{
  if(META_DEBUG)
    {
    std::cout << "MetaVesselTube()" << std::endl;
    }
  this->Clear();
}


MetaVesselTube::
MetaVesselTube(const MetaVesselTube *_VesselTube)
:MetaTube(_VesselTube)
{
  if(META_DEBUG)
    {
    std::cout << "MetaVesselTube()" << std::endl;
    }
  this->Clear();
}


MetaVesselTube::
MetaVesselTube(unsigned int dim)
:MetaTube(dim)
{
  if(META_DEBUG)
    {
    std::cout << "MetaVesselTube()" << std::endl;
    }
  this->Clear();
}

/** Destructor */
MetaVesselTube::
~MetaVesselTube()
{
}

/** Clear VesselTube information */
void MetaVesselTube::
Clear()
{
  if(META_DEBUG)
    {
    std::cout << "MetaVesselTube: Clear" << std::endl;
    }

  MetaTube::Clear();

  strcpy(m_ObjectSubTypeName,"Vessel");
}

//
void MetaVesselTube::
PrintInfo() const
{
  std::cout << "VesselTube" << std::endl;

  MetaTube::PrintInfo();
}

#if (METAIO_USE_NAMESPACE)
};
#endif
