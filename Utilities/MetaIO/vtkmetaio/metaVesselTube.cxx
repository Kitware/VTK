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
#  pragma warning(disable : 4702)
#  pragma warning(disable : 4284)
#endif

#include "metaVesselTube.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

VesselTubePnt::VesselTubePnt(int dim)
  : TubePnt(dim)
{}

VesselTubePnt::~VesselTubePnt() = default;

/** MetaVesselTube Constructors */
MetaVesselTube::MetaVesselTube()
  : MetaTube()
{
  META_DEBUG_PRINT( "MetaVesselTube()" );
  MetaVesselTube::Clear();
}


MetaVesselTube::MetaVesselTube(const char * _headerName)
  : MetaTube(_headerName)
{
  META_DEBUG_PRINT( "MetaVesselTube()" );
  MetaVesselTube::Clear();
}


MetaVesselTube::MetaVesselTube(const MetaVesselTube * _vesselTube)
  : MetaTube(_vesselTube)
{
  META_DEBUG_PRINT( "MetaVesselTube()" );
  MetaVesselTube::Clear();
}


MetaVesselTube::MetaVesselTube(unsigned int dim)
  : MetaTube(dim)
{
  META_DEBUG_PRINT( "MetaVesselTube()" );
  MetaVesselTube::Clear();
}

/** Destructor */
MetaVesselTube::~MetaVesselTube() = default;

/** Clear VesselTube information */
void
MetaVesselTube::Clear()
{
  META_DEBUG_PRINT( "MetaVesselTube: Clear" );

  MetaTube::Clear();

  strcpy(m_ObjectSubTypeName, "Vessel");
}

//
void
MetaVesselTube::PrintInfo() const
{
  std::cout << "VesselTube" << std::endl;

  MetaTube::PrintInfo();
}

#if (METAIO_USE_NAMESPACE)
};
#endif
