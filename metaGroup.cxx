/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaGroup.h"

#ifdef _MSC_VER
#  pragma warning(disable : 4702)
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

//
// MedImage Constructors
//
MetaGroup::MetaGroup()
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaGroup()" );
  MetaGroup::Clear();
}

//
MetaGroup::MetaGroup(const char * _headerName)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaGroup()" );
  MetaGroup::Clear();
  MetaGroup::Read(_headerName);
}

//
MetaGroup::MetaGroup(const MetaGroup * _group)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaGroup()" );
  MetaGroup::Clear();
  MetaGroup::CopyInfo(_group);
}

MetaGroup::MetaGroup(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaGroup()" );
  MetaGroup::Clear();
}

//
MetaGroup::~MetaGroup()
{
MetaObject::M_Destroy();
}

//
void
MetaGroup::PrintInfo() const
{
  MetaObject::PrintInfo();
}

void
MetaGroup::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

/** Clear group information */
void
MetaGroup::Clear()
{
  META_DEBUG_PRINT( "MetaGroup: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Group");
}

/** Set Read fields */
void
MetaGroup::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaGroup: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  auto * mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "EndGroup", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  mF->required = false;
}

void
MetaGroup::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  auto * mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "EndGroup", MET_NONE);
  m_Fields.push_back(mF);
}


bool
MetaGroup::M_Read()
{
  META_DEBUG_PRINT( "MetaGroup: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaGroup: M_Read: Error parsing file" << std::endl;
    return false;
  }

  META_DEBUG_PRINT( "MetaGroup: M_Read: Parsing Header" );

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
