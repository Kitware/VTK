/*=========================================================================

  Program:   MetaIO
  Module:    metaArrow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "metaArrow.h"

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
// Constructors
//
MetaArrow::
MetaArrow()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaArrow::
MetaArrow(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaArrow()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaArrow::
MetaArrow(const MetaArrow *_Arrow)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaArrow()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_Arrow);
}

MetaArrow::
MetaArrow(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow()" << METAIO_STREAM::endl;
  Clear();
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
  METAIO_STREAM::cout << "Length = " << M_Length << METAIO_STREAM::endl;
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
      arrow = (const MetaArrow *)(_object);
      }
    catch( ... )
      {
      return;
      }
    if( arrow )
      {
      M_Length = arrow->Length();
      }
    }
  }


void  MetaArrow::
Length(float length)
{
  M_Length = length;
}

float  MetaArrow::
Length(void) const 
{
  return M_Length;
}
  
/** Clear Arrow information */
void MetaArrow::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  M_Length = 1;
}
        
/** Destroy Arrow information */
void MetaArrow::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaArrow::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Length", MET_FLOAT, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void MetaArrow::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Arrow");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Length", MET_FLOAT, M_Length);
  m_Fields.push_back(mF);
}


bool MetaArrow::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow: M_Read: Loading Header" << METAIO_STREAM::endl;
  
  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaArrow: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaArrow: M_Read: Parsing Header" << METAIO_STREAM::endl;
 
  MET_FieldRecordType * mF;
 
  mF = MET_GetFieldRecord("Length", &m_Fields);
  if(mF->defined)
    {
    M_Length= (float)mF->value[0];
    }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif

