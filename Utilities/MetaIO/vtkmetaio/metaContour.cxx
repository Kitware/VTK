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

#include "metaContour.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

ContourControlPnt::
ContourControlPnt(int dim)
  {
  m_Id = 0;
  m_Dim = dim;
  m_X = new float[m_Dim];
  m_XPicked = new float[m_Dim];
  m_V = new float[m_Dim];
  for(unsigned int i=0;i<m_Dim;i++)
    {
    m_X[i] = 0;
    m_XPicked[i] = 0;
    m_V[i] = 0;
    }
  //Color is red by default
  m_Color[0]=1.0f;
  m_Color[1]=0.0f;
  m_Color[2]=0.0f;
  m_Color[3]=1.0f;
  }

ContourControlPnt::
~ContourControlPnt()
  {
  delete [] m_X;
  delete [] m_XPicked;
  delete [] m_V;
  }

/** Constructor */
MetaContour::
MetaContour()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour()" << METAIO_STREAM::endl;
  Clear();
}

/** Constructor */
MetaContour::
MetaContour(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaContour()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaContour::
MetaContour(const MetaContour *_Contour)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaContour()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_Contour);
}



//
MetaContour::
MetaContour(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaContour::
~MetaContour()
{
  Clear();
  M_Destroy();
}

//
void MetaContour::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "ControlPointDim = " << m_ControlPointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NControlPoints = " << m_NControlPoints << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "InterpolatedPointDim = " << m_InterpolatedPointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NInterpolatedPoints = " << m_NInterpolatedPoints << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Display Orientation = " << m_DisplayOrientation << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Attached to Slice = " << m_AttachedToSlice << METAIO_STREAM::endl;
}

void MetaContour::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

void MetaContour::
Closed(bool close)
{
  m_Closed = close;
}

bool MetaContour
::Closed() const
{
  return m_Closed;
}


void MetaContour::
ControlPointDim(const char* pointDim)
{
  strcpy(m_ControlPointDim,pointDim);
}

const char* MetaContour::
ControlPointDim(void) const
{
  return m_ControlPointDim;
}

void MetaContour::
InterpolatedPointDim(const char* pointDim)
{
  strcpy(m_InterpolatedPointDim,pointDim);
}

const char* MetaContour::
InterpolatedPointDim(void) const
{
  return m_InterpolatedPointDim;
}

int MetaContour::
NControlPoints(void) const
{
  return m_NControlPoints;
}

int MetaContour::
NInterpolatedPoints(void) const
{
  return m_NInterpolatedPoints;
}


MET_InterpolationEnumType
MetaContour::Interpolation(void) const
{
  return m_InterpolationType;
}

void MetaContour::Interpolation(MET_InterpolationEnumType _interpolation)
{
  m_InterpolationType = _interpolation;
}


/** Clear Contour information */
void MetaContour::
Clear(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaContour: Clear" << METAIO_STREAM::endl;
    }
  MetaObject::Clear();
  m_InterpolationType = MET_NO_INTERPOLATION;
  m_NControlPoints = 0;
  m_NInterpolatedPoints = 0;

  // Delete the list of control points.
  ControlPointListType::iterator it = m_ControlPointsList.begin();
  ControlPointListType::iterator itEnd = m_ControlPointsList.end();
  while(it != itEnd)
    {
    ContourControlPnt* pnt = *it;
    it++;
    delete pnt;
    }
  m_ControlPointsList.clear();

  // Delete the list of interpolated points
  InterpolatedPointListType::iterator itInterpolated =
                                             m_InterpolatedPointsList.begin();
  InterpolatedPointListType::iterator itInterpolatedEnd =
                                             m_InterpolatedPointsList.end();
  while(itInterpolated != itInterpolatedEnd)
    {
    ContourInterpolatedPnt* pnt = *itInterpolated;
    itInterpolated++;
    delete pnt;
    }
  m_InterpolatedPointsList.clear();

  strcpy(m_ControlPointDim, "id x y z xp yp zp nx ny nz r g b a");
  strcpy(m_InterpolatedPointDim, "id x y z r g b a");
  m_Closed = false;
  m_DisplayOrientation = -1;
  m_AttachedToSlice = -1;
}

/** Destroy Contour information */
void MetaContour::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set if the contour is pinned to a particulare slice */
void MetaContour::
AttachedToSlice(long int slice)
{
  m_AttachedToSlice = slice;
}

/** Get if the contour is pinned to a particulare slice */
long int MetaContour::
AttachedToSlice() const
{
  return m_AttachedToSlice;
}

/** Get the orientation of the display */
void MetaContour::
DisplayOrientation(int display)
{
  m_DisplayOrientation = display;
}

int MetaContour::
DisplayOrientation() const
{
  return m_DisplayOrientation;
}

/** Set Read fields */
void MetaContour::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour: M_SetupReadFields" << METAIO_STREAM::endl;

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Closed", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PinToSlice", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "DisplayOrientation", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ControlPointDim", MET_STRING, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NControlPoints", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ControlPoints", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}

void MetaContour::
M_SetupWriteFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour: M_SetupWriteFields" << METAIO_STREAM::endl;

  strcpy(m_ObjectTypeName,"Contour");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Closed", MET_INT, m_Closed);
  m_Fields.push_back(mF);

  if(m_AttachedToSlice != -1)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PinToSlice", MET_INT, m_AttachedToSlice);
    m_Fields.push_back(mF);
    }

  if(m_DisplayOrientation != -1)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "DisplayOrientation", MET_INT, m_DisplayOrientation);
    m_Fields.push_back(mF);
    }

  if(strlen(m_ControlPointDim)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ControlPointDim", MET_STRING,
                           strlen(m_ControlPointDim),m_ControlPointDim);
    m_Fields.push_back(mF);
    }

  m_NControlPoints = static_cast<int>(m_ControlPointsList.size());
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NControlPoints", MET_INT,m_NControlPoints);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ControlPoints", MET_NONE);
  m_Fields.push_back(mF);
}


bool MetaContour::
M_Read(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour: M_Read: Loading Header" << METAIO_STREAM::endl;

  if(!MetaObject::M_Read())
  {
    METAIO_STREAM::cout << "MetaContour: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  if(META_DEBUG) METAIO_STREAM::cout << "MetaContour: M_Read: Parsing Header" << METAIO_STREAM::endl;

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("Closed", &m_Fields);
  if(mF->defined)
    {
    if(mF->value[0])
      {
      m_Closed = true;
      }
    }

  mF = MET_GetFieldRecord("DisplayOrientation", &m_Fields);
  if(mF->defined)
    {
    if(mF->value[0])
      {
      m_DisplayOrientation = (int)mF->value[0];
      }
    }

  mF = MET_GetFieldRecord("PinToSlice", &m_Fields);
  if(mF->defined)
    {
    if(mF->value[0])
      {
      m_AttachedToSlice = (long int)mF->value[0];
      }
    }

  mF = MET_GetFieldRecord("NControlPoints", &m_Fields);
  if(mF->defined)
    {
    m_NControlPoints= (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("ControlPointDim", &m_Fields);
  if(mF->defined)
    {
    strcpy(m_ControlPointDim,(char *)(mF->value));
    }

  int pntDim;
  char** pntVal = NULL;
  MET_StringToWordArray(m_ControlPointDim, &pntDim, &pntVal);

  int i;
  for(i=0;i<pntDim;i++)
    {
    delete [] pntVal[i];
    }
  delete [] pntVal;

  if(m_BinaryData)
    {
    int readSize = m_NControlPoints*pntDim*4;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if(gc != readSize)
      {
      METAIO_STREAM::cout << "MetaContour: m_Read: data not read completely"
                          << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc
                          << METAIO_STREAM::endl;
      delete [] _data;
      return false;
      }

    int d;
    i = 0;
    unsigned int k;
    for(int j=0; j<m_NControlPoints; j++)
      {
      ContourControlPnt* pnt = new ContourControlPnt(m_NDims);

        {
        int id;
        char * const num = (char *)(&id);
        for(k=0;k<sizeof(int);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&id,MET_INT);
        i+=sizeof(int);
        pnt->m_Id = id;
        }

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
        pnt->m_X[d] = td;
        }

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
        pnt->m_XPicked[d] = td;
        }

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
        pnt->m_V[d] = td;
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
        pnt->m_Color[d] = td;
        }
      m_ControlPointsList.push_back(pnt);
      }
    delete [] _data;
    }
  else
    {
    float* v = new float[pntDim];
    // Ensure that there is enough space.
    assert(pntDim >= 1 + m_NDims * 3 + 4);

    for(int j=0; j<m_NControlPoints; j++)
      {
      ContourControlPnt* pnt = new ContourControlPnt(m_NDims);

      for(int k=0; k<pntDim; k++)
        {
        *m_ReadStream >> v[k];
        m_ReadStream->get(); // char c =
        }

      unsigned long pos = 0;
      pnt->m_Id = (unsigned long)v[pos];
      pos++;

      int d;
      for(d=0; d<m_NDims; d++)
        {
        pnt->m_X[d] = v[pos];
        pos++;
        }

      for(d=0; d<m_NDims; d++)
        {
        pnt->m_XPicked[d] = v[pos];
        pos++;
        }

      for(d=0; d<m_NDims; d++)
        {
        pnt->m_V[d] = v[pos];
        pos++;
        }

      for(d=0; d<4; d++)
        {
        pnt->m_Color[d] = v[pos];
        pos++;
        }

      m_ControlPointsList.push_back(pnt);
      }

    delete [] v;

    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
      {
      c = static_cast<char>(m_ReadStream->get());// to avoid unrecognize charactere
      }
    }

  // Read the interpolated point
  this->ClearFields();
  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Interpolation", MET_STRING, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
  MET_Read(*m_ReadStream,&m_Fields,'=',false,false);

  mF = MET_GetFieldRecord("Interpolation", &m_Fields);
  if(mF && mF->defined)
    {
    MET_StringToInterpolationType((char *)mF->value, &m_InterpolationType);
    }

  // Only read points if explicit interpolation
  if(m_InterpolationType == MET_EXPLICIT_INTERPOLATION)
    {
    this->ClearFields();
    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "InterpolatedPointDim", MET_STRING, true);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "NInterpolatedPoints", MET_INT, true);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitReadField(mF, "InterpolatedPoints", MET_NONE, true);
    mF->terminateRead = true;
    m_Fields.push_back(mF);

    MET_Read(*m_ReadStream,&m_Fields);

    mF = MET_GetFieldRecord("NInterpolatedPoints", &m_Fields);
    if(mF->defined)
      {
      m_NInterpolatedPoints= (int)mF->value[0];
      }

    mF = MET_GetFieldRecord("InterpolatedPointDim", &m_Fields);
    if(mF->defined)
      {
      strcpy(m_InterpolatedPointDim,(char *)(mF->value));
      }

    MET_StringToWordArray(m_InterpolatedPointDim, &pntDim, &pntVal);

    for(i=0;i<pntDim;i++)
      {
      delete [] pntVal[i];
      }
    delete [] pntVal;

    if(m_BinaryData)
      {
      int readSize = m_NInterpolatedPoints*pntDim*4;

      char* _data = new char[readSize];
      m_ReadStream->read((char *)_data, readSize);

      int gc = static_cast<int>(m_ReadStream->gcount());
      if(gc != readSize)
        {
        METAIO_STREAM::cout << "MetaContour: m_Read: data not read completely"
                            << METAIO_STREAM::endl;
        METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc
                            << METAIO_STREAM::endl;
        delete [] _data;
        return false;
        }

      int d;
      unsigned int k;
      i=0;
      for(int j=0; j<m_NInterpolatedPoints; j++)
        {
        ContourInterpolatedPnt* pnt = new ContourInterpolatedPnt(m_NDims);

          {
          int id;
          char * const num = (char *)(&id);
          for(k=0;k<sizeof(int);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&id,MET_INT);
          i+=sizeof(int);
          pnt->m_Id = id;
          }

        for(d=0; d<m_NDims; d++)
          {
          float x;
          char * const num = (char *)(&x);
          for(k=0;k<sizeof(float);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
          i+=sizeof(float);
          pnt->m_X[d] = x;
          }

        for(d=0; d<4; d++)
          {
          float x;
          char * const num = (char *)(&x);
          for(k=0;k<sizeof(float);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
          i+=sizeof(float);
          pnt->m_Color[d] = x;
          }
        m_InterpolatedPointsList.push_back(pnt);
        }
      delete [] _data;
      }
    else
      {
      float* v = new float[pntDim];
      // Ensure that there is enough space.
      assert(pntDim >= 1 + m_NDims + 4);

      for(int j=0; j<m_NInterpolatedPoints; j++)
        {
        ContourInterpolatedPnt* pnt = new ContourInterpolatedPnt(m_NDims);

        for(int k=0; k<pntDim; k++)
          {
          *m_ReadStream >> v[k];
          m_ReadStream->get(); // char c =
          }

        unsigned long pos = 0;
        pnt->m_Id = (unsigned long)v[pos];
        pos++;

        int d;
        for(d=0; d<m_NDims; d++)
          {
          pnt->m_X[d] = v[pos];
          pos++;
          }

        for(d=0; d<4; d++)
          {
          pnt->m_Color[d] = v[pos];
          pos++;
          }

        m_InterpolatedPointsList.push_back(pnt);
        }

      delete [] v;

      char c = ' ';
      while( (c!='\n') && (!m_ReadStream->eof()))
        {
        c = static_cast<char>(m_ReadStream->get());// to avoid unrecognize charactere
        }
      }
    }

  return true;
}


bool MetaContour::
M_Write(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaContour: M_Write" << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaContour: M_Read: Error parsing file"
                        << METAIO_STREAM::endl;
    return false;
    }

  /** Then write the control points */
  if(m_BinaryData)
    {
    ControlPointListType::const_iterator it = m_ControlPointsList.begin();
    ControlPointListType::const_iterator itEnd = m_ControlPointsList.end();

    char* data = new char[(m_NDims*3+5)*m_NControlPoints*4];
    int i=0;
    int d;
    while(it != itEnd)
      {
      unsigned int id = (*it)->m_Id;
      MET_SwapByteIfSystemMSB(&id,MET_UINT);
      MET_DoubleToValue((double)id,MET_UINT,data,i++);

      for(d = 0; d < m_NDims; d++)
        {
        float pntX = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&pntX,MET_FLOAT);
        MET_DoubleToValue((double)pntX,MET_FLOAT,data,i++);
        }

      for(d = 0; d < m_NDims; d++)
        {
        float pntX = (*it)->m_XPicked[d];
        MET_SwapByteIfSystemMSB(&pntX,MET_FLOAT);
        MET_DoubleToValue((double)pntX,MET_FLOAT,data,i++);
        }

      for(d = 0; d < m_NDims; d++)
        {
        float pntX = (*it)->m_V[d];
        MET_SwapByteIfSystemMSB(&pntX,MET_FLOAT);
        MET_DoubleToValue((double)pntX,MET_FLOAT,data,i++);
        }

      for(d=0; d<4; d++)
        {
        float pntX = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&pntX,MET_FLOAT);
        MET_DoubleToValue((double)pntX,MET_FLOAT,data,i++);
        }
      it++;
      }

    m_WriteStream->write((char *)data,(m_NDims*3+5)*m_NControlPoints*4);
    m_WriteStream->write("\n",1);
    delete [] data;
    }
  else
    {
    ControlPointListType::const_iterator it = m_ControlPointsList.begin();
    ControlPointListType::const_iterator itEnd = m_ControlPointsList.end();

    int d;
    while(it != itEnd)
      {
      *m_WriteStream << (*it)->m_Id << " ";

      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_X[d] << " ";
        }

      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_XPicked[d] << " ";
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

  this->ClearFields();
  MET_FieldRecordType * mF;

  if(m_InterpolationType != MET_NO_INTERPOLATION)
    {
    char s[255];
    mF = new MET_FieldRecordType;
    strcpy(s, MET_InterpolationTypeName[m_InterpolationType]);
    MET_InitWriteField(mF, "Interpolation", MET_STRING, strlen(s), s);
    m_Fields.push_back(mF);
    }

  m_NInterpolatedPoints = static_cast<int>(m_InterpolatedPointsList.size());
  if(m_NInterpolatedPoints>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "InterpolatedPointDim", MET_STRING,
                           strlen(m_InterpolatedPointDim),m_InterpolatedPointDim);
    m_Fields.push_back(mF);

    m_NInterpolatedPoints = static_cast<int>(m_InterpolatedPointsList.size());
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "NInterpolatedPoints", MET_INT,m_NInterpolatedPoints);
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "InterpolatedPoints", MET_NONE);
    m_Fields.push_back(mF);
    }

  MET_Write(*m_WriteStream,&m_Fields);

  if(m_BinaryData)
    {
    InterpolatedPointListType::const_iterator it =
                                              m_InterpolatedPointsList.begin();
    InterpolatedPointListType::const_iterator itEnd =
                                              m_InterpolatedPointsList.end();

    char* data = new char[(m_NDims+5)*m_NInterpolatedPoints*4];
    int i=0;
    int d;
    while(it != itEnd)
      {
      unsigned int id = (*it)->m_Id;
      MET_SwapByteIfSystemMSB(&id,MET_UINT);
      MET_DoubleToValue((double)id,MET_UINT,data,i++);

      for(d = 0; d < m_NDims; d++)
        {
        float x = (*it)->m_X[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,MET_FLOAT,data,i++);
        }
      for(d=0; d<4; d++)
        {
        float x = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,MET_FLOAT,data,i++);
        }
      it++;
      }

    m_WriteStream->write((char *)data,(m_NDims+5)*m_NInterpolatedPoints*4);
    m_WriteStream->write("\n",1);
    delete [] data;
    }
  else
    {
    InterpolatedPointListType::const_iterator it =
                                              m_InterpolatedPointsList.begin();
    InterpolatedPointListType::const_iterator itEnd =
                                              m_InterpolatedPointsList.end();

    int d;
    while(it != itEnd)
      {
      *m_WriteStream << (*it)->m_Id << " ";

      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_X[d] << " ";
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
