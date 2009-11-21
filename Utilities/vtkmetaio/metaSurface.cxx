/*=========================================================================

  Program:   MetaIO
  Module:    metaSurface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifdef _MSC_VER
#pragma warning(disable:4702)
#pragma warning(disable:4284)
#endif

#include "metaSurface.h"

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

SurfacePnt::
SurfacePnt(int dim)
{ 
  m_Dim = dim;
  m_X = new float[m_Dim];
  m_V = new float[m_Dim];
  for(unsigned int i=0;i<m_Dim;i++)
    {
    m_X[i] = 0;
    m_V[i] = 0;
    } 
  //Color is red by default
  m_Color[0]=1.0f;
  m_Color[1]=0.0f;
  m_Color[2]=0.0f;
  m_Color[3]=1.0f;
}

SurfacePnt::
~SurfacePnt()
{
  delete []m_X;
  delete []m_V;
}
  

//
// MetaSurface Constructors
//
MetaSurface::
MetaSurface()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaSurface::
MetaSurface(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaSurface()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaSurface::
MetaSurface(const MetaSurface *_surface)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaSurface()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_surface);
}



//
MetaSurface::
MetaSurface(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaSurface::
~MetaSurface()
{
  Clear();
  
  M_Destroy();
}

//
void MetaSurface::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "PointDim = " << m_PointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaSurface::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

    

void MetaSurface::
PointDim(const char* pointDim)
{
  strcpy(m_PointDim,pointDim);
}
    
const char* MetaSurface::
PointDim(void) const
{
  return m_PointDim;
}

void MetaSurface::
NPoints(int npnt)
{
  m_NPoints = npnt;
}

int MetaSurface::
NPoints(void) const
{
  return m_NPoints;
}

/** Clear Surface information */
void MetaSurface::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  m_NPoints = 0;
  // Delete the list of pointers to tubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    SurfacePnt* pnt = *it;
    it++;
    delete pnt;
  }
  m_PointList.clear();
  strcpy(m_PointDim, "x y z v1x v1y v1z r g b");
  m_ElementType = MET_FLOAT;
}
        
/** Destroy Surface information */
void MetaSurface::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaSurface::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: M_SetupReadFields" << METAIO_STREAM::endl;

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

void MetaSurface::
M_SetupWriteFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: M_SetupWriteFields" << METAIO_STREAM::endl;

  strcpy(m_ObjectTypeName,"Surface");
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

MET_ValueEnumType MetaSurface::
ElementType(void) const
{
  return m_ElementType;
}

void MetaSurface::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}


bool MetaSurface::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: M_Read: Loading Header" << METAIO_STREAM::endl;

  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaSurface: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: M_Read: Parsing Header" << METAIO_STREAM::endl;
 
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

  int i;
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
    int readSize = m_NPoints*(m_NDims*2+4)*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = m_ReadStream->gcount();
    if(gc != readSize)
    {
      METAIO_STREAM::cout << "MetaSurface: m_Read: data not read completely" 
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc << METAIO_STREAM::endl;
      return false;
    }

    i=0;
    int d;
    unsigned int k;
    for(int j=0; j<m_NPoints; j++) 
    {
      SurfacePnt* pnt = new SurfacePnt(m_NDims);
     
      for(d=0; d<m_NDims; d++)
      {
        char* num = new char[sizeof(float)];
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        float td = (float)((float*)num)[0];
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float); 
        pnt->m_X[d] = (float)td;
        delete [] num;
      }

      for(d=0; d<m_NDims; d++)
      {
        char* num = new char[sizeof(float)];
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        float td = (float)((float*)num)[0];
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float); 
        pnt->m_V[d] = (float)td;
        delete [] num;
      }

       for(d=0; d<4; d++)
      {
        char* num = new char[sizeof(float)];
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        float td = (float)((float*)num)[0];
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float); 
        pnt->m_Color[d] = (float)td;
        delete [] num;
      }
      m_PointList.push_back(pnt);
    }
    delete [] _data;
  }
  else
  {
    for(int j=0; j<m_NPoints; j++) 
    {
      SurfacePnt* pnt = new SurfacePnt(m_NDims);

      for(int k=0; k<pntDim; k++)
      {
        *m_ReadStream >> v[k];
        m_ReadStream->get(); // char c =
      }

      int d;
      for(d=0; d<m_NDims; d++)
      {
        pnt->m_X[d] = v[d];
      }

      for(d=m_NDims; d<m_NDims*2; d++)
      {
        pnt->m_V[d-m_NDims] = v[d];
      }

      for(d=0; d<4; d++)
      {
        pnt->m_Color[d] = v[d+2*m_NDims];
      }

      m_PointList.push_back(pnt);
    }

      
    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
    {
      c = m_ReadStream->get();// to avoid unrecognize charactere
    }
  }

  return true;
}


bool MetaSurface::
M_Write(void)
{

  if(META_DEBUG) METAIO_STREAM::cout << "MetaSurface: M_Write" << METAIO_STREAM::endl;

  if(!MetaObject::M_Write())
  {
    METAIO_STREAM::cout << "MetaSurface: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  /** Then copy all points */
  
  if(m_BinaryData)
  {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = new char[(m_NDims*2+4)*m_NPoints*elementSize];
    int i=0;
    int d;
    while(it != itEnd)
    {
      for(d = 0; d < m_NDims; d++)
      {
        float x = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);     
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
      }

      for(d = 0; d < m_NDims; d++)
      {
        float v = (*it)->m_V[d];
        MET_SwapByteIfSystemMSB(&v,MET_FLOAT);    
        MET_DoubleToValue((double)v,m_ElementType,data,i++);
      }
      
      for(d=0; d<4; d++)
      {
        float c = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&c,MET_FLOAT);    
        MET_DoubleToValue((double)c,m_ElementType,data,i++);
      }

      it++;
    }
   
    m_WriteStream->write((char *)data,(m_NDims*2+4)*m_NPoints*elementSize);
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

      for(d = 0; d < m_NDims; d++)
      {
        *m_WriteStream << (*it)->m_V[d] << " ";
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

