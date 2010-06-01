/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTubeGraph.h"

#ifdef _MSC_VER
#pragma warning(disable:4702)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

/** MetaTubeGraph Constructors */
MetaTubeGraph::
MetaTubeGraph()
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph()" << METAIO_STREAM::endl;
    }
  Clear();
}


MetaTubeGraph::
MetaTubeGraph(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph()" << METAIO_STREAM::endl;
    }
  Clear();
  Read(_headerName);
}


MetaTubeGraph::
MetaTubeGraph(const MetaTubeGraph *_tube)
:MetaObject()
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph()" << METAIO_STREAM::endl;
    }
  Clear();
  CopyInfo(_tube);
}


MetaTubeGraph::
MetaTubeGraph(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph()" << METAIO_STREAM::endl;
    }
  Clear();
}

/** Destructor */
MetaTubeGraph::
~MetaTubeGraph()
{
  // Delete the list of pointers to tubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
    {
    TubeGraphPnt* pnt = *it;
    it++;
    delete pnt;
    }  
  m_PointList.clear();
  M_Destroy();
}

//
void MetaTubeGraph::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "Root = " << m_Root << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "PointDim = " << m_PointDim << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaTubeGraph::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

    

void MetaTubeGraph::
PointDim(const char* pointDim)
{
  strcpy(m_PointDim,pointDim);
}
    
const char* MetaTubeGraph::
PointDim(void) const
{
  return m_PointDim;
}

void MetaTubeGraph::
NPoints(int npnt)
{
  m_NPoints = npnt;
}

int MetaTubeGraph::
NPoints(void) const
{
  return m_NPoints;
}

void MetaTubeGraph::
Root(int root)
{
  m_Root = root;
}
    
int MetaTubeGraph:: 
Root(void) const
{
  return m_Root;
}


/** Clear tube information */
void MetaTubeGraph::
Clear(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph: Clear" << METAIO_STREAM::endl;
    }
  MetaObject::Clear();
  // Delete the list of pointers to tubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    TubeGraphPnt* pnt = *it;
    it++;
    delete pnt;
  }  
  m_PointList.clear();

  m_Root = 0;
  m_NPoints = 0;
  strcpy(m_PointDim, "Node r p txx txy txz tyx tyy tyz tzx tzy tzz");
  m_ElementType = MET_FLOAT;
}
        
/** Destroy tube information */
void MetaTubeGraph::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaTubeGraph::
M_SetupReadFields(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph: M_SetupReadFields" 
                        << METAIO_STREAM::endl;
    }

  MetaObject::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Root", MET_INT, false);
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

void MetaTubeGraph::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"TubeGraph");
  MetaObject::M_SetupWriteFields();

  MET_FieldRecordType * mF;
  FieldsContainerType::iterator it;

  mF = MET_GetFieldRecord("TransformMatrix",&m_Fields);
  it = m_Fields.begin();
  while(it != m_Fields.end())
    {
    if(*it == mF)
      {
      m_Fields.erase(it);
      break;
      }
    it++;
    }

  mF = MET_GetFieldRecord("Offset",&m_Fields);
  it = m_Fields.begin();
  while(it != m_Fields.end())
    {
    if(*it == mF)
      {
      m_Fields.erase(it);
      break;
      }
    it++;
    }

  mF = MET_GetFieldRecord("ElementSpacing",&m_Fields);
  it = m_Fields.begin();
  while(it != m_Fields.end())
    {
    if(*it == mF)
      {
      m_Fields.erase(it);
      break;
      }
    it++;
    }

  mF = MET_GetFieldRecord("CenterOfRotation",&m_Fields);
  it = m_Fields.begin();
  while(it != m_Fields.end())
    {
    if(*it == mF)
      {
      m_Fields.erase(it);
      break;
      }
    it++;
    }

  if(m_Root>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Root", MET_INT,m_Root);
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



bool MetaTubeGraph::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph: M_Read: Loading Header" 
                        << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaTubeGraph: M_Read: Error parsing file" 
                        << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph: M_Read: Parsing Header" 
                        << METAIO_STREAM::endl;
    }
 
  MET_FieldRecordType * mF;
 
  mF = MET_GetFieldRecord("Root", &m_Fields);
  if(mF->defined)
    {
    m_Root= (int)mF->value[0];
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


  int i;
  int posR = -1;
  int posP = -1;
  int posTx = -1;
  int posGraphNode = -1;

  int pntDim;
  char** pntVal = NULL;
  MET_StringToWordArray(m_PointDim, &pntDim, &pntVal); 
 
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTubeGraph: Parsing point dim" 
                        << METAIO_STREAM::endl;
    }

  unsigned int j;
  for(j = 0; j < (unsigned int)pntDim; j++) 
    {
    if(!strcmp(pntVal[j], "node") || !strcmp(pntVal[j], "Node"))
      {
      posGraphNode = j;
      }
    if(!strcmp(pntVal[j], "s") || !strcmp(pntVal[j], "S") ||
      !strcmp(pntVal[j], "r") || !strcmp(pntVal[j], "R") ||
      !strcmp(pntVal[j], "rad") || !strcmp(pntVal[j], "Rad") ||
      !strcmp(pntVal[j], "radius") || !strcmp(pntVal[j], "Radius"))
      {
      posR = j;
      }
    if(!strcmp(pntVal[j], "p") || !strcmp(pntVal[j], "P"))
      {
      posP = j;
      }
    if(!strcmp(pntVal[j], "txx"))
      {
      posTx = j;
      }
    }

  for(i=0;i<pntDim;i++)
    {
    delete [] pntVal[i];
    }
  delete [] pntVal;

  float v[30];
  
  if(m_Event)
    {
    m_Event->StartReading(m_NPoints);
    }

  if(m_BinaryData)
    {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int readSize = m_NPoints*pntDim*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = m_ReadStream->gcount();
    if(gc != readSize)
      {
      METAIO_STREAM::cout << "MetaLine: m_Read: data not read completely" 
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize << " : actual = " << gc
                << METAIO_STREAM::endl;
      return false;
      }

    double td;
    for(j=0; j<(unsigned int)m_NPoints; j++) 
      {
      TubeGraphPnt* pnt = new TubeGraphPnt(m_NDims);
      
      MET_ValueToDouble(m_ElementType,_data,posGraphNode,&td);
      pnt->m_GraphNode=(int)td;

      if(posR != -1)
        {
        MET_ValueToDouble(m_ElementType, _data, posR, &td);
        pnt->m_R = (float)td;
        }

      if(posP != -1)
        {
        MET_ValueToDouble(m_ElementType, _data, posP, &td);
        pnt->m_P = (float)td;
        }

      if(posTx != -1)
        {
        for(int r=0; r<m_NDims*m_NDims; r++)
          {
          MET_ValueToDouble(m_ElementType, _data, posTx+r, &td);
          pnt->m_T[r] = (float)td;
          }
        }
      m_PointList.push_back(pnt);
      }
    delete [] _data;
    }
  else
    {
    for(j=0; j<(unsigned int)m_NPoints; j++) 
      {
      if(m_Event)
        {
        m_Event->SetCurrentIteration(j+1);
        }

     TubeGraphPnt* pnt = new TubeGraphPnt(m_NDims);

     for(int k=0; k<pntDim; k++)
       {
       *m_ReadStream >> v[k];
       m_ReadStream->get();
       }

     pnt->m_GraphNode = (int)v[posGraphNode];

     if(posR != -1)
       {
       pnt->m_R = v[posR];
       }

     if(posP != -1)
       {
       pnt->m_P = v[posP];
       }

      if(posTx >= 0 && posTx<pntDim) 
        {
        for(int r=0; r<m_NDims*m_NDims; r++)
          {
          pnt->m_T[r] = v[posTx+r];
          }
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

  return true;
}

MET_ValueEnumType MetaTubeGraph::
ElementType(void) const
{
  return m_ElementType;
}

void MetaTubeGraph::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}

bool MetaTubeGraph::
M_Write(void)
{

  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaTubeGraph: M_Read: Error parsing file" 
                        << METAIO_STREAM::endl;
    return false;
    }

  /** Then copy all tubes points */
  if(m_BinaryData)
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    char* data = new char[(m_NDims*m_NDims+3)*m_NPoints*elementSize];
    int i=0;
    int d;
    while(it != itEnd)
      {
      MET_DoubleToValue((double)(*it)->m_GraphNode,m_ElementType,data,i++);

      MET_DoubleToValue((double)(*it)->m_R,m_ElementType,data,i++);  

      MET_DoubleToValue((double)(*it)->m_P,m_ElementType,data,i++);  
      
      for(d = 0; d < m_NDims*m_NDims; d++)
        {
        MET_DoubleToValue((double)(*it)->m_T[d],m_ElementType,data,i++);  
        }

      it++;
      }

    m_WriteStream->write((char *)data,
                         (m_NDims*m_NDims+3)*m_NPoints*elementSize);
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
      *m_WriteStream << (*it)->m_GraphNode << " ";

      *m_WriteStream << (*it)->m_R << " ";

      *m_WriteStream << (*it)->m_P << " ";

      for(d = 0; d < m_NDims*m_NDims; d++)
        {
        *m_WriteStream << (*it)->m_T[d] << " ";
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

