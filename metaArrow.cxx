/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaArrow.h"

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
// Constructors
//
MetaArrow::
MetaArrow()
:MetaObject()
{
  if(META_DEBUG) std::cout << "MetaArrow()" << std::endl;
  Clear();
}

//
MetaArrow::
MetaArrow(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  std::cout << "MetaArrow()" << std::endl;
  Clear();
  Read(_headerName);
}

//
MetaArrow::
MetaArrow(const MetaArrow *_Arrow)
:MetaObject()
{
  if(META_DEBUG)  std::cout << "MetaArrow()" << std::endl;
  Clear();
  CopyInfo(_Arrow);
}

MetaArrow::
MetaArrow(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) std::cout << "MetaArrow()" << std::endl;
  Clear();
  m_NDims = dim;
}

//
MetaArrow::
~MetaArrow()
{
  M_Destroy();
}

//
void MetaArrow::
PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "Length = " << M_Length << std::endl;
  std::cout << "Direction = ";
  for (int i = 0; i < m_NDims; i++)
    {
    std::cout << M_Direction[i] << " ";
    }
  std::cout << std::endl;
}

void MetaArrow::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);

  if(_object)
    {
    const MetaArrow * arrow;
    try
      {
      arrow = static_cast<const MetaArrow *>(_object);
      }
    catch( ... )
      {
      return;
      }
    if( arrow )
      {
      M_Length = arrow->Length();
      const double* direction = arrow->Direction();
      for (int i = 0; i < m_NDims; i++)
        {
        M_Direction[i] = direction[i];
        }
      }
    }
}


void  MetaArrow::
Length(float length)
 {
  M_Length = length;
 }

float  MetaArrow::
Length() const
 {
  return M_Length;
 }

void  MetaArrow::
Direction(const double *direction)
 {
  for (int i = 0; i < m_NDims; i++)
    {
    M_Direction[i] = direction[i];
    }
 }

const double * MetaArrow::
Direction() const
 {
  return M_Direction;
 }

/** Clear Arrow information */
void MetaArrow::
Clear()
{
  if(META_DEBUG) std::cout << "MetaArrow: Clear" << std::endl;
  MetaObject::Clear();

  strcpy(m_ObjectTypeName,"Arrow");

  M_Length = 1;

  // zero out direction then set to (1,0,0)
  memset(M_Direction, 0, 10*sizeof(double));
  M_Direction[0] = 1.0;
}

/** Destroy Arrow information */
void MetaArrow::
M_Destroy()
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaArrow::
M_SetupReadFields()
{
  if(META_DEBUG) std::cout << "MetaArrow: M_SetupReadFields" << std::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF= new MET_FieldRecordType;
  MET_InitReadField(mF, "Length", MET_FLOAT, true);
  mF->terminateRead = false;
  m_Fields.push_back(mF);

  int nDimsRecordNumber = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF= new MET_FieldRecordType;
  MET_InitReadField(mF, "Direction", MET_DOUBLE_ARRAY, true, nDimsRecordNumber);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void MetaArrow::
M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Length", MET_FLOAT, M_Length);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Direction", MET_DOUBLE_ARRAY, m_NDims, M_Direction);
  m_Fields.push_back(mF);
}


bool MetaArrow::
M_Read()
{
  if(META_DEBUG) std::cout << "MetaArrow: M_Read: Loading Header" << std::endl;

  if(!MetaObject::M_Read())
{
    std::cout << "MetaArrow: M_Read: Error parsing file" << std::endl;
    return false;
}

  if(META_DEBUG) std::cout << "MetaArrow: M_Read: Parsing Header" << std::endl;

  MET_FieldRecordType * mF_length;
  mF_length = MET_GetFieldRecord("Length", &m_Fields);
  if(mF_length->defined)
    {
    M_Length= (float)mF_length->value[0];
    }

  MET_FieldRecordType * mF_direction;
  mF_direction = MET_GetFieldRecord("Direction", &m_Fields);
  if(mF_direction->defined)
    {
    for (int i = 0; i < m_NDims; i++)
      {
      M_Direction[i] = (double)mF_direction->value[i];
      }
    }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
