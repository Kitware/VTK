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
#pragma warning(disable:4702)
#pragma warning(disable:4284)
#endif

#include "metaBlob.h"

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

BlobPnt::
BlobPnt(int dim)
{
  m_Dim = dim;
  m_X = new float[m_Dim];
  for(unsigned int i=0;i<m_Dim;i++)
    {
    m_X[i] = 0;
    }

  //Color is red by default
  m_Color[0]=1.0f;
  m_Color[1]=0.0f;
  m_Color[2]=0.0f;
  m_Color[3]=1.0f;
}

BlobPnt::
~BlobPnt()
{
  delete []m_X;
}

//
// MedImage Constructors
//
MetaBlob::
MetaBlob()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob()" << METAIO_STREAM::endl;
  m_NPoints = 0;
  Clear();
}

//
MetaBlob::
MetaBlob(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaBlob()" << METAIO_STREAM::endl;
  m_NPoints = 0;
  Clear();
  Read(_headerName);
}

//
MetaBlob::
MetaBlob(const MetaBlob *_blob)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaBlob()" << METAIO_STREAM::endl;
  m_NPoints = 0;
  Clear();
  CopyInfo(_blob);
}



//
MetaBlob::
MetaBlob(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob()" << METAIO_STREAM::endl;
  m_NPoints = 0;
  Clear();
}

//
MetaBlob::
~MetaBlob()
{
  Clear();
  M_Destroy();
}

//
void MetaBlob::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "PointDim = " << m_PointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaBlob::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}



void MetaBlob::
PointDim(const char* pointDim)
{
  strcpy(m_PointDim,pointDim);
}

const char* MetaBlob::
PointDim(void) const
{
  return m_PointDim;
}

void MetaBlob::
NPoints(size_t npnt)
{
  m_NPoints = npnt;
}

size_t MetaBlob::
NPoints(void) const
{
  return m_NPoints;
}


/** Clear blob information */
void MetaBlob::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob: Clear: m_NPoints" << METAIO_STREAM::endl;
  // Delete the list of pointers to blobs.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    BlobPnt* pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();
  m_NPoints = 0;
  strcpy(m_PointDim, "x y z red green blue alpha");
  m_ElementType = MET_FLOAT;
}

/** Destroy blob information */
void MetaBlob::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaBlob::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob: M_SetupReadFields" << METAIO_STREAM::endl;

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

MET_ValueEnumType MetaBlob::
ElementType(void) const
{
  return m_ElementType;
}

void MetaBlob::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}


void MetaBlob::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Blob");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  char s[255];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_ElementType, s);
  MET_InitWriteField(mF, "ElementType", MET_STRING, strlen(s), s);
  m_Fields.push_back(mF);

  if(strlen(m_PointDim)>0)
  {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointDim", MET_STRING,
                           strlen(m_PointDim),m_PointDim);
    m_Fields.push_back(mF);
  }

  m_NPoints = (int)m_PointList.size();
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NPoints", MET_INT,static_cast<double>(m_NPoints));
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Points", MET_NONE);
  m_Fields.push_back(mF);

}



