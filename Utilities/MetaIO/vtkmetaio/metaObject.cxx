/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaObject.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#include <mem.h>
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


//
// MetaObject Constructors
//
MetaObject::
MetaObject(void)
  {
  m_NDims = 0;
  this->ClearFields();
  this->ClearUserFields();
  this->ClearAdditionalFields();
  MetaObject::Clear();
  m_ReadStream = NULL;
  m_WriteStream = NULL;
  m_FileName[0] = '\0';
  m_Event = NULL;
  m_DoublePrecision = 6;
  m_DistanceUnits = MET_DISTANCE_UNITS_UNKNOWN;
  }

MetaObject::
MetaObject(const char * _fileName)
  {
  m_NDims = 0;
  this->ClearFields();
  this->ClearUserFields();
  this->ClearAdditionalFields();
  MetaObject::Clear();
  m_ReadStream = NULL;
  m_WriteStream = NULL;
  this->Read(_fileName);
  m_Event = NULL;
  m_DoublePrecision = 6;
  m_DistanceUnits = MET_DISTANCE_UNITS_UNKNOWN;
  }

MetaObject::
MetaObject(unsigned int dim)
  {
  m_NDims = 0;
  this->ClearFields();
  this->ClearUserFields();
  this->ClearAdditionalFields();
  MetaObject::Clear();
  m_ReadStream = NULL;
  m_WriteStream = NULL;
  m_FileName[0] = '\0';
  InitializeEssential(dim);
  m_Event = NULL;
  m_DoublePrecision = 6;
  m_DistanceUnits = MET_DISTANCE_UNITS_UNKNOWN;
  }


MetaObject::
~MetaObject(void)
  {
  M_Destroy();
  delete m_ReadStream;
  delete m_WriteStream;

  this->ClearFields();
  this->ClearUserFields();
  this->ClearAdditionalFields();
  }


//
// Clear Fields only, if the pointer is in the UserField list it is not deleted.
void MetaObject::
ClearFields()
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject:ClearFields" << METAIO_STREAM::endl;
    }

  FieldsContainerType::iterator  it  = m_Fields.begin();
  FieldsContainerType::iterator  end = m_Fields.end();
  while( it != end )
    {
    MET_FieldRecordType* field = *it;
    ++it;

    // Check if the pointer is not in one of the user's list
    bool exists = false;
    FieldsContainerType::iterator  it2  = m_UserDefinedWriteFields.begin();
    FieldsContainerType::iterator  end2 = m_UserDefinedWriteFields.end();
    while( it2 != end2 )
    {
      if(*it2 == field)
        {
        exists = true;
        break;
        }
      ++it2;
    }

    if(!exists)
      {
      it2  = m_UserDefinedReadFields.begin();
      end2 = m_UserDefinedReadFields.end();
      while( it2 != end2 )
      {
        if(*it2 == field)
          {
          exists = true;
          break;
          }
        ++it2;
      }
    }

    if(!exists)
      {
      delete field;
      }
    }
  m_Fields.clear();
  }


// Clear UserFields
void MetaObject
::ClearUserFields()
  {
  // Clear write field
  FieldsContainerType::iterator  it  = m_UserDefinedWriteFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedWriteFields.end();
  while( it != end )
    {
    MET_FieldRecordType* field = *it;
    ++it;
    delete field;
    }


  // Clear read field
  it  = m_UserDefinedReadFields.begin();
  end = m_UserDefinedReadFields.end();
  while( it != end )
    {
    MET_FieldRecordType* field = *it;

    // check if the pointer has not been deleted yet
    bool deleted = false;
    FieldsContainerType::iterator  it2  = m_UserDefinedWriteFields.begin();
    FieldsContainerType::iterator  end2 = m_UserDefinedWriteFields.end();
    while( it2 != end2 )
      {
      if(*it2 == *it)
        {
        deleted = true;
        break;
        }
      ++it2;
      }

    ++it;

    if(!deleted)
      {
      delete field;
      }
    }

  m_UserDefinedWriteFields.clear();
  m_UserDefinedReadFields.clear();
  }

// Clear AdditionalReadFields
void MetaObject
::ClearAdditionalFields()
  {
  // Clear read field
  FieldsContainerType::iterator it  = m_AdditionalReadFields.begin();
  FieldsContainerType::iterator end = m_AdditionalReadFields.end();
  while( it != end )
    {
    MET_FieldRecordType* field = *it;
    if( field )
      {
      delete field;
      }
    ++it;
    }

  m_AdditionalReadFields.clear();
  }

//
//
void MetaObject::
FileName(const char *_fileName)
  {
  if(_fileName != NULL)
    {
    if(_fileName[0] != '\0')
      {
      strcpy(m_FileName, _fileName);
      }
    }
  }

const char * MetaObject::
FileName(void) const
  {
  return m_FileName;
  }

void MetaObject::
CopyInfo(const MetaObject * _object)
  {
  if(NDims() != _object->NDims())
    {
    METAIO_STREAM::cout << "MetaObject: CopyInfo: Warning: NDims not same size"
                        << METAIO_STREAM::endl;
    }

  FileName(_object->FileName());
  Comment(_object->Comment());
  ObjectTypeName(_object->ObjectTypeName());
  ObjectSubTypeName(_object->ObjectSubTypeName());
  CenterOfRotation(_object->CenterOfRotation());
  Offset(_object->Offset());
  TransformMatrix(_object->TransformMatrix());
  ElementSpacing(_object->ElementSpacing());
  ID(_object->ID());
  Color(_object->Color());
  ParentID(_object->ParentID());
  AcquisitionDate(_object->AcquisitionDate());
  Name(_object->Name());
  BinaryData(_object->BinaryData());
  BinaryDataByteOrderMSB(_object->BinaryDataByteOrderMSB());
  DistanceUnits(_object->DistanceUnits());
  }

