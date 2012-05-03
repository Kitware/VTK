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

#include "metaVesselTube.h"

#include <stdio.h>
#include <ctype.h>
#include <string>


#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

VesselTubePnt::
VesselTubePnt(int dim)
{
  m_Dim = dim;
  m_X = new float[m_Dim];
  m_T = new float[m_Dim];
  m_V1= new float[m_Dim];
  m_V2= new float[m_Dim];
  for(unsigned int i=0;i<m_Dim;i++)
    {
    m_X[i] = 0;
    m_V1[i]= 0;
    m_V2[i]= 0;
    m_T[i]= 0;
    }
  m_Alpha1=0;
  m_Alpha2=0;
  m_Alpha3=0;
  m_R=0;
  m_Medialness=0;
  m_Ridgeness=0;
  m_Branchness=0;
  m_Mark=false;

  //Color is red by default
  m_Color[0]=1.0f;
  m_Color[1]=0.0f;
  m_Color[2]=0.0f;
  m_Color[3]=1.0f;
  m_ID = -1;
}

VesselTubePnt::
~VesselTubePnt()
{
  delete []m_X;
  delete []m_V1;
  delete []m_V2;
  delete []m_T;
}

/** MetaVesselTube Constructors */
MetaVesselTube::
MetaVesselTube()
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube()" << METAIO_STREAM::endl;
    }
  Clear();
}


MetaVesselTube::
MetaVesselTube(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube()" << METAIO_STREAM::endl;
    }
  Clear();
  Read(_headerName);
}


MetaVesselTube::
MetaVesselTube(const MetaVesselTube *_VesselTube)
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube()" << METAIO_STREAM::endl;
    }
  Clear();
  CopyInfo(_VesselTube);
}


MetaVesselTube::
MetaVesselTube(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube()" << METAIO_STREAM::endl;
    }
  Clear();
}

/** Destructor */
MetaVesselTube::
~MetaVesselTube()
{
  // Delete the list of pointers to VesselTubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
    {
    VesselTubePnt* pnt = *it;
    it++;
    delete pnt;
    }
  m_PointList.clear();
  M_Destroy();
}

//
void MetaVesselTube::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "ParentPoint = " << m_ParentPoint
                      << METAIO_STREAM::endl;
  if(m_Root)
    {
    METAIO_STREAM::cout << "Root = " << "True" << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "Root = " << "False" << METAIO_STREAM::endl;
    }
  METAIO_STREAM::cout << "Artery = " << m_Artery << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "PointDim = " << m_PointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaVesselTube::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}



void MetaVesselTube::
PointDim(const char* pointDim)
{
  strcpy(m_PointDim,pointDim);
}

const char* MetaVesselTube::
PointDim(void) const
{
  return m_PointDim;
}

void MetaVesselTube::
NPoints(int npnt)
{
  m_NPoints = npnt;
}

int MetaVesselTube::
NPoints(void) const
{
  return m_NPoints;
}

void MetaVesselTube::
Root(bool root)
{
  m_Root = root;
}

bool MetaVesselTube::
Root(void) const
{
  return m_Root;
}


void MetaVesselTube::
Artery(bool artery)
{
  m_Artery = artery;
}

bool MetaVesselTube::
Artery(void) const
{
  return m_Artery;
}


void  MetaVesselTube::
ParentPoint(int parentpoint)
{
  m_ParentPoint = parentpoint;
}

int MetaVesselTube::
ParentPoint(void) const
{
  return m_ParentPoint;
}

/** Clear VesselTube information */
void MetaVesselTube::
Clear(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube: Clear" << METAIO_STREAM::endl;
    }
  MetaObject::Clear();
  // Delete the list of pointers to VesselTubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
    {
    VesselTubePnt* pnt = *it;
    it++;
    delete pnt;
    }
  m_PointList.clear();

  m_ParentPoint= -1;
  m_Root = false;
  m_Artery = true;
  m_NPoints = 0;
  strcpy(m_PointDim, "x y z r rn mn bn mk v1x v1y v1z v2x v2y v2z tx ty tz a1 a2 a3 red green blue alpha id");
  m_ElementType = MET_FLOAT;
}

/** Destroy VesselTube information */
void MetaVesselTube::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaVesselTube::
M_SetupReadFields(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube: M_SetupReadFields"
                        << METAIO_STREAM::endl;
    }

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  // int nDimsRecNum = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ParentPoint", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Root", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Artery", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "PointDim", MET_STRING, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NPoints", MET_INT, true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Points", MET_NONE, true);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

}

void MetaVesselTube::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Tube");
  strcpy(m_ObjectSubTypeName,"Vessel");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  if(m_ParentPoint>=0 && m_ParentID>=0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ParentPoint", MET_INT,m_ParentPoint);
    m_Fields.push_back(mF);
    }

  if(m_Root)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Root", MET_STRING, strlen("True"), "True");
    m_Fields.push_back(mF);
    }
  else
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Root", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
    }

  if(m_Artery)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Artery", MET_STRING, strlen("True"), "True");
    m_Fields.push_back(mF);
    }
  else
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Artery", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
    }

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



bool MetaVesselTube::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube: M_Read: Loading Header"
                        << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaVesselTube: M_Read: Error parsing file"
                        << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube: M_Read: Parsing Header"
                        << METAIO_STREAM::endl;
    }

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("ParentPoint", &m_Fields);
  if(mF->defined)
    {
    m_ParentPoint= (int)mF->value[0];
    }

  m_Root = false;
  mF = MET_GetFieldRecord("Root", &m_Fields);
  if(mF->defined)
    {
    if(*((char *)(mF->value)) == 'T'
       || *((char*)(mF->value)) == 't'
       || *((char*)(mF->value)) == '1')
      {
      m_Root = true;
      }
    else
      {
      m_Root = false;
      }
    }

  m_Artery = true;
  mF = MET_GetFieldRecord("Artery", &m_Fields);
  if(mF->defined)
    {
    if(*((char *)(mF->value)) == 'T' || *((char*)(mF->value)) == 't')
      {
      m_Artery = true;
      }
    else
      {
      m_Artery = false;
      }
    }

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if(mF->defined)
    {
    m_NPoints= (int)mF->value[0];
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
  int posR = -1;
  int posRn = -1;
  int posMn = -1;
  int posBn = -1;
  int posMk = -1;
  int posV1x = -1;
  int posV1y = -1;
  int posV1z = -1;
  int posV2x = -1;
  int posV2y = -1;
  int posV2z = -1;
  int posTx = -1;
  int posTy = -1;
  int posTz = -1;
  int posA1 = -1;
  int posA2 = -1;
  int posA3 = -1;
  int posRed = -1;
  int posGreen = -1;
  int posBlue = -1;
  int posAlpha = -1;
  int posID = -1;

  int pntDim;
  char** pntVal = NULL;
  MET_StringToWordArray(m_PointDim, &pntDim, &pntVal);

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaVesselTube: Parsing point dim"
                        << METAIO_STREAM::endl;
    }

  int j;
  for(j = 0; j < pntDim; j++)
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
    if(((char *)pntVal[j])[0] == 'w' || ((char *)pntVal[j])[0] == 'W')
      {
      posDim[(int)pntVal[j][1]+3] = j;
      }
    if(!strcmp(pntVal[j], "s") || !strcmp(pntVal[j], "S") ||
      !strcmp(pntVal[j], "r") || !strcmp(pntVal[j], "R") ||
      !strcmp(pntVal[j], "rad") || !strcmp(pntVal[j], "Rad") ||
      !strcmp(pntVal[j], "radius") || !strcmp(pntVal[j], "Radius"))
      {
      posR = j;
      }

    if(!strcmp(pntVal[j], "rn") || !strcmp(pntVal[j], "RN"))
      {
      posRn = j;
      }
    if(!strcmp(pntVal[j], "mn") || !strcmp(pntVal[j], "MN"))
      {
      posMn = j;
      }
    if(!strcmp(pntVal[j], "bn") || !strcmp(pntVal[j], "BN"))
      {
      posBn = j;
      }
    if(!strcmp(pntVal[j], "mk") || !strcmp(pntVal[j], "MK"))
      {
      posMk = j;
      }
    if(!strcmp(pntVal[j], "v1x"))
      {
      posV1x = j;
      }
    if(!strcmp(pntVal[j], "v1y"))
      {
      posV1y = j;
      }
    if(!strcmp(pntVal[j], "v1z"))
      {
      posV1z = j;
      }
    if(!strcmp(pntVal[j], "v2x"))
      {
      posV2x = j;
      }
    if(!strcmp(pntVal[j], "v2y"))
      {
      posV2y = j;
      }
    if(!strcmp(pntVal[j], "v2z"))
      {
      posV2z = j;
      }
    if(!strcmp(pntVal[j], "tx"))
      {
      posTx = j;
      }
    if(!strcmp(pntVal[j], "ty"))
      {
      posTy = j;
      }
    if(!strcmp(pntVal[j], "tz"))
      {
      posTz = j;
      }
    if(!strcmp(pntVal[j], "a1"))
      {
      posA1 = j;
      }
    if(!strcmp(pntVal[j], "a2"))
      {
      posA2 = j;
      }
    if(!strcmp(pntVal[j], "a3"))
      {
      posA3 = j;
      }

    if(!strcmp(pntVal[j], "red"))
      {
      posRed = j;
      }
    if(!strcmp(pntVal[j], "green"))
      {
      posGreen = j;
      }

    if(!strcmp(pntVal[j], "blue"))
      {
      posBlue = j;
      }
    if(!strcmp(pntVal[j], "alpha"))
      {
      posAlpha = j;
      }
    if(!strcmp(pntVal[j], "id") || !strcmp(pntVal[j], "ID"))
      {
      posID = j;
      }
    }

  for(i=0;i<pntDim;i++)
    {
    delete [] pntVal[i];
    }
  delete [] pntVal;

  float v[50];

  if(m_Event)
    {
    m_Event->StartReading(m_NPoints);
    }

  if(m_BinaryData)
    {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int readSize = m_NPoints*(m_NDims*(2+m_NDims)+10)*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = m_ReadStream->gcount();
    if(gc != readSize)
      {
      METAIO_STREAM::cout << "MetaLine: m_Read: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize
                << " : actual = " << gc << METAIO_STREAM::endl;
      delete [] _data;
      delete [] posDim;
      return false;
      }

    i=0;
    int d;
    unsigned int k;
    for(j=0; j<(int)m_NPoints; j++)
      {
      VesselTubePnt* pnt = new VesselTubePnt(m_NDims);

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

        {
        float td;
        char * num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_R = (float)td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Ridgeness = td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Medialness = (float)td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Branchness = (float)td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);

        if((float)td == 1.0)
          {
          pnt->m_Mark = true;
          }
        else
          {
          pnt->m_Mark = false;
          }
        }

      for(d = 0; d < m_NDims; d++)
        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_V1[d] = (float)td;
        }

      if(m_NDims==3)
        {
        for(d = 0; d < m_NDims; d++)
          {
          float td;
          char * const num = (char *)(&td);
          for(k=0;k<sizeof(float);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
          i+=sizeof(float);
          pnt->m_V2[d] = (float)td;
          }
        }

      for(d = 0; d < m_NDims; d++)
        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_T[d] = (float)td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Alpha1 = (float)td;
        }

        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Alpha2 = (float)td;
        }

      if(m_NDims>=3)
        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_Alpha3 = (float)td;
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

        {
        int id;
        char * const num = (char *)(&id);
        for(k=0;k<sizeof(int);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&id,MET_FLOAT);
        i+=sizeof(int);
        pnt->m_ID=id;
        }

      m_PointList.push_back(pnt);
      }
    delete [] _data;
    }
  else
    {
    for(j=0; j<(int)m_NPoints; j++)
      {
      if(m_Event)
        {
        m_Event->SetCurrentIteration(j+1);
        }

      VesselTubePnt* pnt = new VesselTubePnt(m_NDims);

      for(int k=0; k<pntDim; k++)
        {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
        }

      for(int d=0; d<m_NDims; d++)
        {
        pnt->m_X[d] = v[posDim[d]];
        }

      pnt->m_R = v[posR];

      if(posMn >= (int)0 && posMn < pntDim)
        {
        pnt->m_Medialness = v[posMn];
        }

      if(posRn >= (int)0 && posRn < pntDim)
        {
        pnt->m_Ridgeness = v[posRn];
        }

      if(posBn >= (int)0 && posBn < pntDim)
        {
        pnt->m_Branchness = v[posBn];
        }

      if(posMk >= 0 && posMk < pntDim)
        {
        pnt->m_Mark = (v[posMk] > 0) ? true:false;
        }

      if(posV1x>=0 && posV1x<pntDim)
        {
        pnt->m_V1[0] = v[posV1x];
        if(posV1y >= 0 && posV1y<pntDim)
          {
          pnt->m_V1[1] = v[posV1y];
          }
        if(posV1z >= 0 && m_NDims>2 && posV1z<pntDim)
          {
          pnt->m_V1[2] = v[posV1z];
          }
        }
      if(posV2x >= 0 && posV2x<pntDim)
        {
        pnt->m_V2[0] = v[posV2x];
        if(posV2y >= 0 && posV2y<pntDim)
          {
          pnt->m_V2[1] = v[posV2y];
          }
        if(posV2z >= 0 && m_NDims>2 && posV2z<pntDim)
          {
          pnt->m_V2[2] = v[posV2z];
          }
        }
      if(posTx >= 0 && posTx<pntDim)
        {
        pnt->m_T[0] = v[posTx];
        if(posTy >= 0 && posTy<pntDim)
          {
          pnt->m_T[1] = v[posTy];
          }
        if(posTz >= 0 && m_NDims>2 && posTz<pntDim)
          {
          pnt->m_T[2] = v[posTz];
          }
        }
      if(posA1 >= 0 && posA1<pntDim)
        {
        pnt->m_Alpha1 = v[posA1];
        }
      if(posA2 >= 0 && posA2<pntDim)
        {
        pnt->m_Alpha2 = v[posA2];
        }
      if(posA3 >= 0 && posA3<pntDim)
        {
        pnt->m_Alpha3 = v[posA3];
        }

      if(posRed >= 0 && posRed < pntDim)
        {
        pnt->m_Color[0] = v[posRed];
        }

      if(posGreen >= 0 && posGreen < pntDim)
        {
        pnt->m_Color[1] = v[posGreen];
        }

      if(posBlue >= 0 && posBlue < pntDim)
        {
        pnt->m_Color[2] = v[posBlue];
        }

      if(posAlpha >= 0 && posAlpha < pntDim)
        {
        pnt->m_Color[3] = v[posAlpha];
        }

      if(posID >= 0 && posID < pntDim)
        {
        pnt->m_ID = (int)v[posID];
        }

      m_PointList.push_back(pnt);
      }


    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
      {
      c = m_ReadStream->get();// to avoid unrecognize charactere
      }
    }

  if(m_Event)
    {
    m_Event->StopReading();
    }

  delete []posDim;
  return true;
}

MET_ValueEnumType MetaVesselTube::
ElementType(void) const
{
  return m_ElementType;
}

void MetaVesselTube::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}

bool MetaVesselTube::
M_Write(void)
{

  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaVesselTube: M_Read: Error parsing file"
                        << METAIO_STREAM::endl;
    return false;
    }

  /** Then copy all VesselTubes points */
  if(m_BinaryData)
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = new char[(m_NDims*(2+m_NDims)+10)*m_NPoints*elementSize];
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

      float x = (*it)->m_R;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);
      x = (*it)->m_Ridgeness;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);
      x = (*it)->m_Medialness;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);
      x = (*it)->m_Branchness;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);
      x = (*it)->m_Mark;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);

      MET_DoubleToValue((double)x,m_ElementType,data,i++);

      for(d = 0; d < m_NDims; d++)
        {
        x = (*it)->m_V1[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
        }

      if(m_NDims==3)
        {
        for(d = 0; d < m_NDims; d++)
          {
          x = (*it)->m_V2[d];
          MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
          MET_DoubleToValue((double)x,m_ElementType,data,i++);
          }
        }

      for(d = 0; d < m_NDims; d++)
        {
        x = (*it)->m_T[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
        }

      x = (*it)->m_Alpha1;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);
      x = (*it)->m_Alpha2;
      MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
      MET_DoubleToValue((double)x,m_ElementType,data,i++);

      if(m_NDims>=3)
        {
        x = (*it)->m_Alpha3;
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
        }

      for(d=0; d<4; d++)
        {
        x = (*it)->m_Color[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
        }

      int id = (*it)->m_ID;
      MET_SwapByteIfSystemMSB(&id,MET_INT);
      MET_DoubleToValue((double)id,m_ElementType,data,i++);

      it++;
      }

    m_WriteStream->write((char *)data,
                         (m_NDims*(2+m_NDims)+10)*m_NPoints*elementSize);
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

      *m_WriteStream << (*it)->m_R << " ";
      *m_WriteStream << (*it)->m_Ridgeness << " ";
      *m_WriteStream << (*it)->m_Medialness << " ";
      *m_WriteStream << (*it)->m_Branchness << " ";
      if((*it)->m_Mark)
        {
        *m_WriteStream << 1 << " ";
        }
      else
        {
        *m_WriteStream << 0 << " ";
        }

      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_V1[d] << " ";
        }

      if(m_NDims>=3)
        {
        for(d = 0; d < m_NDims; d++)
          {
          *m_WriteStream << (*it)->m_V2[d] << " ";
          }
        }

      for(d = 0; d < m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_T[d] << " ";
        }

      *m_WriteStream << (*it)->m_Alpha1 << " ";
      *m_WriteStream << (*it)->m_Alpha2 << " ";

      if(m_NDims>=3)
        {
        *m_WriteStream << (*it)->m_Alpha3 << " ";
        }

      for(d=0;d<4;d++)
        {
        *m_WriteStream << (*it)->m_Color[d] << " ";
        }

      *m_WriteStream << (*it)->m_ID << " ";

      *m_WriteStream << METAIO_STREAM::endl;
      it++;
      }
    }
  return true;

}

#if (METAIO_USE_NAMESPACE)
};
#endif

