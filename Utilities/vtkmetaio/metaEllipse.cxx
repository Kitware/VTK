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
#pragma warning(disable:4702)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string>
#include <string.h> // for memset

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#include <mem.h>
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

//
// MedImage Constructors
//
MetaEllipse::
MetaEllipse()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse()" << METAIO_STREAM::endl;
  Clear();

}

//
MetaEllipse::
MetaEllipse(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaEllipse()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaEllipse::
MetaEllipse(const MetaEllipse *_ellipse)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaEllipse()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_ellipse);
}

MetaEllipse::
MetaEllipse(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaEllipse::
~MetaEllipse()
{
  M_Destroy();
}

//
void MetaEllipse::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "Radius = ";
  for(int i=0;i<m_NDims;i++)
  {
    METAIO_STREAM::cout << m_Radius[i] << " ";
  }
  METAIO_STREAM::cout << METAIO_STREAM::endl;
}

void MetaEllipse::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

void  MetaEllipse::
Radius(const float* radius)
{
  for(int i=0; i<m_NDims; i++)
  {
    m_Radius[i] = radius[i];
  }
}


void  MetaEllipse::
Radius(float radius)
{
  for(int i=0;i<m_NDims;i++)
  {
    m_Radius[i] = radius;
  }
}

void  MetaEllipse::
Radius(float r1,float r2)
{
  m_Radius[0] = r1;
  m_Radius[1] = r2;
}

void  MetaEllipse::
Radius(float r1,float r2, float r3)
{
  m_Radius[0] = r1;
  m_Radius[1] = r2;
  m_Radius[2] = r3;
}

const float*  MetaEllipse::
Radius(void) const 
{
  return m_Radius;
}
  
/** Clear ellipse information */
void MetaEllipse::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  memset(m_Radius, 0, 100*sizeof(float));
  
  for(int i=0; i<m_NDims; i++)
  {
    m_Radius[i] = 1;
  }
}
        
/** Destroy ellipse information */
void MetaEllipse::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaEllipse::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  int nDimsRecNum = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Radius", MET_FLOAT_ARRAY, true,nDimsRecNum);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

}

void MetaEllipse::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Ellipse");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Radius", MET_FLOAT_ARRAY, m_NDims,m_Radius);
  m_Fields.push_back(mF);
}


bool MetaEllipse::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse: M_Read: Loading Header" << METAIO_STREAM::endl;
  
  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaEllipse: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaEllipse: M_Read: Parsing Header" << METAIO_STREAM::endl;
 
  MET_FieldRecordType * mF;
 
  mF = MET_GetFieldRecord("Radius", &m_Fields);
  if(mF->defined)
  {
    for(int i=0;i<m_NDims;i++)
    {
      m_Radius[i]= (float)mF->value[i];
    }
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif

