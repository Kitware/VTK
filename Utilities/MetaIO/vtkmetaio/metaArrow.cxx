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
#  pragma warning(disable : 4702)
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

//
// Constructors
//
MetaArrow::MetaArrow()
{
  META_DEBUG_PRINT( "MetaArrow()" );
  MetaArrow::Clear();
}

//
MetaArrow::MetaArrow(const char * _headerName)
{
  META_DEBUG_PRINT( "MetaArrow()" );
  MetaArrow::Clear();
  MetaArrow::Read(_headerName);
}

//
MetaArrow::MetaArrow(const MetaArrow * _arrow)
{
  META_DEBUG_PRINT( "MetaArrow()" );
  MetaArrow::Clear();
  MetaArrow::CopyInfo(_arrow);
}

MetaArrow::MetaArrow(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaArrow()" );
  MetaArrow::Clear();
  MetaObject::InitializeEssential(dim);
}

//
MetaArrow::~MetaArrow()
{
MetaObject::M_Destroy();
}

//
void
MetaArrow::PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "Length = " << m_Length << '\n';
  std::cout << "Position = ";
  for (int i = 0; i < m_NDims; i++)
  {
    std::cout << m_Position[i] << " ";
  }
  std::cout << '\n';
  std::cout << "Direction = ";
  for (int i = 0; i < m_NDims; i++)
  {
    std::cout << m_Direction[i] << " ";
  }
  std::cout << '\n';
}

void
MetaArrow::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);

  if (_object)
  {
    const MetaArrow * arrow;
    try
    {
      arrow = dynamic_cast<const MetaArrow *>(_object);
    }
    catch (...)
    {
      return;
    }
    if (arrow)
    {
      m_Length = arrow->Length();
      const double * direction = arrow->Direction();
      for (int i = 0; i < m_NDims; i++)
      {
        m_Direction[i] = direction[i];
      }
      const double * position = arrow->Position();
      for (int i = 0; i < m_NDims; i++)
      {
        m_Position[i] = position[i];
      }
    }
  }
}


void
MetaArrow::Length(float length)
{
  m_Length = length;
}

float
MetaArrow::Length() const
{
  return m_Length;
}

void
MetaArrow::Direction(const double * direction)
{
  for (int i = 0; i < m_NDims; i++)
  {
    m_Direction[i] = direction[i];
  }
}

const double *
MetaArrow::Direction() const
{
  return m_Direction;
}

const double *
MetaArrow::Position() const
{
  if (m_APIVersion == 1)
  {
    return m_Position;
  }
  else
  {
    return MetaObject::Position();
  }
}

double
MetaArrow::Position(int _i) const
{
  if (m_APIVersion == 1)
  {
    return m_Position[_i];
  }
  else
  {
    return MetaObject::Position(_i);
  }
}

void
MetaArrow::Position(const double * position)
{
  if (m_APIVersion == 1)
  {
    for (int i = 0; i < m_NDims; i++)
    {
      m_Position[i] = position[i];
    }
  }
  else
  {
    MetaObject::Position(position);
  }
}

void
MetaArrow::Position(int _i, double value)
{
  if (m_APIVersion == 1)
  {
    m_Position[_i] = value;
  }
  else
  {
    MetaObject::Position(_i, value);
  }
}

/** Clear Arrow information */
void
MetaArrow::Clear()
{
  META_DEBUG_PRINT( "MetaArrow: Clear" );
  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Arrow");

  m_Length = 1;

  // zero out direction then set to (1,0,0)
  memset(m_Direction, 0, 10 * sizeof(double));
  m_Direction[0] = 1.0;

  memset(m_Position, 0, 10 * sizeof(double));
}

/** Set Read fields */
void
MetaArrow::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaArrow: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Length", MET_FLOAT, true);
  mF->terminateRead = false;
  m_Fields.push_back(mF);

  int nDimsRecordNumber = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Position", MET_DOUBLE_ARRAY, false, nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Direction", MET_DOUBLE_ARRAY, true, nDimsRecordNumber);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void
MetaArrow::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Length", MET_FLOAT, m_Length);
  m_Fields.push_back(mF);

  if (m_APIVersion == 1)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Position", MET_DOUBLE_ARRAY, static_cast<size_t>(m_NDims), m_Position);
    m_Fields.push_back(mF);
  }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Direction", MET_DOUBLE_ARRAY, static_cast<size_t>(m_NDims), m_Direction);
  m_Fields.push_back(mF);
}

bool
MetaArrow::M_Read()
{
  META_DEBUG_PRINT( "MetaArrow: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaArrow: M_Read: Error parsing file" << '\n';
    return false;
  }

  META_DEBUG_PRINT( "MetaArrow: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;
  mF = MET_GetFieldRecord("Length", &m_Fields);
  if (mF && mF->defined)
  {
    m_Length = static_cast<float>(mF->value[0]);
  }

  mF = MET_GetFieldRecord("Position", &m_Fields);
  if (mF && mF->defined)
  {
    if (m_APIVersion == 1)
    {
      for (int i = 0; i < m_NDims; i++)
      {
        m_Position[i] = mF->value[i];
      }
      if (m_FileFormatVersion == 0)
      {
        for (int i = 0; i < m_NDims; i++)
        {
          m_Offset[i] = 0;
        }
      }
    }
  }
  else
  {
    if (m_FileFormatVersion == 1)
    {
      std::cout << "MetaArrow: M_Read: Position not found" << '\n';
      return false;
    }
    else // Old file format
    {
      if (m_APIVersion == 1) // new API - move offset to position
      {
        for (int i = 0; i < m_NDims; i++)
        {
          m_Position[i] = m_Offset[i];
          m_Offset[i] = 0;
        }
      }
    }
  }

  mF = MET_GetFieldRecord("Direction", &m_Fields);
  if (mF && mF->defined)
  {
    for (int i = 0; i < m_NDims; i++)
    {
      m_Direction[i] = mF->value[i];
    }
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