bool MetaBlob::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob: M_Read: Loading Header" << METAIO_STREAM::endl;

  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaBlob: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaBlob: M_Read: Parsing Header" << METAIO_STREAM::endl;

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if(mF->defined)
  {
    m_NPoints= (int)mF->value[0];
  }

  mF = MET_GetFieldRecord("ElementType", &m_Fields);
  if(mF->defined)
  {
    MET_StringToType((char *)(mF->value), &m_ElementType);
  }


  mF = MET_GetFieldRecord("PointDim", &m_Fields);
  if(mF->defined)
  {
    strcpy(m_PointDim,(char *)(mF->value));
  }

  int* posDim= new int[m_NDims];
  int i;
  for(i= 0; i < m_NDims; i++)
  {
    posDim[i] = -1;
  }

  int pntDim;
  char** pntVal = NULL;
  MET_StringToWordArray(m_PointDim, &pntDim, &pntVal);


  for(int j = 0; j < pntDim; j++)
  {
    if(!strcmp(pntVal[j], "x") || !strcmp(pntVal[j], "X"))
    {
      posDim[0] = j;
    }
    if(!strcmp(pntVal[j], "y") || !strcmp(pntVal[j], "Y"))
    {
      posDim[1] = j;
    }
    if(!strcmp(pntVal[j], "z") || !strcmp(pntVal[j], "Z"))
    {
      posDim[2] = j;
    }
  }

  for(i=0;i<pntDim;i++)
    {
    delete [] pntVal[i];
    }

  delete [] pntVal;

  float v[16];

  if(m_BinaryData)
  {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    size_t readSize = m_NPoints*(m_NDims+4)*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    size_t gc = static_cast<size_t>(m_ReadStream->gcount());
    if(gc != readSize)
    {
      METAIO_STREAM::cout << "MetaBlob: m_Read: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc << METAIO_STREAM::endl;
      delete [] _data;
      delete [] posDim;
      return false;
    }

    i=0;
    int d;
    unsigned int k;
    for(size_t j=0; j<m_NPoints; j++)
    {
      BlobPnt* pnt = new BlobPnt(m_NDims);

      for(d=0; d<m_NDims; d++)
      {
        float* num = new float[1];
        char* numAlias = reinterpret_cast<char*>(num);
        for(k=0;k<sizeof(float);k++)
          {
          numAlias[k] = _data[i+k];
          }
        float td = num[0];
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_X[d] = td;
        delete [] num;
      }

      for(d=0; d<4; d++)
      {
        float* num = new float[1];
        char* numAlias = reinterpret_cast<char*>(num);
        for(k=0;k<sizeof(float);k++)
          {
          numAlias[k] = _data[i+k];
          }
        float td = num[0];
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Color[d] = td;
        delete [] num;
      }

      m_PointList.push_back(pnt);
    }
    delete [] _data;
  }
  else
  {
    for(size_t j=0; j<m_NPoints; j++)
    {
      BlobPnt* pnt = new BlobPnt(m_NDims);

      for(int k=0; k<pntDim; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
      }

      int d;
      for(d=0; d<m_NDims; d++)
      {
        pnt->m_X[d] = v[posDim[d]];
      }

      for(d=0; d<4; d++)
      {
        pnt->m_Color[d] = v[d+m_NDims];
      }

      m_PointList.push_back(pnt);
    }

    if(m_NPoints>0)
      {
      char c = ' ';
      while( (c!='\n') && (!m_ReadStream->eof()))
        {
        c = static_cast<char>(m_ReadStream->get());// to avoid unrecognized characters
        }
      }
  }

  delete [] posDim;
  return true;
}


bool MetaBlob::
M_Write(void)
{

  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaBlob: M_Read: Error parsing file"
                        << METAIO_STREAM::endl;
    return false;
    }

  /** Then copy all points */
  if(m_BinaryData)
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = new char[(m_NDims+4)*m_NPoints*elementSize];
    int i=0;
    int d;
    while(it != itEnd)
      {
      for(d = 0; d < m_NDims; d++)
        {
        float pntX = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&pntX,MET_FLOAT);
        MET_DoubleToValue((double)pntX,m_ElementType,data,i++);
        }

      for(d = 0; d < 4; d++)
        {
        float c = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&c,MET_FLOAT);
        MET_DoubleToValue((double)c,m_ElementType,data,i++);
        }
      ++it;
      }
    m_WriteStream->write((char *)data,(m_NDims+4)*m_NPoints*elementSize);
    m_WriteStream->write("\n",1);
    delete [] data;
    }
  else
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();

    int d;
    while(it != itEnd)
      {
      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_X[d] << " ";
        }

      for(d = 0; d < 4; d++)
        {
        *m_WriteStream << (*it)->m_Color[d] << " ";
        }

      *m_WriteStream << METAIO_STREAM::endl;
      ++it;
      }
    }

  return true;

}

#if (METAIO_USE_NAMESPACE)
};
#endif
