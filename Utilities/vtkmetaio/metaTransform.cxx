/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaTransform.h"

#ifdef _MSC_VER
#pragma warning(disable:4702)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


/** MetaTransform constructors */
MetaTransform::
MetaTransform()
:MetaObject()
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaTransform()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaTransform::
MetaTransform(const char *_headerName)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaTransform()" << METAIO_STREAM::endl;
  Clear();
  Read(_headerName);
}

//
MetaTransform::
MetaTransform(const MetaTransform *_group)
:MetaObject()
{
  if(META_DEBUG)  METAIO_STREAM::cout << "MetaTransform()" << METAIO_STREAM::endl;
  Clear();
  CopyInfo(_group);
}

MetaTransform::
MetaTransform(unsigned int dim)
:MetaObject(dim)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaTransform()" << METAIO_STREAM::endl;
  Clear();
}

//
MetaTransform::
~MetaTransform()
{
  delete parameters;
  M_Destroy();
}

//
void MetaTransform::
PrintInfo() const
{
  MetaObject::PrintInfo();
}

void MetaTransform::
CopyInfo(const MetaObject * _object)
{
  MetaObject::CopyInfo(_object);
}

/** Clear group information */
void MetaTransform::
Clear(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaTransform: Clear" << METAIO_STREAM::endl;
  MetaObject::Clear();
  if(parameters)
    {
    delete parameters;
    }
  parameters = NULL;
  parametersDimension = 0;
  transformOrder = 0;


  for(unsigned int i=0;i<100;i++)
    {
    gridSpacing[i] = 1;
    gridOrigin[i] = 0;
    gridRegionSize[i] = 0;
    gridRegionIndex[i] = 0;
    }
}

/** Destroy group information */
void MetaTransform::
M_Destroy(void)
{
  MetaObject::M_Destroy();
}

/** Set Read fields */
void MetaTransform::
M_SetupReadFields(void)
{
  if(META_DEBUG) METAIO_STREAM::cout << "MetaTransform: M_SetupReadFields" << METAIO_STREAM::endl;
  MetaObject::M_SetupReadFields();

  int nDimsRecordNumber = MET_GetFieldRecordNumber("NDims", &m_Fields);

  MET_FieldRecordType* mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Order", MET_INT,false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "GridRegionSize", MET_DOUBLE_ARRAY,false,nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "GridRegionIndex", MET_DOUBLE_ARRAY,false,nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "GridOrigin", MET_DOUBLE_ARRAY,false,nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "GridSpacing", MET_DOUBLE_ARRAY,false,nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NParameters", MET_INT,true);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Parameters", MET_NONE);
  mF->terminateRead = true;
  m_Fields.push_back(mF);

}

void MetaTransform::
M_SetupWriteFields(void)
{
  strcpy(m_ObjectTypeName,"Transform");
  MetaObject::M_SetupWriteFields();

  // We don't want to write the matrix and the offset
  MET_FieldRecordType * mF;
  mF = MET_GetFieldRecord("TransformMatrix",&m_Fields);

  FieldsContainerType::iterator it = m_Fields.begin();
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

  int i;
  bool writeCoR = false;
  for(i=0;i<m_NDims;i++)
    {
    if(m_CenterOfRotation[i] != 0.0)
      {
      writeCoR = true;
      break;
      }
    }

  if(!writeCoR)
    {
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
    }

  if(transformOrder > 0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Order", MET_INT,transformOrder);
    m_Fields.push_back(mF);
    }

  // Grid Spacing
  bool writeGridSpacing = false;
  for(i=0;i<100;i++)
    {
    if(gridSpacing[i] != 1)
      {
      writeGridSpacing = true;
      break;
      }
    }

  if(writeGridSpacing)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "GridSpacing", MET_DOUBLE_ARRAY,m_NDims,gridSpacing);
    m_Fields.push_back(mF);
    }

  // Grid Origin
  bool writeGridOrigin = false;
  for(i=0;i<100;i++)
    {
    if(gridOrigin[i] != 0)
      {
      writeGridOrigin = true;
      break;
      }
    }

  if(writeGridOrigin)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "GridOrigin", MET_DOUBLE_ARRAY,m_NDims,gridOrigin);
    m_Fields.push_back(mF);
    }

  // Grid region size
  bool writeGridRegionSize = false;
  for(i=0;i<100;i++)
    {
    if(gridRegionSize[i] != 0)
      {
      writeGridRegionSize = true;
      break;
      }
    }

  if(writeGridRegionSize)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "GridRegionSize", MET_DOUBLE_ARRAY,m_NDims,gridRegionSize);
    m_Fields.push_back(mF);
    }


  // Grid region index
  bool writeGridRegionIndex = false;
  for(i=0;i<100;i++)
    {
    if(gridRegionIndex[i] != 0)
      {
      writeGridRegionIndex = true;
      break;
      }
    }

  if(writeGridRegionIndex)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "GridRegionIndex", MET_DOUBLE_ARRAY,m_NDims,gridRegionIndex);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NParameters", MET_INT,parametersDimension);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Parameters", MET_NONE);
  m_Fields.push_back(mF);
}

bool MetaTransform::
M_Write(void)
{

  if(!MetaObject::M_Write())
  {
    METAIO_STREAM::cout << "MetaLandmark: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
  }

  /** Then copy all points */
  if(m_BinaryData)
    {
    char* data = new char[parametersDimension*sizeof(double)];
    unsigned int j=0;
    for(unsigned int i=0;i<parametersDimension;i++)
      {
      data[j] = (char)parameters[i];
      j+=sizeof(double);
      }
    m_WriteStream->write((char *)data,parametersDimension*sizeof(double));
    m_WriteStream->write("\n",1);
    delete [] data;
    }
  else
    {
     for(unsigned int i=0;i<parametersDimension;i++)
      {
      *m_WriteStream << parameters[i] << " ";
      }
      *m_WriteStream << METAIO_STREAM::endl;
    }

  return true;

}


// Set/Get the spacing
const double * MetaTransform::GridSpacing(void) const
{
  return gridSpacing;
}

void  MetaTransform::GridSpacing(const double * _gridSpacing)
{
  for(int i=0;i<m_NDims;i++)
    {
    gridSpacing[i] = _gridSpacing[i];
    }
}

// Set/Get the grid index
const double * MetaTransform::GridOrigin(void) const
{
  return gridOrigin;
}

void  MetaTransform::GridOrigin(const double * _gridOrigin)
{
  for(int i=0;i<m_NDims;i++)
    {
    gridOrigin[i] = _gridOrigin[i];
    }
}

// Set/Get the region size
const double * MetaTransform::GridRegionSize(void) const
{
  return gridRegionSize;
}

void  MetaTransform::GridRegionSize(const double * _gridRegionSize)
{
  for(int i=0;i<m_NDims;i++)
    {
    gridRegionSize[i] = _gridRegionSize[i];
    }
}

// Set/Get the region index
const double * MetaTransform::GridRegionIndex(void) const
{
  return gridRegionIndex;
}

void  MetaTransform::GridRegionIndex(const double * _gridRegionIndex)
{
  for(int i=0;i<m_NDims;i++)
    {
    gridRegionIndex[i] = _gridRegionIndex[i];
    }
}

const double * MetaTransform::Parameters(void) const
{
  return parameters;
}


void  MetaTransform::Parameters(unsigned int dimension, const double * _parameters)
{
  parametersDimension = dimension;

  if(parameters)
    {
    delete parameters;
    }

  parameters = new double[parametersDimension];

  // Copy the parameters
  for(unsigned int i=0;i<parametersDimension;i++)
    {
    parameters[i] = _parameters[i];
    }
}

bool MetaTransform::
M_Read(void)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTransform: M_Read: Loading Header" << METAIO_STREAM::endl;
    }

  if(!MetaObject::M_Read())
    {
    METAIO_STREAM::cout << "MetaTransform: M_Read: Error parsing file" << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaTransform: M_Read: Parsing Header" << METAIO_STREAM::endl;
    }

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("NParameters", &m_Fields);
  if(mF->defined)
    {
    parametersDimension = (unsigned int)mF->value[0];
    }

  mF = MET_GetFieldRecord("GridSpacing", &m_Fields);
  int i;
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      gridSpacing[i] = static_cast<double>( mF->value[i] );
      }
    }

  mF = MET_GetFieldRecord("GridOrigin", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      gridOrigin[i] = static_cast<double>( mF->value[i] );
      }
    }

  mF = MET_GetFieldRecord("GridRegionSize", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      gridRegionSize[i] = static_cast<double>( mF->value[i] );
      }
    }

  mF = MET_GetFieldRecord("GridRegionIndex", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      gridRegionIndex[i] = static_cast<double>( mF->value[i] );
      }
    }


  mF = MET_GetFieldRecord("Order", &m_Fields);
  if(mF->defined)
    {
    transformOrder = (unsigned int)mF->value[0];
    }

  if(parameters)
    {
    delete parameters;
    }

  parameters = new double[parametersDimension];

  if(m_BinaryData)
    {
    char* _data = new char[parametersDimension*sizeof(double)];
    m_ReadStream->read((char *)_data, parametersDimension*sizeof(double));

    unsigned int gc = m_ReadStream->gcount();
    if(gc != parametersDimension*sizeof(double))
      {
      METAIO_STREAM::cout << "MetaTransform: m_Read: data not read completely"
                << METAIO_STREAM::endl;
      METAIO_STREAM::cout << "   ideal = " << parametersDimension*sizeof(double) << " : actual = " << gc << METAIO_STREAM::endl;
      return false;
      delete [] _data;
      }

    unsigned long k=0;

    for(unsigned int j=0; j<parametersDimension; j++)
      {
      parameters[j] = _data[k];
      k += sizeof(double);
      }
    delete [] _data;
  }
  else
    {
    for(unsigned int k=0; k<parametersDimension; k++)
      {
      *m_ReadStream >> parameters[k];
      m_ReadStream->get();
      }
    }

/*
    char c = ' ';
    while( (c!='\n') && (!m_ReadStream->eof()))
    {
      c = m_ReadStream->get();// to avoid unrecognize charactere
    }
 */
  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif

