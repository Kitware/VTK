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
#pragma warning(disable:4702)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

//
// MedImage Constructors
//
MetaGroup::
MetaGroup()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGroup()" << METAIO_STREAM::endl;
  Clear();

}

//
MetaGroup::
MetaGroup(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaGroup()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaGroup::
MetaGroup(const MetaGroup *_group)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaGroup()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_group);
}

MetaGroup::
MetaGroup(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGroup()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaGroup::
~MetaGroup()
{
  M_Destroy();
}

//
void MetaGroup::
PrintInfo() const
{
  MetaObject::PrintInfo();
}

void MetaGroup::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

/** Clear group information */
void MetaGroup::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGroup: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
}

/** Destroy group information */
void MetaGroup::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaGroup::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGroup: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "EndGroup", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  mF->required = false;
}

void MetaGroup::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Group");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "EndGroup", MET_NONE);
  m_Fields.push_back(mF);
}


bool MetaGroup::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaGroup: M_Read: Loading Header" << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaGroup: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaGroup: M_Read: Parsing Header" << METAIO_STREAM::endl;
    }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
