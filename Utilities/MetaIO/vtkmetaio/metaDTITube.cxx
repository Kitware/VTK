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
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4702 )
#pragma warning ( disable : 4284 )
#endif

#include "metaDTITube.h"

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif

DTITubePnt::
DTITubePnt(int dim)
{
  m_Dim = dim;
  m_X = new float[m_Dim];
  m_TensorMatrix = new float[6];

  unsigned int i=0;
  for(i=0;i<m_Dim;i++)
    {
    m_X[i] = 0;
    }

  // Initialize the tensor matrix to identity
  for(i=0;i<6;i++)
    {
    m_TensorMatrix[i] = 0;
    }
  m_TensorMatrix[0] = 1;
  m_TensorMatrix[3] = 1;
  m_TensorMatrix[5] = 1;
}

DTITubePnt::
~DTITubePnt()
{
  delete []m_X;
  delete []m_TensorMatrix;
  m_ExtraFields.clear();
}

const DTITubePnt::FieldListType &
DTITubePnt::
GetExtraFields() const
{
  return m_ExtraFields;
}

void DTITubePnt::
AddField(const char* name, float value)
{
  FieldType field(name,value);
  m_ExtraFields.push_back(field);
}

float DTITubePnt::
GetField(const char* name) const
{
  FieldListType::const_iterator it = m_ExtraFields.begin();
  FieldListType::const_iterator itEnd = m_ExtraFields.end();
  while(it != itEnd)
    {
    if(!strcmp((*it).first.c_str(),name))
      {
      return (*it).second;
      }
    ++it;
    }
  return -1;
}


/** MetaDTITube Constructors */
MetaDTITube::
MetaDTITube()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaDTITube()" << METAIO_STREAM::endl;
  Clear();
}


MetaDTITube::
MetaDTITube(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaDTITube()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}


MetaDTITube::
MetaDTITube(const MetaDTITube *_DTITube)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaDTITube()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_DTITube);
}


MetaDTITube::
MetaDTITube(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaDTITube()" << METAIO_STREAM::endl;
  Clear();
}

/** Destructor */
MetaDTITube::
~MetaDTITube()
{
  // Delete the list of pointers to DTITubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    DTITubePnt* pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();
  M_Destroy();
}

//
void MetaDTITube::
PrintInfo() const
{
  MetaObject::PrintInfo();
  METAIO_STREAM::cout << "ParentPoint = " << m_ParentPoint << METAIO_STREAM::endl;
  if(m_Root)
    {
    METAIO_STREAM::cout << "Root = " << "True" << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "Root = " << "True" << METAIO_STREAM::endl;
    }
  METAIO_STREAM::cout << "PointDim = " << m_PointDim.c_str()
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NPoints = " << m_NPoints << METAIO_STREAM::endl;
  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;
}

void MetaDTITube::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}



void MetaDTITube::
PointDim(const char* pointDim)
{
  m_PointDim = pointDim;
}

const char* MetaDTITube::
PointDim(void) const
{
  return m_PointDim.c_str();
}

void MetaDTITube::
NPoints(int npnt)
{
  m_NPoints = npnt;
}

int MetaDTITube::
NPoints(void) const
{
  return m_NPoints;
}

void MetaDTITube::
Root(bool root)
{
  m_Root = root;
}

bool MetaDTITube::
Root(void) const
{
  return m_Root;
}


void  MetaDTITube::
ParentPoint(int parentpoint)
{
  m_ParentPoint = parentpoint;
}

int MetaDTITube::
ParentPoint(void) const
{
  return m_ParentPoint;
}

/** Clear DTITube information */
void MetaDTITube::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaDTITube: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  // Delete the list of pointers to DTITubes.
  PointListType::iterator it = m_PointList.begin();
  while(it != m_PointList.end())
  {
    DTITubePnt* pnt = *it;
    ++it;
    delete pnt;
  }
  m_PointList.clear();

  m_ParentPoint= -1;
  m_Root = false;
  m_NPoints = 0;
  m_PointDim = "x y z tensor1 tensor2 tensor3 tensor4 tensor5 tensor6";
  m_ElementType = MET_FLOAT;
}

/** Destroy DTITube information */
void MetaDTITube::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaDTITube::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaDTITube: M_SetupReadFields" << METAIO_STREAM::endl;

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

void MetaDTITube::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Tube");
  strcpy(m_ObjectSubTypeName,"DTI");
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

  // Create the new PointDim field
  m_PointDim = "x y z tensor1 tensor2 tensor3 tensor4 tensor5 tensor6";

  // All the points in the tube have the same number of fields
  const DTITubePnt::FieldListType & extraList =
                                    (*(m_PointList.begin()))->GetExtraFields();
  DTITubePnt::FieldListType::const_iterator itFields = extraList.begin();
  DTITubePnt::FieldListType::const_iterator itFieldsEnd = extraList.end();
  while(itFields !=  itFieldsEnd)
    {
    m_PointDim += " ";
    m_PointDim += (*itFields).first;
    ++itFields;
    }

  if(m_PointDim.size()>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "PointDim", MET_STRING,
                           m_PointDim.size(),m_PointDim.c_str());
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

/** Return the position given the name of the field */
int MetaDTITube::GetPosition(const char* name) const
{
  METAIO_STL::vector<PositionType>::const_iterator it = m_Positions.begin();
  METAIO_STL::vector<PositionType>::const_iterator itEnd = m_Positions.end();
  while(it != itEnd)
    {
    if(!strcmp((*it).first.c_str(),name))
      {
      return (*it).second;
      }
    ++it;
    }

  return -1;
}

bool MetaDTITube::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaDTITube: M_Read: Loading Header" << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaDTITube: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaDTITube: M_Read: Parsing Header" << METAIO_STREAM::endl;
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

  mF = MET_GetFieldRecord("NPoints", &m_Fields);
  if(mF->defined)
    {
    m_NPoints= (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("PointDim", &m_Fields);
  if(mF->defined)
    {
    m_PointDim = (char *)(mF->value);
    }

  int i;

  int pntDim;
  char** pntVal = NULL;
  char pointDim[255];

  for(unsigned t = 0;t<m_PointDim.size();t++)
    {
    pointDim[t] = m_PointDim[t];
    }
  pointDim[m_PointDim.size()] = '\0';

  MET_StringToWordArray(pointDim, &pntDim, &pntVal);

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaDTITube: Parsing point dim" << METAIO_STREAM::endl;
    }

  int j;
  m_Positions.clear();
  for(j = 0; j < pntDim; j++)
    {
    PositionType p(pntVal[j],j);
    m_Positions.push_back(p);
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
    int readSize = m_NPoints*pntDim*elementSize;

    char* _data = new char[readSize];
    m_ReadStream->read((char *)_data, readSize);

    int gc = static_cast<int>(m_ReadStream->gcount());
    if(gc != readSize)
      {
      METAIO_STREAM::cout << "MetaLine: m_Read: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << readSize
                << " : actual = " << gc << METAIO_STREAM::endl;
      delete [] _data;
      return false;
      }

    i=0;
    int d;
    unsigned int k;
    for(j=0; j<m_NPoints; j++)
      {
      DTITubePnt* pnt = new DTITubePnt(m_NDims);

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

      for(d=0; d<6; d++)
        {
        float td;
        char * const num = (char *)(&td);
        for(k=0;k<sizeof(float);k++)
          {
          num[k] = _data[i+k];
          }
        MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
        i+=sizeof(float);
        pnt->m_TensorMatrix[d] = (float)td;
        }

      METAIO_STL::vector<PositionType>::const_iterator itFields =
                                                           m_Positions.begin();
      METAIO_STL::vector<PositionType>::const_iterator itFieldsEnd =
                                                           m_Positions.end();
      while(itFields !=  itFieldsEnd)
        {
        if(strcmp((*itFields).first.c_str(),"x")
          && strcmp((*itFields).first.c_str(),"y")
          && strcmp((*itFields).first.c_str(),"z")
          && strcmp((*itFields).first.c_str(),"tensor1")
          && strcmp((*itFields).first.c_str(),"tensor2")
          && strcmp((*itFields).first.c_str(),"tensor3")
          && strcmp((*itFields).first.c_str(),"tensor4")
          && strcmp((*itFields).first.c_str(),"tensor5")
          && strcmp((*itFields).first.c_str(),"tensor6")
          )
          {
          float td;
          char * const num = (char *)(&td);
          for(k=0;k<sizeof(float);k++)
            {
            num[k] = _data[i+k];
            }
          MET_SwapByteIfSystemMSB(&td,MET_FLOAT);
          i+=sizeof(float);
          pnt->AddField((*itFields).first.c_str(),(float)td);
          }
        ++itFields;
        }

      m_PointList.push_back(pnt);
      }
    delete [] _data;
    }
  else
    {
    for(j=0; j<m_NPoints; j++)
      {
      if(m_Event)
        {
        m_Event->SetCurrentIteration(j+1);
        }

      DTITubePnt* pnt = new DTITubePnt(m_NDims);

      for(int k=0; k<pntDim; k++)
        {
        *m_ReadStream >> v[k];
        m_ReadStream->get();
        }


      const int positionOfX = this->GetPosition("x");
      const int positionOfY = this->GetPosition("y");

      if( positionOfX < 0 )
        {
        METAIO_STREAM::cerr << "MetaDTITube: M_Read: 'x' not found." << METAIO_STREAM::endl;
        }

      if( positionOfY < 0 )
        {
        METAIO_STREAM::cerr << "MetaDTITube: M_Read: 'y' not found." << METAIO_STREAM::endl;
        }

      pnt->m_X[0] = v[positionOfX];
      pnt->m_X[1] = v[positionOfY];

      if(m_NDims == 3)
        {
        const int positionOfZ = this->GetPosition("z");

        if( positionOfZ < 0 )
          {
          METAIO_STREAM::cerr << "MetaDTITube: M_Read: 'z' not found." << METAIO_STREAM::endl;
          }

        pnt->m_X[2] = v[positionOfZ];
        }

      // Read tensor1
      if(this->GetPosition("tensor1") >= 0
         && this->GetPosition("tensor1") < pntDim)
        {
        pnt->m_TensorMatrix[0] = v[this->GetPosition("tensor1")];
        }
      // Read tensor2
      if(this->GetPosition("tensor2") >= 0
         && this->GetPosition("tensor2") < pntDim)
        {
        pnt->m_TensorMatrix[1] = v[this->GetPosition("tensor2")];
        }
      // Read tensor3
      if(this->GetPosition("tensor3") >= 0
         && this->GetPosition("tensor3") < pntDim)
        {
        pnt->m_TensorMatrix[2] = v[this->GetPosition("tensor3")];
        }
      // Read tensor4
      if(this->GetPosition("tensor4") >= 0
         && this->GetPosition("tensor4") < pntDim)
        {
        pnt->m_TensorMatrix[3] = v[this->GetPosition("tensor4")];
        }
      // Read tensor5
      if(this->GetPosition("tensor5") >= 0
         && this->GetPosition("tensor5") < pntDim)
        {
        pnt->m_TensorMatrix[4] = v[this->GetPosition("tensor5")];
        }
      // Read tensor6
      if(this->GetPosition("tensor6") >= 0
         && this->GetPosition("tensor6") < pntDim)
        {
        pnt->m_TensorMatrix[5] = v[this->GetPosition("tensor6")];
        }

      // Add the extrafields
      METAIO_STL::vector<PositionType>::const_iterator itFields =
                                                           m_Positions.begin();
      METAIO_STL::vector<PositionType>::const_iterator itFieldsEnd =
                                                           m_Positions.end();
      while(itFields != itFieldsEnd)
        {
        if(strcmp((*itFields).first.c_str(),"x")
          && strcmp((*itFields).first.c_str(),"y")
          && strcmp((*itFields).first.c_str(),"z")
          && strcmp((*itFields).first.c_str(),"tensor1")
          && strcmp((*itFields).first.c_str(),"tensor2")
          && strcmp((*itFields).first.c_str(),"tensor3")
          && strcmp((*itFields).first.c_str(),"tensor4")
          && strcmp((*itFields).first.c_str(),"tensor5")
          && strcmp((*itFields).first.c_str(),"tensor6")
          )
          {
          pnt->AddField((*itFields).first.c_str(),
                        v[this->GetPosition((*itFields).first.c_str())]);
          }
        ++itFields;
        }

      m_PointList.push_back(pnt);
      }

    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
      {
      c = static_cast<char>(m_ReadStream->get());// to avoid unrecognize charactere
      }
    }

  if(m_Event)
    {
    m_Event->StopReading();
    }

  return true;
}

MET_ValueEnumType MetaDTITube::
ElementType(void) const
{
  return m_ElementType;
}

void MetaDTITube::
ElementType(MET_ValueEnumType _elementType)
{
  m_ElementType = _elementType;
}

bool MetaDTITube::
M_Write(void)
{

  if(!MetaObject::M_Write())
    {
    METAIO_STREAM::cout << "MetaDTITube: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  /** Then copy all DTITubes points */
  if(m_BinaryData)
    {
    PointListType::const_iterator it = m_PointList.begin();
    PointListType::const_iterator itEnd = m_PointList.end();
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);

    unsigned int pntDim = m_NDims+6;
    const DTITubePnt::FieldListType & extraList =
                                    (*(m_PointList.begin()))->GetExtraFields();
    pntDim += static_cast<unsigned int>(extraList.size());

    char* data = new char[pntDim*m_NPoints*elementSize];
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

      for(d = 0; d < 6; d++)
        {
        float x = (*it)->m_TensorMatrix[d];
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType, data, i++);
        }

      // Add the extra fields
      const DTITubePnt::FieldListType & extraList2 = (*it)->GetExtraFields();
      DTITubePnt::FieldListType::const_iterator itFields = extraList2.begin();
      DTITubePnt::FieldListType::const_iterator itFieldsEnd = extraList2.end();
      while(itFields !=  itFieldsEnd)
        {
        float x = (*itFields).second;
        MET_SwapByteIfSystemMSB(&x,MET_FLOAT);
        MET_DoubleToValue((double)x,m_ElementType,data,i++);
        ++itFields;
        }

      ++it;
      }

    m_WriteStream->write((char *)data,i*elementSize);
    m_WriteStream->write("\n",1);
    delete []data;
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

      for(d = 0; d < 6; d++)
        {
        *m_WriteStream << (*it)->m_TensorMatrix[d] << " ";
        }

      // Add the extra fields
      const DTITubePnt::FieldListType & extraList = (*it)->GetExtraFields();
      DTITubePnt::FieldListType::const_iterator itFields = extraList.begin();
      DTITubePnt::FieldListType::const_iterator itFieldsEnd = extraList.end();
      while(itFields !=  itFieldsEnd)
        {
        *m_WriteStream << (*itFields).second << " ";
        ++itFields;
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