bool MetaObject::
Read(const char *_fileName)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Read" << METAIO_STREAM::endl;
    }

  if(_fileName != NULL)
    {
    strcpy(m_FileName, _fileName);
    }

  METAIO_STREAM::ifstream * tmpReadStream = new METAIO_STREAM::ifstream;

#ifdef __sgi
  tmpReadStream->open(m_FileName, METAIO_STREAM::ios::in);
#else
  tmpReadStream->open(m_FileName, METAIO_STREAM::ios::binary |
                                  METAIO_STREAM::ios::in);
#endif

  if(!tmpReadStream->rdbuf()->is_open())
    {
    delete tmpReadStream;
    return false;
    }

  if( !this->ReadStream(0, tmpReadStream) )
    {
    tmpReadStream->close();
    delete tmpReadStream;
    return false;
    }

  tmpReadStream->close();

  delete tmpReadStream;

  return true;
  }


bool MetaObject::
ReadStream(int _nDims, METAIO_STREAM::ifstream * _stream)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: ReadStream" << METAIO_STREAM::endl;
    }

  M_Destroy();

  fflush(NULL);

  Clear();

  M_SetupReadFields();

  if(_nDims > 0)
    {
    MET_FieldRecordType * mF = MET_GetFieldRecord("NDims", &m_Fields);
    mF->value[0] = _nDims;
    mF->defined = true;
    }

  if(m_ReadStream)
    {
    delete m_ReadStream;
    }

  m_ReadStream = _stream;

  bool result = M_Read();

  m_ReadStream= NULL;

  return result;
  }


bool MetaObject::
Write(const char *_fileName)
  {
  if(_fileName != NULL)
    {
    FileName(_fileName);
    }

  M_SetupWriteFields();

  if(!m_WriteStream)
    {
    m_WriteStream = new METAIO_STREAM::ofstream;
    }

#ifdef __sgi
  // Create the file. This is required on some older sgi's
  METAIO_STREAM::ofstream tFile(m_FileName, METAIO_STREAM::ios::out);
  tFile.close();
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::out);
#else
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::binary |
                                  METAIO_STREAM::ios::out);
#endif

  if(!m_WriteStream->rdbuf()->is_open())
    {
    return false;
    }

  bool result = M_Write();

  m_WriteStream->close();
  delete m_WriteStream;
  m_WriteStream = 0;

  return result;
  }

//
//
void MetaObject::
PrintInfo(void) const
  {
  int i, j;

  METAIO_STREAM::cout << "FileName = _" << m_FileName << "_"
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Comment = _" << m_Comment << "_"
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ObjectType = _" << m_ObjectTypeName << "_"
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ObjectSubType = _" << m_ObjectSubTypeName << "_"
                      << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "NDims = " << m_NDims << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Name = " << m_Name << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ID = " << m_ID << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "ParentID = " << m_ParentID << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "AcquisitionDate = " << m_AcquisitionDate << METAIO_STREAM::endl;
  if(m_CompressedData)
    {
    METAIO_STREAM::cout << "CompressedData = True" << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "CompressedData = False" << METAIO_STREAM::endl;
    }
  METAIO_STREAM::cout << "m_CompressedDataSize = " << m_CompressedDataSize
                      << METAIO_STREAM::endl;
  if(m_BinaryData)
    {
    METAIO_STREAM::cout << "BinaryData = True" << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "BinaryData = False" << METAIO_STREAM::endl;
    }
  if(m_BinaryData && m_BinaryDataByteOrderMSB)
    {
    METAIO_STREAM::cout << "BinaryDataByteOrderMSB = True"
                        << METAIO_STREAM::endl;
    }
  else
    {
    METAIO_STREAM::cout << "BinaryDataByteOrderMSB = False"
                        << METAIO_STREAM::endl;
    }
  METAIO_STREAM::cout << "Color = " ;
  for(i=0; i<4; i++)
    {
    METAIO_STREAM::cout << m_Color[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Offset = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_Offset[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "TransformMatrix = ";
  METAIO_STREAM::cout << METAIO_STREAM::endl;
  for(i=0; i<m_NDims; i++)
    {
    for(j=0; j<m_NDims; j++)
      {
      METAIO_STREAM::cout << m_TransformMatrix[i*m_NDims+j] << " ";
      }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    }

  METAIO_STREAM::cout << "CenterOfRotation = ";
  METAIO_STREAM::cout << METAIO_STREAM::endl;
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_CenterOfRotation[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementSpacing = ";
  for(i=0; i<m_NDims; i++)
    {
    METAIO_STREAM::cout << m_ElementSpacing[i] << " ";
    }
  METAIO_STREAM::cout << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "DistanceUnits = " << this->DistanceUnitsName()
                      << METAIO_STREAM::endl;

  // Print User's fields :
  FieldsContainerType::const_iterator  itw = m_UserDefinedWriteFields.begin();
  FieldsContainerType::const_iterator  itr  = m_UserDefinedReadFields.begin();
  FieldsContainerType::const_iterator  endw = m_UserDefinedWriteFields.end();
  FieldsContainerType::const_iterator it;
  while( itw != endw )
    {
    if((*itw)->defined)
      {
      it=itw;
      }
    else
      {
      it=itr;
      }

    printf("%s: ",(*it)->name);

    if((*it)->type == MET_STRING)
      {
      printf("%s",(char *) (*it)->value);
      }
    else if( (*it)->type == MET_ASCII_CHAR ||
             (*it)->type == MET_CHAR ||
             (*it)->type == MET_UCHAR ||
             (*it)->type == MET_SHORT ||
             (*it)->type == MET_USHORT ||
             (*it)->type == MET_LONG ||
             (*it)->type == MET_ULONG ||
             (*it)->type == MET_INT ||
             (*it)->type == MET_UINT ||
             (*it)->type == MET_FLOAT ||
             (*it)->type == MET_DOUBLE )
      {
      printf("%s : %f\n",(*it)->name,(*it)->value[0]);
      }
    else if( (*it)->type ==MET_CHAR_ARRAY ||
             (*it)->type ==MET_UCHAR_ARRAY ||
             (*it)->type ==MET_SHORT_ARRAY ||
             (*it)->type ==MET_USHORT_ARRAY ||
             (*it)->type ==MET_INT_ARRAY ||
             (*it)->type ==MET_UINT_ARRAY ||
             (*it)->type ==MET_FLOAT_ARRAY ||
             (*it)->type ==MET_DOUBLE_ARRAY )
      {
      for(i=0; i<(*it)->length; i++)
        {
        printf("%f ",(*it)->value[i]);
        }
      }
    else if((*it)->type == MET_FLOAT_MATRIX)
      {
      METAIO_STREAM::cout << METAIO_STREAM::endl;
      for(i=0; i<(*it)->length*(*it)->length; i++)
        {
        printf("%f ",(*it)->value[i]);
        if(i==(*it)->length-1)
          {
          METAIO_STREAM::cout << METAIO_STREAM::endl;
          }
        }
      }
    METAIO_STREAM::cout << METAIO_STREAM::endl;

    ++itw;
    ++itr;
    }
  }

const char * MetaObject::
Comment(void) const
  {
  return m_Comment;
  }

void MetaObject::
Comment(const char * _comment)
  {
  strcpy(m_Comment, _comment);
  }

const char * MetaObject::
ObjectTypeName(void) const
  {
  return m_ObjectTypeName;
  }

void MetaObject::
ObjectTypeName(const char * _objectTypeName)
  {
  strcpy(m_ObjectTypeName, _objectTypeName);
  }

const char * MetaObject::
ObjectSubTypeName(void) const
  {
  return m_ObjectSubTypeName;
  }

void MetaObject::
ObjectSubTypeName(const char * _objectSubTypeName)
  {
  strcpy(m_ObjectSubTypeName, _objectSubTypeName);
  }

int MetaObject::
NDims(void) const
  {
  return m_NDims;
  }

const double * MetaObject::
Offset(void) const
  {
  return m_Offset;
  }

double MetaObject::
Offset(int _i) const
  {
  return m_Offset[_i];
  }

void MetaObject::
Offset(const double * _position)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_Offset[i] = _position[i];
    }
  }

void MetaObject::
Offset(int _i, double _value)
  {
  m_Offset[_i] = _value;
  }


const double * MetaObject::
Position(void) const
  {
  return m_Offset;
  }

double MetaObject::
Position(int _i) const
  {
  return m_Offset[_i];
  }

void MetaObject::
Position(const double * _position)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_Offset[i] = _position[i];
    }
  }

void MetaObject::
Position(int _i, double _value)
  {
  m_Offset[_i] = _value;
  }

const double * MetaObject::
Origin(void) const
  {
  return m_Offset;
  }

double MetaObject::
Origin(int _i) const
  {
  return m_Offset[_i];
  }

void MetaObject::
Origin(const double * _position)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_Offset[i] = _position[i];
    }
  }

void MetaObject::
Origin(int _i, double _value)
  {
  m_Offset[_i] = _value;
  }

//
//
const double * MetaObject::
TransformMatrix(void) const
  {
  return m_TransformMatrix;
  }

double MetaObject::
TransformMatrix(int _i, int _j) const
  {
  return m_TransformMatrix[_i*m_NDims+_j];
  }

void MetaObject::
TransformMatrix(const double * _orientation)
  {
  int i;
  for(i=0; i<m_NDims*m_NDims; i++)
    {
    m_TransformMatrix[i] = _orientation[i];
    }
  }

void MetaObject::
TransformMatrix(int _i, int _j, double _value)
  {
  m_TransformMatrix[_i*m_NDims+_j] = _value;
  }

//
const double * MetaObject::
Rotation(void) const
  {
  return m_TransformMatrix;
  }

double MetaObject::
Rotation(int _i, int _j) const
  {
  return m_TransformMatrix[_i*m_NDims+_j];
  }

void MetaObject::
Rotation(const double * _orientation)
  {
  int i;
  for(i=0; i<m_NDims*m_NDims; i++)
    {
    m_TransformMatrix[i] = _orientation[i];
    }
  }

void MetaObject::
Rotation(int _i, int _j, double _value)
  {
  m_TransformMatrix[_i*m_NDims+_j] = _value;
  }

//
const double * MetaObject::
Orientation(void) const
  {
  return m_TransformMatrix;
  }

double MetaObject::
Orientation(int _i, int _j) const
  {
  return m_TransformMatrix[_i*m_NDims+_j];
  }

