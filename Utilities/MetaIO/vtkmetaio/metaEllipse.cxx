/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaEllipse.h"

#ifdef _MSC_VER
#  pragma warning(disable : 4702)
#endif

#include <cstring> // for memset

#if defined(__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#  include <mem.h>
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

//
// MedImage Constructors
//
MetaEllipse::MetaEllipse()
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaEllipse()" );
  MetaEllipse::Clear();
}

//
MetaEllipse::MetaEllipse(const char * _headerName)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaEllipse()" );
  MetaEllipse::Clear();
  MetaEllipse::Read(_headerName);
}

//
MetaEllipse::MetaEllipse(const MetaEllipse * _ellipse)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaEllipse()" );
  MetaEllipse::Clear();
  MetaEllipse::CopyInfo(_ellipse);
}

MetaEllipse::MetaEllipse(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaEllipse()" );
  MetaEllipse::Clear();
}

//
MetaEllipse::~MetaEllipse()
{
MetaObject::M_Destroy();
}

//
void
MetaEllipse::PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "Radius = ";
  for (int i = 0; i < m_NDims; i++)
  {
    std::cout << m_Radius[i] << " ";
  }
  std::cout << std::endl;
}

void
MetaEllipse::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

void
MetaEllipse::Radius(const float * radius)
{
  for (int i = 0; i < m_NDims; i++)
  {
    m_Radius[i] = radius[i];
  }
}


void
MetaEllipse::Radius(float radius)
{
  for (int i = 0; i < m_NDims; i++)
  {
    m_Radius[i] = radius;
  }
}

void
MetaEllipse::Radius(float r1, float r2)
{
  m_Radius[0] = r1;
  m_Radius[1] = r2;
}

void
MetaEllipse::Radius(float r1, float r2, float r3)
{
  m_Radius[0] = r1;
  m_Radius[1] = r2;
  m_Radius[2] = r3;
}

const float *
MetaEllipse::Radius() const
{
  return m_Radius;
}

/** Clear ellipse information */
void
MetaEllipse::Clear()
{
  META_DEBUG_PRINT( "MetaEllipse: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Ellipse");

  memset(m_Radius, 0, 100 * sizeof(float));

  for (int i = 0; i < m_NDims; i++)
  {
    m_Radius[i] = 1;
  }
}

/** Set Read fields */
void
MetaEllipse::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaEllipse: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  int nDimsRecNum = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Radius", MET_FLOAT_ARRAY, true, nDimsRecNum);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void
MetaEllipse::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Radius", MET_FLOAT_ARRAY, static_cast<size_t>(m_NDims), m_Radius);
  m_Fields.push_back(mF);
}


bool
MetaEllipse::M_Read()
{
  META_DEBUG_PRINT( "MetaEllipse: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaEllipse: M_Read: Error parsing file" << std::endl;
    return false;
  }

  META_DEBUG_PRINT( "MetaEllipse: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("Radius", &m_Fields);
  if (mF->defined)
  {
    for (int i = 0; i < m_NDims; i++)
    {
      m_Radius[i] = static_cast<float>(mF->value[i]);
    }
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
