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

#include <cctype>
#include <cstdio>
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
  if(META_DEBUG) std::cout << "MetaGroup()" << std::endl;
  Clear();

}

//
MetaGroup::
MetaGroup(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  std::cout << "MetaGroup()" << std::endl;
  Clear();
  Read(_headerName);
}

//
MetaGroup::
MetaGroup(const MetaGroup *_group)
:MetaObject()
{
  if(META_DEBUG)  std::cout << "MetaGroup()" << std::endl;
  Clear();
  CopyInfo(_group);
}

MetaGroup::
MetaGroup(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) std::cout << "MetaGroup()" << std::endl;
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
Clear()
{
  if(META_DEBUG) std::cout << "MetaGroup: Clear" << std::endl;

  MetaObject::Clear();

  strcpy(m_ObjectTypeName,"Group");
}

/** Destroy group information */
void MetaGroup::
M_Destroy()
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaGroup::
M_SetupReadFields()
{
  if(META_DEBUG) std::cout << "MetaGroup: M_SetupReadFields" << std::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "EndGroup", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  mF->required = false;
}

void MetaGroup::
M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "EndGroup", MET_NONE);
  m_Fields.push_back(mF);
}


bool MetaGroup::
M_Read()
{
  if(META_DEBUG)
    {
    std::cout << "MetaGroup: M_Read: Loading Header" << std::endl;
    }

  if(!MetaObject::M_Read())
    {
    std::cout << "MetaGroup: M_Read: Error parsing file" << std::endl;
    return false;
    }

  if(META_DEBUG)
    {
    std::cout << "MetaGroup: M_Read: Parsing Header" << std::endl;
    }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