void MetaObject::
Orientation(const double * _orientation)
  {
  int i;
  for(i=0; i<m_NDims*m_NDims; i++)
    {
    m_TransformMatrix[i] = _orientation[i];
    }
  }

void MetaObject::
Orientation(int _i, int _j, double _value)
  {
  m_TransformMatrix[_i*m_NDims+_j] = _value;
  }

//
//
const double * MetaObject::
CenterOfRotation(void) const
  {
  return m_CenterOfRotation;
  }

double MetaObject::
CenterOfRotation(int _i) const
  {
  return m_CenterOfRotation[_i];
  }

void MetaObject::
CenterOfRotation(const double * _position)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_CenterOfRotation[i] = _position[i];
    }
  }

void MetaObject::
CenterOfRotation(int _i, double _value)
  {
  m_CenterOfRotation[_i] = _value;
  }

//
const char * MetaObject::
DistanceUnitsName(void) const
  {
  return (const char *)(MET_DistanceUnitsTypeName[m_DistanceUnits]);
  }

MET_DistanceUnitsEnumType MetaObject::
DistanceUnits(void) const
  {
  return m_DistanceUnits;
  }

void MetaObject::
DistanceUnits(MET_DistanceUnitsEnumType _distanceUnits)
  {
  m_DistanceUnits = _distanceUnits;
  }

void MetaObject::
DistanceUnits(const char * _distanceUnits)
  {
  int i;
  bool found = false;
  for(i=0; i<MET_NUM_DISTANCE_UNITS_TYPES; i++)
    {
    if(!strcmp(_distanceUnits, MET_DistanceUnitsTypeName[i]))
      {
      m_DistanceUnits = static_cast<MET_DistanceUnitsEnumType>(i);
      found = true;
      break;
      }
    }
  if(!found)
    {
    m_DistanceUnits = MET_DISTANCE_UNITS_UNKNOWN;
    }
  }

//
//
const char * MetaObject::
AnatomicalOrientationAcronym(void) const
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_OrientationAcronym[i] = MET_OrientationTypeName[m_AnatomicalOrientation[i]][0];
    }
  m_OrientationAcronym[i] = '\0';
  return m_OrientationAcronym;
  }

const MET_OrientationEnumType * MetaObject::
AnatomicalOrientation(void) const
  {
  return m_AnatomicalOrientation;
  }

MET_OrientationEnumType MetaObject::
AnatomicalOrientation(int _dim) const
  {
  return m_AnatomicalOrientation[_dim];
  }

void MetaObject::
AnatomicalOrientation(const char *_ao)
  {
  int i, j;
  for(i=0; i<m_NDims; i++)
    {
    for(j=0; j<MET_NUM_ORIENTATION_TYPES; j++)
      {
      if(_ao[i] == MET_OrientationTypeName[j][0])
        {
        m_AnatomicalOrientation[i] = (MET_OrientationEnumType)j;
        break;
        }
      }
    if(j == MET_NUM_ORIENTATION_TYPES)
      {
      m_AnatomicalOrientation[i] = MET_ORIENTATION_UNKNOWN;
      }
    }
  }

void MetaObject::
AnatomicalOrientation(const MET_OrientationEnumType *_ao)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_AnatomicalOrientation[i] = _ao[i];
    }
  }

void MetaObject::
AnatomicalOrientation(int _dim, MET_OrientationEnumType _ao)
  {
  m_AnatomicalOrientation[_dim] = _ao;
  }

void MetaObject::
AnatomicalOrientation(int _dim, char _ao)
  {
  int j;
  for(j=0; j<MET_NUM_ORIENTATION_TYPES; j++)
    {
    if(_ao == MET_OrientationTypeName[j][0])
      {
      m_AnatomicalOrientation[_dim] = (MET_OrientationEnumType)j;
      return;
      }
    }

  m_AnatomicalOrientation[_dim] = MET_ORIENTATION_UNKNOWN;
  }

//
//
const float * MetaObject::
ElementSpacing(void) const
  {
  return m_ElementSpacing;
  }

float MetaObject::
ElementSpacing(int _i) const
  {
  return m_ElementSpacing[_i];
  }

void MetaObject::
ElementSpacing(const float * _elementSpacing)
  {
  int i;
  for(i=0; i<m_NDims; i++)
    {
    m_ElementSpacing[i] = _elementSpacing[i];
    }
  }

void MetaObject::
ElementSpacing(int _i, float _value)
  {
  m_ElementSpacing[_i] = _value;
  }


void  MetaObject::
Name(const char *_Name)
  {
  if(_Name != NULL)
    {
    strcpy(m_Name, _Name);
    }
  }

const char  * MetaObject::
Name(void) const
  {
  return m_Name;
  }


const float * MetaObject::
Color(void) const
  {
  return m_Color;
  }

void  MetaObject::
Color(float _r, float _g, float _b, float _a)
  {
  m_Color[0] = _r;
  m_Color[1] = _g;
  m_Color[2] = _b;
  m_Color[3] = _a;
  }
void MetaObject::
Color(const float * _color)
  {
  for(unsigned int i=0; i<4; i++)
    {
    m_Color[i] = _color[i];
    }
  }


void  MetaObject::
ID(int _id)
  {
  m_ID = _id;
  }

int  MetaObject::
ID(void) const
  {
  return m_ID;
  }

void  MetaObject::
ParentID(int _parentId)
  {
  m_ParentID = _parentId;
  }

int   MetaObject::ParentID(void) const
  {
  return m_ParentID;
  }

void  MetaObject::
AcquisitionDate(const char * _acquisitionDate)
  {
  for(size_t i=0; i<strlen( _acquisitionDate ); i++)
    {
    m_AcquisitionDate[i] = _acquisitionDate[i];
    }
  m_AcquisitionDate[strlen( _acquisitionDate )] = '\0';
  }

const char * MetaObject::AcquisitionDate(void) const
  {
  return m_AcquisitionDate;
  }

void MetaObject::CompressedData(bool _compressedData)
  {
  m_CompressedData = _compressedData;
  }

bool MetaObject::CompressedData(void) const
  {
  return m_CompressedData;
  }

void  MetaObject::BinaryData(bool _binaryData)
  {
  m_BinaryData = _binaryData;
  }

bool   MetaObject::BinaryData(void) const
  {
  return m_BinaryData;
  }

bool MetaObject::
BinaryDataByteOrderMSB(void) const
  {
  return m_BinaryDataByteOrderMSB;
  }

void MetaObject::
BinaryDataByteOrderMSB(bool _elementByteOrderMSB)
  {
  m_BinaryDataByteOrderMSB = _elementByteOrderMSB;
  }

void MetaObject::
Clear(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Clear()" << METAIO_STREAM::endl;
    }
  strcpy(m_Comment, "");
  strcpy(m_ObjectTypeName, "Object");
  strcpy(m_ObjectSubTypeName, "");
  strcpy(m_Name, "");

  memset(m_Offset, 0, 10*sizeof(float));
  memset(m_TransformMatrix, 0, 100*sizeof(float));
  memset(m_CenterOfRotation, 0, 10*sizeof(float));
  memset(m_Color, 0, 4*sizeof(float));

  m_ID = -1;
  m_Color[0]=1.0f;
  m_Color[1]=1.0f;
  m_Color[2]=1.0f;
  m_Color[3]=1.0f; // white by default
  m_ParentID = -1;
  strcpy(m_AcquisitionDate, "");
  m_BinaryData = false;
  m_BinaryDataByteOrderMSB = MET_SystemByteOrderMSB();
  m_CompressedDataSize = 0;
  m_CompressedData = false;
  m_WriteCompressedDataSize = true;

  m_DistanceUnits = MET_DISTANCE_UNITS_UNKNOWN;

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Clear: m_NDims=" << m_NDims
                        << METAIO_STREAM::endl;
    }
  int i;
  for(i=0; i<10; i++)
    {
    m_ElementSpacing[i] = 1;
    m_AnatomicalOrientation[i] = MET_ORIENTATION_UNKNOWN;
    }
/*
  METAIO_STL::vector<MET_FieldRecordType *>::iterator fieldIter;
  for(fieldIter=m_Fields.begin(); fieldIter!=m_Fields.end(); fieldIter++)
    {
    if(META_DEBUG) METAIO_STREAM::cout << "field = " << (*fieldIter)->name << METAIO_STREAM::endl;
    MET_FieldRecordType* field = *fieldIter;
    delete field;
    field = NULL;
    if(META_DEBUG) METAIO_STREAM::cout << " has been deleted." << METAIO_STREAM::endl;
    }
  m_Fields.clear();*/
  this->ClearFields();
  }

bool MetaObject::
InitializeEssential(int _nDims)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Initialize" << METAIO_STREAM::endl;
    }

  M_Destroy();

  if(_nDims > 10)
    {
    METAIO_STREAM::cout
      << "MetaObject: Initialize: Warning: Number of dimensions limited to 10"
      << METAIO_STREAM::endl
      << "Resetting number of dimensions to 10"
      << METAIO_STREAM::endl;
    _nDims = 10;
    }

  if(_nDims < 0)
    {
    METAIO_STREAM::cout
      << "MetaObject: Initialize: Warning: Number of dimensions must be >= 0"
      << METAIO_STREAM::endl
      << "Resetting number of dimensions to 0"
      << METAIO_STREAM::endl;
    _nDims = 0;
    }

  m_NDims = _nDims;

  return true;
  }

void MetaObject::
M_Destroy(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Destroy" << METAIO_STREAM::endl;
    }
  }

void MetaObject::
M_SetupReadFields(void)
  {
  this->ClearFields();
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: M_SetupReadFields"
                        << METAIO_STREAM::endl;
    }

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Comment", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "AcquisitionDate", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectType", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ObjectSubType", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NDims", MET_INT, true);
  mF->required = true;
  m_Fields.push_back(mF);

  int nDimsRecordNumber = MET_GetFieldRecordNumber("NDims", &m_Fields);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Name", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ID", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ParentID", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "AcquisitionDate", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CompressedData", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CompressedDataSize", MET_FLOAT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "BinaryData", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementByteOrderMSB", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "BinaryDataByteOrderMSB", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Color", MET_FLOAT_ARRAY, false,-1,4);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Position", MET_FLOAT_ARRAY, false,
                     nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Origin", MET_FLOAT_ARRAY, false,
                     nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Offset", MET_FLOAT_ARRAY, false,
                     nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "TransformMatrix", MET_FLOAT_MATRIX, false,
                    nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Rotation", MET_FLOAT_MATRIX, false,
                    nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Orientation", MET_FLOAT_MATRIX, false,
                    nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CenterOfRotation", MET_FLOAT_ARRAY, false,
                     nDimsRecordNumber);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "DistanceUnits", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "AnatomicalOrientation", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementSpacing", MET_FLOAT_ARRAY, false,
                     nDimsRecordNumber);
  mF->required = false;
  m_Fields.push_back(mF);

  // Add User's field
  FieldsContainerType::iterator  it  = m_UserDefinedReadFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedReadFields.end();
  while( it != end )
    {
    m_Fields.push_back(*it);
    ++it;
    }


 }


void MetaObject::
M_SetupWriteFields(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: M_SetupWriteFields"
                        << METAIO_STREAM::endl;
    }

  this->ClearFields();

  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: M_SetupWriteFields: Creating Fields"
                        << METAIO_STREAM::endl;
    }

  MET_FieldRecordType * mF;

  if(strlen(m_Comment)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Comment", MET_STRING, strlen(m_Comment), m_Comment);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ObjectType", MET_STRING, strlen(m_ObjectTypeName),
                    m_ObjectTypeName);
  m_Fields.push_back(mF);

  if(strlen(m_ObjectSubTypeName)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ObjectSubType", MET_STRING,
                      strlen(m_ObjectSubTypeName),
                      m_ObjectSubTypeName);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "NDims", MET_INT, m_NDims);
  m_Fields.push_back(mF);

  if(strlen(m_Name)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Name", MET_STRING, strlen(m_Name),m_Name);
    m_Fields.push_back(mF);
    }

  if(m_ID>=0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ID", MET_INT, m_ID);
    m_Fields.push_back(mF);
    }

  if(m_ParentID >= 0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ParentID", MET_INT, m_ParentID);
    m_Fields.push_back(mF);
    }

  if(strlen(m_AcquisitionDate) > 0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "AcquisitionDate", MET_STRING,
                       strlen(m_AcquisitionDate), m_AcquisitionDate);
    m_Fields.push_back(mF);
    }

  bool valSet = false;
  int i;
  for(i=0; i<4; i++)
    {
    if(m_Color[i] != 1)
      {
      valSet = true;
      break;
      }
    }
  if(valSet)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Color", MET_FLOAT_ARRAY, 4,
                         m_Color);
    m_Fields.push_back(mF);
    }

  if(m_BinaryData)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "BinaryData", MET_STRING, strlen("True"), "True");
    m_Fields.push_back(mF);

    mF = new MET_FieldRecordType;
    if(m_BinaryDataByteOrderMSB)
      {
      MET_InitWriteField(mF, "BinaryDataByteOrderMSB", MET_STRING,
                         strlen("True"), "True");
      }
    else
      {
      MET_InitWriteField(mF, "BinaryDataByteOrderMSB", MET_STRING,
                         strlen("False"), "False");
      }
    m_Fields.push_back(mF);

    if(m_CompressedData)
      {
      mF = new MET_FieldRecordType;
      MET_InitWriteField(mF, "CompressedData", MET_STRING, strlen("True"),
                         "True");
      m_Fields.push_back(mF);

      if(m_WriteCompressedDataSize && m_CompressedDataSize>0)
        {
        mF = new MET_FieldRecordType;
        MET_InitWriteField(mF, "CompressedDataSize", MET_UINT,
                           static_cast<double>(m_CompressedDataSize));
        m_Fields.push_back(mF);
        }
      }
    else
      {
      mF = new MET_FieldRecordType;
      MET_InitWriteField(mF, "CompressedData", MET_STRING, strlen("False"),
                         "False");
      m_Fields.push_back(mF);
      }
    }
  else
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "BinaryData", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
    }

  valSet = false;
  for(i=0; i<m_NDims*m_NDims; i++)
    {
    if(m_TransformMatrix[i] != 0)
      {
      valSet = true;
      break;
      }
    }
  if(!valSet)
    {
    for(i=0; i<m_NDims; i++)
      {
      m_TransformMatrix[i+i*m_NDims] = 1;
      }
    }
  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "TransformMatrix", MET_FLOAT_MATRIX, m_NDims,
                     m_TransformMatrix);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Offset", MET_FLOAT_ARRAY, m_NDims,
                     m_Offset);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "CenterOfRotation", MET_FLOAT_ARRAY, m_NDims,
                     m_CenterOfRotation);
  m_Fields.push_back(mF);

  if(m_DistanceUnits != MET_DISTANCE_UNITS_UNKNOWN)
    {
    const char * str = DistanceUnitsName();
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "DistanceUnits",
                       MET_STRING, strlen(str), str);
    m_Fields.push_back(mF);
    }

  if(m_AnatomicalOrientation[0] != MET_ORIENTATION_UNKNOWN)
    {
    const char * str = AnatomicalOrientationAcronym();
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "AnatomicalOrientation",
                       MET_STRING, strlen(str), str);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ElementSpacing", MET_FLOAT_ARRAY, m_NDims,
                     m_ElementSpacing);
  m_Fields.push_back(mF);

  // Add User's field
  FieldsContainerType::iterator  it  = m_UserDefinedWriteFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedWriteFields.end();
  while( it != end )
    {
    m_Fields.push_back(*it);
    ++it;
    }
  }

bool MetaObject::
M_Read(void)
  {

  this->ClearAdditionalFields();

  if(!MET_Read(*m_ReadStream, &m_Fields, '=', false, true,
     &m_AdditionalReadFields))
    {
    METAIO_STREAM::cerr << "MetaObject: Read: MET_Read Failed"
                        << METAIO_STREAM::endl;
    return false;
    }

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("Comment", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_Comment, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("ObjectType", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_ObjectTypeName, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("ObjectSubType", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_ObjectSubTypeName, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("NDims", &m_Fields);
  if(mF && mF->defined)
    {
    m_NDims = (int)mF->value[0];
    }

  if(m_NDims>0)
    {
    MetaObject::InitializeEssential(m_NDims);
    }

  mF = MET_GetFieldRecord("Name", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_Name, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("ID", &m_Fields);
  if(mF && mF->defined)
    {
    m_ID = (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("ParentID", &m_Fields);
  if(mF && mF->defined)
    {
    m_ParentID = (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("AcquisitionDate", &m_Fields);
  if(mF && mF->defined)
    {
    for(size_t i=0; i<strlen((char *)mF->value); i++)
      {
      m_AcquisitionDate[i] = ((char *)mF->value)[i];
      }
    m_AcquisitionDate[strlen((char *)mF->value)] = '\0';
    }

  mF = MET_GetFieldRecord("CompressedData",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' || ((char *)(mF->value))[0] == 't'
       || ((char *)(mF->value))[0] == '1')
      {
      m_CompressedData = true;
      }
    else
      {
      m_CompressedData = false;
      }
    }

  mF = MET_GetFieldRecord("CompressedDataSize",  &m_Fields);
  if(mF && mF->defined)
    {
    m_CompressedDataSize = (unsigned int)mF->value[0];
    }

  mF = MET_GetFieldRecord("BinaryData",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' || ((char *)(mF->value))[0] == 't'
       || ((char *)(mF->value))[0] == '1')
      {
      m_BinaryData = true;
      }
    else
      {
      m_BinaryData = false;
      }
    }

  mF = MET_GetFieldRecord("ElementByteOrderMSB",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' || ((char *)(mF->value))[0] == 't'
       || ((char *)(mF->value))[0] == '1')
      {
      m_BinaryDataByteOrderMSB = true;
      }
    else
      {
      m_BinaryDataByteOrderMSB = false;
      }
    }

  mF = MET_GetFieldRecord("BinaryDataByteOrderMSB",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' || ((char *)(mF->value))[0] == 't'
       || ((char *)(mF->value))[0] == '1')
      {
      m_BinaryDataByteOrderMSB = true;
      }
    else
      {
      m_BinaryDataByteOrderMSB = false;
      }
    }

  int i;
  mF = MET_GetFieldRecord("Color", &m_Fields);
  if(mF)
    {
    if(mF->defined)
      {
      for(i=0; i<mF->length && i < 4; i++)
        {
        m_Color[i] = static_cast<float>( mF->value[i] );
        }
      }
    else
      {
      for(i=0; i<mF->length && i < 4; i++)
        {
        m_Color[i] = static_cast<unsigned int>( 1 );
        }
      }
    }

  mF = MET_GetFieldRecord("Position", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      m_Offset[i] = static_cast<double>( mF->value[i] );
      }
    }
  mF = MET_GetFieldRecord("Offset", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      m_Offset[i] = static_cast<double>( mF->value[i] );
      }
    }
  mF = MET_GetFieldRecord("Origin", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      m_Offset[i] = static_cast<double>( mF->value[i] );
      }
    }

  bool transformMatrixDefined = false;
  mF = MET_GetFieldRecord("Orientation", &m_Fields);
  if(mF && mF->defined)
    {
    transformMatrixDefined = true;
    int len = mF->length;
    for(i=0; i<len*len; i++)
      {
      m_TransformMatrix[i] = static_cast<double>( mF->value[i] );
      }
    }
  mF = MET_GetFieldRecord("Rotation", &m_Fields);
  if(mF && mF->defined)
    {
    transformMatrixDefined = true;
    int len = mF->length;
    for(i=0; i<len*len; i++)
      {
      m_TransformMatrix[i] = static_cast<double>( mF->value[i] );
      }
    }
  mF = MET_GetFieldRecord("TransformMatrix", &m_Fields);
  if(mF && mF->defined)
    {
    transformMatrixDefined = true;
    int len = mF->length;
    for(i=0; i<len*len; i++)
      {
      m_TransformMatrix[i] = static_cast<double>( mF->value[i] );
      }
    }
  if(!transformMatrixDefined)
    {
    for(i=0; i<m_NDims; i++)
      {
      m_TransformMatrix[i+i*m_NDims] = 1;
      }
    }

  mF = MET_GetFieldRecord("CenterOfRotation", &m_Fields);
  if(mF && mF->defined)
    {
    for(i=0; i<mF->length; i++)
      {
      m_CenterOfRotation[i] = static_cast<double>( mF->value[i] );
      }
    }
  else
    {
    for(i=0; i<m_NDims; i++)
      {
      m_CenterOfRotation[i] = 0;
      }
    }

  mF = MET_GetFieldRecord("DistanceUnits", &m_Fields);
  if(mF && mF->defined)
    {
    DistanceUnits((const char *)(mF->value));
    }

  mF = MET_GetFieldRecord("AnatomicalOrientation", &m_Fields);
  if(mF && mF->defined)
    {
    AnatomicalOrientation((const char *)(mF->value));
    }

  mF = MET_GetFieldRecord("ElementSpacing", &m_Fields);
  if(mF)
    {
    if(mF->defined)
      {
      for(i=0; i<mF->length && i < 10; i++)
        {
        m_ElementSpacing[i] = static_cast<float>( mF->value[i] );
        if (META_DEBUG)
          {
          METAIO_STREAM::cout << "metaObject: M_Read: elementSpacing["
                              << i << "] = "
                              << m_ElementSpacing[i] << METAIO_STREAM::endl;
          }
        }
      }
    else
      {
      for(i=0; i<mF->length && i < 10; i++)
        {
        m_ElementSpacing[i] = 1;
        if (META_DEBUG)
          {
          METAIO_STREAM::cout << "metaObject: M_Read: elementSpacing["
                              << i << "] = "
                              << m_ElementSpacing[i] << METAIO_STREAM::endl;
          }
        }
      }
    }

   // Set the read record field in the m_UserDefinedWriteFields
   FieldsContainerType::iterator  it  = m_UserDefinedReadFields.begin();
   FieldsContainerType::iterator  end = m_UserDefinedReadFields.end();
   while( it != end )
   {
     mF = MET_GetFieldRecord((*it)->name, &m_Fields);
     //
     // DON'T put the same cross-linked element from the UD readFields
     // into the userDefined write fields more than once. That
     // causes a double free, and an abort.
     FieldsContainerType::iterator dup;
     for(dup = m_UserDefinedWriteFields.begin();
         dup != m_UserDefinedWriteFields.end();
         ++dup)
       {
       if( (*dup) == mF )
         {
         break;
         }
       }
     if(dup == m_UserDefinedWriteFields.end())
       {
       m_UserDefinedWriteFields.push_back(mF);
       }
     ++it;
   }

  return true;
  }

bool MetaObject::
M_Write(void)
  {
  m_WriteStream->precision(m_DoublePrecision);

  if(!MET_Write(*m_WriteStream, & m_Fields))
    {
    METAIO_STREAM::cerr << "MetaObject: Write: MET_Write Failed"
                        << METAIO_STREAM::endl;
    return false;
    }

  return true;
  }


bool MetaObject
::Append(const char *_headName)
{
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaObject: Append" << METAIO_STREAM::endl;
    }

  if(_headName != NULL)
    {
    FileName(_headName);
    }

  M_SetupWriteFields();

  if(!m_WriteStream)
    {
    m_WriteStream = new METAIO_STREAM::ofstream;
    }

#ifdef __sgi
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::out
                                  | METAIO_STREAM::ios::in);
  if(!m_WriteStream->rdbuf()->is_open())
    {
    delete m_WriteStream;
    m_WriteStream = 0;
    return false;
    }
  m_WriteStream->seekp(0,METAIO_STREAM::ios::end);
#else
  m_WriteStream->open(m_FileName, METAIO_STREAM::ios::binary
                                  | METAIO_STREAM::ios::out
                                  | METAIO_STREAM::ios::app);
  if(!m_WriteStream->rdbuf()->is_open())
    {
    delete m_WriteStream;
    m_WriteStream = 0;
    return false;
    }
#endif

  M_Write();

  m_WriteStream->close();

  delete m_WriteStream;
  m_WriteStream = 0;
  return true;

}



// Get the user field
void* MetaObject
::GetUserField(const char* _name)
{
  FieldsContainerType::iterator  it  = m_UserDefinedWriteFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedWriteFields.end();
  while( it != end )
  {
    int eSize;
    MET_SizeOfType((*it)->type, &eSize);
    const unsigned int itLength =
                static_cast<unsigned int>( (*it)->length );
    void * out;
    if(!strcmp((*it)->name,_name))
      {
      if((*it)->type == MET_STRING)
        {
        out = (void*) (new char[(itLength+1)*eSize] );
        memcpy( out, (*it)->value, itLength * eSize );
        static_cast<char*>(out)[itLength]=0;
        }
      else if((*it)->type == MET_FLOAT_MATRIX)
        {
        const unsigned int numMatrixElements = itLength * itLength;
        out = (void*) (new char[numMatrixElements*eSize] );
        for( unsigned int i=0; i < numMatrixElements; i++ )
          {
          MET_DoubleToValue((*it)->value[i],(*it)->type,out,i);
          }
        }
      else
        {
        out = (void*) (new char[itLength*eSize] );
        for( unsigned int i=0; i < itLength; i++ )
          {
          MET_DoubleToValue((*it)->value[i],(*it)->type,out,i);
          }
        }
      return out;
      }
    ++it;
  }
  return NULL;
}

int MetaObject
::GetNumberOfAdditionalReadFields()
{
  return (int)(m_AdditionalReadFields.size());
}

char * MetaObject
::GetAdditionalReadFieldName( int i )
{
  return m_AdditionalReadFields[i]->name;
}

char * MetaObject
::GetAdditionalReadFieldValue( int i )
{
  return (char *)(m_AdditionalReadFields[i]->value);
}

int MetaObject
::GetAdditionalReadFieldValueLength( int i )
{
  return m_AdditionalReadFields[i]->length;
}

bool MetaObject
::AddUserField(const char* _fieldName,MET_ValueEnumType _type,int _length,
                        bool _required,int _dependsOn)
{
  MET_FieldRecordType* mFr = new MET_FieldRecordType;
  MET_InitReadField(mFr,_fieldName, _type, _required,_dependsOn,_length);
  m_UserDefinedReadFields.push_back(mFr);
  return 1;
}

void MetaObject::M_PrepareNewReadStream()
{
  if(m_ReadStream)
    {
    if(m_ReadStream->rdbuf()->is_open())
      {
      m_ReadStream->close();
      }
    m_ReadStream->clear();
    }
  else
    {
    m_ReadStream = new METAIO_STREAM::ifstream;
    }
}

#if (METAIO_USE_NAMESPACE)
};
#endif
