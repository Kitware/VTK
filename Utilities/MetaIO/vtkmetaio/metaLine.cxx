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

#include "metaLine.h"

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

LinePnt::
LinePnt(int dim)
{
  m_Dim = dim;

  m_X = new float[m_Dim];
  m_V = new float*[m_Dim-1];

  for(unsigned int i=0;i<m_Dim-1;i++)
    {
    m_V[i] = new float[m_Dim];
    for(unsigned int j=0;j<m_Dim;j++)
      {
      m_V[i][j] = 0;
      m_X[j] = 0;
      }
    }

  //Color is red by default
  m_Color[0]=1.0f;
  m_Color[1]=0.0f;
  m_Color[2]=0.0f;
  m_Color[3]=1.0f;
}

LinePnt::
~LinePnt()
{
  delete [] m_X;
  for(unsigned int i=0;i<m_Dim-1;i++)
    {
    delete [] m_V[i];
    }
  delete [] m_V;
}

//
// MetaLine Constructors
//
MetaLine::
MetaLine()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaLine::
MetaLine(const char *_headerName)
:MetaObject(_headerName)
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaLine()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaLine::
MetaLine(const MetaLine *_line)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaLine()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_line);
}



//
MetaLine::
MetaLine(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaLine::
~MetaLine()
{
  Clear();
  M_Destroy();
}

//
void MetaLine::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "PointDim = " << m_PointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaLine::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}



void MetaLine::
PointDim(const char* pointDim)
{
  strcpy(m_PointDim,pointDim);
}

const char* MetaLine::
PointDim(void) const
{
  return m_PointDim;
}

void MetaLine::
NPoints(int npnt)
{
  m_NPoints = npnt;
}

int MetaLine::
NPoints(void) const
{
  return m_NPoints;
}

/** Clear line information */
void MetaLine::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  m_NPoints = 0;
    // Delete the list of pointers to lines.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    LinePnt* pnt = *it;
    it++;
    delete pnt;
  }
  m_PointList.clear();

  strcpy(m_PointDim, "x y z v1x v1y v1z");
  m_ElementType = MET_FLOAT;
}

/** Destroy line information */
void MetaLine::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaLine::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  //int nDimsRecNum = MET_GetFieldRecordNumber("NDims", &m_Fields);

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

void MetaLine::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Line");
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
  MET_InitWriteField(mF, "NPoints", MET_INT,m_NPoints);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Points", MET_NONE);
  m_Fields.push_back(mF);

}

MET_ValueEnumType MetaLine::
ElementType(void) const
{
  return m_ElementType;
}

void MetaLine::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}


bool MetaLine::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine: M_Read: Loading Header" << METAIO_STREAM::endl;

  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaLine: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaLine: M_Read: Parsing Header" << METAIO_STREAM::endl;

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

  int pntDim;
  char** pntVal = NULL;
  MET_StringToWordArray(m_PointDim, &pntDim, &pntVal);

  int ii;
  for(ii=0;ii<pntDim;ii++)
    {
      delete [] pntVal[ii];
    }
  delete [] pntVal;

  float v[16];

  if(m_BinaryData)
  {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int readSize = m_NPoints*(m_NDims*m_NDims+4)*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if(gc != readSize)
    {
      METAIO_STREAM::cout << "MetaLine: m_Read: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc << METAIO_STREAM::endl;
      delete [] _data;
      return false;
    }

    int i=0;
    int d;
    unsigned int k;
    for(int j=0; j<m_NPoints; j++)
    {
      LinePnt* pnt = new LinePnt(m_NDims);

      for(d=0; d<m_NDims; d++)
        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_X[d] = (float)td;
        }

      for(int l=0;l<m_NDims-1;l++)
      {
        for(d=0; d<m_NDims; d++)
        {
          float td;
          char * const num = (char *)(&td);
          for(k=0;k<sizeof(float);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
          i+=sizeof(float);
          pnt->m_V[l][d] = (float)td;
        }
      }

      for(d=0; d<4; d++)
      {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Color[d] = (float)td;
      }

      m_PointList.push_back(pnt);
    }
    delete [] _data;
  }
  else
  {
    for(int j=0; j<m_NPoints; j++)
    {
      LinePnt* pnt = new LinePnt(m_NDims);

      int k;
      int d;
      for(k=0; k<m_NDims; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
      }

      //float* x = new float[m_NDims];
      for(d=0; d<m_NDims; d++)
      {
        pnt->m_X[d] = v[d];
      }

      //pnt.m_X = x;

      for(k=0; k<m_NDims-1; k++)
      {
        for(int jj=0; jj<m_NDims; jj++)
        {
          *m_ReadStream >> v[jj];
          m_ReadStream->get();
        }

        //float* n = new float[m_NDims];
        for(d=0; d<m_NDims; d++)
        {
          pnt->m_V[k][d] = v[d];
        }
        //pnt.m_V[k] = n;
      }
      for(k=0; k<4; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
        pnt->m_Color[k] = v[k];
      }

      m_PointList.push_back(pnt);
    }


    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
    {
      c = static_cast<char>(m_ReadStream->get());// to avoid unrecognized characters
    }
  }

  return true;
}


bool MetaLine::
M_Write(void)
{
  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaLine: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  /** Then copy all points */
  if(m_BinaryData)
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = new char[(m_NDims*m_NDims+4)*m_NPoints*elementSize];
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

      for(int j=0;j<m_NDims-1;j++)
        {
        for(d=0; d<m_NDims; d++)
          {
          float v = (*it)->m_V[j][d];
          MET_SwapByteIfSystemMSB(&v,MET_FLOAT);
          MET_DoubleToValue((double)v,m_ElementType,data,i++);
          }
        }

      for(d=0; d<4; d++)
        {
        float c = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&c,MET_FLOAT);
        MET_DoubleToValue((double)c,m_ElementType,data,i++);
        }

      it++;
      }

    m_WriteStream->write((char *)data,(m_NDims*m_NDims+4)*m_NPoints*elementSize);
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

      for(d = 0; d < m_NDims-1; d++)
      {
        for(int i = 0; i < m_NDims; i++)
        {
          *m_WriteStream << (*it)->m_V[d][i] << " ";
        }
      }

      for(d=0;d<4;d++)
      {
        *m_WriteStream << (*it)->m_Color[d] << " ";
      }

      *m_WriteStream << METAIO_STREAM::endl;
      it++;
    }
  }

  return true;

}

#if (METAIO_USE_NAMESPACE)
};
#endif
