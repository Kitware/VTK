/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaGaussian.h"

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
MetaGaussian::
MetaGaussian()
:MetaObject( )
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian()" << METAIO_STREAM::endl;
  Clear();

}

//
MetaGaussian::
MetaGaussian(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaGaussian()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaGaussian::
MetaGaussian(const MetaGaussian *_gaussian)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaGaussian()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_gaussian);
}

MetaGaussian::
MetaGaussian(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaGaussian::
~MetaGaussian()
{
  M_Destroy();
}

//
void MetaGaussian::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "\n"
            << "Maximum = " << m_Maximum << "\n"
            << "Radius = " << m_Radius
            << METAIO_STREAM::endl;
}

void MetaGaussian::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}
  
/** Clear gaussian information */
void MetaGaussian::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  m_Maximum = 1;
  m_Radius = 1;
}
        
/** Destroy gaussian information */
void MetaGaussian::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaGaussian::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Maximum", MET_FLOAT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Radius", MET_FLOAT, true);
  m_Fields.push_back(mF);

}

void MetaGaussian::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Gaussian");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Maximum", MET_FLOAT, m_Maximum);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Radius", MET_FLOAT,
                     m_Radius);
  m_Fields.push_back(mF);

}


bool MetaGaussian::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian: M_Read: Loading Header"
                           << METAIO_STREAM::endl;

  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaGaussian: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaGaussian: M_Read: Parsing Header"
                           << METAIO_STREAM::endl;

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("Maximum", &m_Fields);
  if( mF->defined )
  {
    m_Maximum = (float)mF->value[0];
  }

  mF = MET_GetFieldRecord("Radius", &m_Fields);
  if( mF->defined )
  {
    m_Radius = (float)mF->value[0];
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif

