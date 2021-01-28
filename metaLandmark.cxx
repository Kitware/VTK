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

#include "metaLandmark.h"

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE
{
#endif

LandmarkPnt::LandmarkPnt(int dim)
{
  m_Dim = static_cast<unsigned int>(dim);
  m_X = new float[m_Dim];
  for (unsigned int i = 0; i < m_Dim; i++)
  {
    m_X[i] = 0;
  }

  // Color is red by default
  m_Color[0] = 1.0f;
  m_Color[1] = 0.0f;
  m_Color[2] = 0.0f;
  m_Color[3] = 1.0f;
}

LandmarkPnt::~LandmarkPnt()
{
  delete[] m_X;
}

//
// MedImage Constructors
//
MetaLandmark::MetaLandmark()
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaLandmark()" );
  m_NPoints = 0;
  MetaLandmark::Clear();
}

//
MetaLandmark::MetaLandmark(const char * _headerName)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaLandmark()" );
  m_NPoints = 0;
  MetaLandmark::Clear();
  MetaLandmark::Read(_headerName);
}

//
MetaLandmark::MetaLandmark(const MetaLandmark * _tube)
  : MetaObject()
{
  META_DEBUG_PRINT( "MetaLandmark()" );
  m_NPoints = 0;
  MetaLandmark::Clear();
  MetaLandmark::CopyInfo(_tube);
}


//
MetaLandmark::MetaLandmark(unsigned int dim)
  : MetaObject(dim)
{
  META_DEBUG_PRINT( "MetaLandmark()" );
  m_NPoints = 0;
  MetaLandmark::Clear();
}

//
MetaLandmark::~MetaLandmark()
{
  MetaLandmark::Clear();
  MetaObject::M_Destroy();
}

//
void
MetaLandmark::PrintInfo() const
{
  MetaObject::PrintInfo();
  std::cout << "PointDim = " << m_PointDim << std::endl;
  std::cout << "NPoints = " << m_NPoints << std::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  std::cout << "ElementType = " << str << std::endl;
}

void
MetaLandmark::CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}


void
MetaLandmark::PointDim(const char * pointDim)
{
  strcpy(m_PointDim, pointDim);
}

const char *
MetaLandmark::PointDim() const
{
  return m_PointDim;
}

void
MetaLandmark::NPoints(int npnt)
{
  m_NPoints = npnt;
}

int
MetaLandmark::NPoints() const
{
  return m_NPoints;
}


/** Clear tube information */
void
MetaLandmark::Clear()
{
  META_DEBUG_PRINT( "MetaLandmark: Clear" );

  MetaObject::Clear();

  strcpy(m_ObjectTypeName, "Landmark");

  META_DEBUG_PRINT( "MetaLandmark: Clear: m_NPoints" );
  // Delete the list of pointers to tubes.
  auto it = m_PointList.begin();
  while (it != m_PointList.end())
  {
    LandmarkPnt * pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();
  m_NPoints = 0;
  strcpy(m_PointDim, "x y z red green blue alpha");
  m_ElementType = MET_FLOAT;
}

/** Set Read fields */
void
MetaLandmark::M_SetupReadFields()
{
  META_DEBUG_PRINT( "MetaLandmark: M_SetupReadFields" );

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDim", MET_STRING, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NPoints", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Points", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

MET_ValueEnumType
MetaLandmark::ElementType() const
{
  return m_ElementType;
}

void
MetaLandmark::ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}


void
MetaLandmark::M_SetupWriteFields()
{
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  char s[255];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_ElementType, s);
  MET_InitWriteField(mF, "ElementType", MET_STRING, strlen(s), s);
  m_Fields.push_back(mF);

  if (strlen(m_PointDim) > 0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointDim", MET_STRING, strlen(m_PointDim), m_PointDim);
    m_Fields.push_back(mF);
  }

  m_NPoints = static_cast<int>(m_PointList.size());
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NPoints", MET_INT, m_NPoints);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Points", MET_NONE);
  m_Fields.push_back(mF);
}


bool
MetaLandmark::M_Read()
{
  META_DEBUG_PRINT( "MetaLandmark: M_Read: Loading Header" );

  if (!MetaObject::M_Read())
  {
    std::cout << "MetaLandmark: M_Read: Error parsing file" << std::endl;
    return false;
  }

  META_DEBUG_PRINT( "MetaLandmark: M_Read: Parsing Header" );

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if (mF->defined)
  {
    m_NPoints = static_cast<int>(mF->value[0]);
  }

  mF = MET_GetFieldRecord("ElementType", &m_Fields);
  if (mF->defined)
  {
    MET_StringToType(reinterpret_cast<char *>(mF->value), &m_ElementType);
  }


  mF = MET_GetFieldRecord("PointDim", &m_Fields);
  if (mF->defined)
  {
    strcpy(m_PointDim, reinterpret_cast<char *>(mF->value));
  }

  int * posDim = new int[m_NDims];
  int   i;
  for (i = 0; i < m_NDims; i++)
  {
    posDim[i] = -1;
  }

  int     pntDim;
  char ** pntVal = nullptr;
  MET_StringToWordArray(m_PointDim, &pntDim, &pntVal);


  int j;
  for (j = 0; j < pntDim; j++)
  {
    if (!strcmp(pntVal[j], "x") || !strcmp(pntVal[j], "X"))
    {
      posDim[0] = j;
    }
    if (!strcmp(pntVal[j], "y") || !strcmp(pntVal[j], "Y"))
    {
      posDim[1] = j;
    }
    if (!strcmp(pntVal[j], "z") || !strcmp(pntVal[j], "Z"))
    {
      posDim[2] = j;
    }
  }

  for (i = 0; i < pntDim; i++)
  {
    delete[] pntVal[i];
  }
  delete[] pntVal;

  float v[16];

  if (m_BinaryData)
  {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    std::streamsize readSize = m_NPoints * (m_NDims + 4) * elementSize;

    char * _data = new char[static_cast<size_t>(readSize)];
    m_ReadStream->read(_data, readSize);

    std::streamsize gc = m_ReadStream->gcount();
    if (gc != readSize)
    {
      std::cout << "MetaLandmark: m_Read: data not read completely" << std::endl;
      std::cout << "   ideal = " << readSize << " : actual = " << gc << std::endl;
      delete[] _data;
      delete[] posDim;
      return false;
    }

    i = 0;
    int          d;
    unsigned int k;
    for (j = 0; j < m_NPoints; j++)
    {
      auto * pnt = new LandmarkPnt(m_NDims);

      for (d = 0; d < m_NDims; d++)
      {
        float        td;
        char * const num = reinterpret_cast<char *>(&td);
        for (k = 0; k < sizeof(float); k++)
        {
          num[k] = _data[i + k];
        }
        MET_SwapByteIfSystemMSB(&td, MET_FLOAT);
        i += sizeof(float);
        pnt->m_X[d] = td;
      }

      for (d = 0; d < 4; d++)
      {
        float        td;
        char * const num = reinterpret_cast<char *>(&td);
        for (k = 0; k < sizeof(float); k++)
        {
          num[k] = _data[i + k];
        }
        MET_SwapByteIfSystemMSB(&td, MET_FLOAT);
        i += sizeof(float);
        pnt->m_Color[d] = td;
      }

      m_PointList.push_back(pnt);
    }
    delete[] _data;
  }
  else
  {
    for (j = 0; j < m_NPoints; j++)
    {
      auto * pnt = new LandmarkPnt(m_NDims);

      for (int k = 0; k < pntDim; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
      }

      int d;
      for (d = 0; d < m_NDims; d++)
      {
        pnt->m_X[d] = v[posDim[d]];
      }

      for (d = 0; d < 4; d++)
      {
        pnt->m_Color[d] = v[d + m_NDims];
      }

      m_PointList.push_back(pnt);
    }


    char c = ' ';
    while ((c != '\n') && (!m_ReadStream->eof()))
    {
      c = static_cast<char>(m_ReadStream->get()); // to avoid unrecognize charactere
    }
  }

  delete[] posDim;
  return true;
}


bool
MetaLandmark::M_Write()
{

  if (!MetaObject::M_Write())
  {
    std::cout << "MetaLandmark: M_Read: Error parsing file" << std::endl;
    return false;
  }

  /** Then copy all points */
  if (m_BinaryData)
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int                           elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char * data = new char[(m_NDims + 4) * m_NPoints * elementSize];
    int    i = 0;
    int    d;
    while (it != itEnd)
    {
      for (d = 0; d < m_NDims; d++)
      {
        float x = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&x, MET_FLOAT);
        MET_DoubleToValue(static_cast<double>(x), m_ElementType, data, i++);
      }

      for (d = 0; d < 4; d++)
      {
        float c = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&c, MET_FLOAT);
        MET_DoubleToValue(static_cast<double>(c), m_ElementType, data, i++);
      }
      ++it;
    }
    m_WriteStream->write(data, (m_NDims + 4) * m_NPoints * elementSize);
    m_WriteStream->write("\n", 1);
    delete[] data;
  }
  else
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();

    int d;
    while (it != itEnd)
    {
      for (d = 0; d < m_NDims; d++)
      {
        *m_WriteStream << (*it)->m_X[d] << " ";
      }

      for (d = 0; d < 4; d++)
      {
        *m_WriteStream << (*it)->m_Color[d] << " ";
      }

      *m_WriteStream << std::endl;
      ++it;
    }
  }

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
