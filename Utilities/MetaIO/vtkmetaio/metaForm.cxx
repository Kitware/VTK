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
#pragma warning(disable:4996)
#endif

#include "metaForm.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0580)
#include <mem.h>
#endif

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


//
// MetaForm Constructors
//
MetaForm::
MetaForm()
{
  this->ClearUserFields();

  MetaForm::Clear();

  m_ReadStream = nullptr;
  m_WriteStream = nullptr;

  m_FileName[0] = '\0';

  m_Event = nullptr;

  m_DoublePrecision = 6;
}

MetaForm::
MetaForm(const char * _fileName)
{
  this->ClearUserFields();

  MetaForm::Clear();

  m_ReadStream = nullptr;
  m_WriteStream = nullptr;

  m_Event = nullptr;

  m_DoublePrecision = 6;

  this->Read(_fileName);
}


MetaForm::
~MetaForm()
{
  M_Destroy();

  if(m_ReadStream != nullptr)
    {
    delete m_ReadStream;
    m_ReadStream = nullptr;
    }
  if(m_WriteStream != nullptr)
    {
    delete m_WriteStream;
    m_WriteStream = nullptr;
    }

  this->ClearFields();
  this->ClearUserFields();
}

//
//
void MetaForm::
PrintInfo() const
{
  int i;

  std::cout << "ReadStream = "
                      << ((m_ReadStream==nullptr)?"NULL":"Set")
                      << std::endl;
  std::cout << "WriteStream = "
                      << ((m_WriteStream==nullptr)?"NULL":"Set")
                      << std::endl;

  std::cout << "FileName = _" << m_FileName << "_"
                      << std::endl;
  std::cout << "Comment = _" << m_Comment << "_"
                      << std::endl;
  std::cout << "FormTypeName = _" << m_FormTypeName << "_"
                      << std::endl;
  std::cout << "Name = " << m_Name << std::endl;
  if(m_BinaryData)
    {
    std::cout << "BinaryData = True" << std::endl;
    }
  else
    {
    std::cout << "BinaryData = False" << std::endl;
    }
  if(m_BinaryDataByteOrderMSB)
    {
    std::cout << "BinaryDataByteOrderMSB = True"
                        << std::endl;
    }
  else
    {
    std::cout << "BinaryDataByteOrderMSB = False"
                        << std::endl;
    }
  if(m_CompressedData)
    {
    std::cout << "CompressedData = True" << std::endl;
    }
  else
    {
    std::cout << "CompressedData = False" << std::endl;
    }
  std::cout << "DoublePrecision = " << m_DoublePrecision
                      << std::endl;
  std::cout << "Event = "
                      << ((m_Event==nullptr)?"NULL":"Set")
                      << std::endl;

  // Print User's fields :
  FieldsContainerType::const_iterator itw = m_UserDefinedWriteFields.begin();
  FieldsContainerType::const_iterator endw = m_UserDefinedWriteFields.end();
  FieldsContainerType::const_iterator itr = m_UserDefinedReadFields.begin();
  FieldsContainerType::const_iterator endr = m_UserDefinedReadFields.end();
  FieldsContainerType::const_iterator it;
  while( itw != endw )
    {
    bool skip = false;
    if((*itw)->defined)
      {
      it=itw;
      }
    else
      {
      it=itr;
      if (it != endr) // either a defined write field or a read field before reaching the end
        {
        skip = true;
        }
      }

    if (!skip)
      {
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
        std::cout << std::endl;
        for(i=0; i<(*it)->length*(*it)->length; i++)
          {
          printf("%f ",(*it)->value[i]);
          if(i==(*it)->length-1)
            {
            std::cout << std::endl;
            }
          }
        }
      std::cout << std::endl;
      }

    ++itw;
    if (itr != endr)
      {
      ++itr;
      }
    }
}

void MetaForm::
CopyInfo(const MetaForm * _form)
{
  FileName(_form->FileName());
  Comment(_form->Comment());
  FormTypeName(_form->FormTypeName());
  Name(_form->Name());
  BinaryData(_form->BinaryData());
  BinaryDataByteOrderMSB(_form->BinaryDataByteOrderMSB());
  CompressedData(_form->CompressedData());
  SetDoublePrecision(_form->GetDoublePrecision());
  // Const issue :( SetEvent(_form->GetEvent());
  // To do: copy user fields
}

void MetaForm::
Clear()
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: Clear()" << std::endl;
    }

  // Preserve m_FileName

  strcpy(m_Comment, "");
  strcpy(m_FormTypeName, "Form");
  strcpy(m_Name, "");

  m_BinaryData = false;
  m_BinaryDataByteOrderMSB = MET_SystemByteOrderMSB();
  m_CompressedData = false;

  this->ClearFields();
}

//
//
// Clear Fields only, if the pointer is in the UserField list it is not deleted.
void MetaForm::
ClearFields()
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm:ClearFields" << std::endl;
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

bool MetaForm::
InitializeEssential()
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: Initialize" << std::endl;
    }

  M_Destroy();

  return true;
}

const char * MetaForm::
FileName() const
{
  return m_FileName.c_str();
}

void MetaForm::
FileName(const char *_fileName)
{
  if(_fileName != nullptr)
    {
    m_FileName = _fileName;
    }
  else
    {
    m_FileName = "";
    }
}

const char * MetaForm::
Comment() const
{
  return m_Comment;
}

void MetaForm::
Comment(const char * _comment)
{
  if(_comment != nullptr)
    {
    strcpy(m_Comment, _comment);
    }
  else
    {
    m_Comment[0] = '\0';
    }
}

const char * MetaForm::
FormTypeName() const
{
  return m_FormTypeName;
}

void MetaForm::
FormTypeName(const char * _formTypeName)
{
  if(_formTypeName != nullptr)
    {
    strcpy(m_FormTypeName, _formTypeName);
    }
  else
    {
    m_FormTypeName[0] = '\0';
    }
}

const char  * MetaForm::
Name() const
{
  return m_Name;
}

void  MetaForm::
Name(const char *_Name)
{
  if(_Name != nullptr)
    {
    strcpy(m_Name, _Name);
    }
  else
    {
    m_Name[0] = '\0';
    }
}


bool MetaForm::
BinaryData() const
{
  return m_BinaryData;
}

void  MetaForm::
BinaryData(bool _binaryData)
{
  m_BinaryData = _binaryData;
}

bool MetaForm::
BinaryDataByteOrderMSB() const
{
  return m_BinaryDataByteOrderMSB;
}

void MetaForm::
BinaryDataByteOrderMSB(bool _elementByteOrderMSB)
{
  m_BinaryDataByteOrderMSB = _elementByteOrderMSB;
}

bool MetaForm::
CompressedData() const
{
  return m_CompressedData;
}

void MetaForm::
CompressedData(bool _compressedData)
{
  m_CompressedData = _compressedData;
}

unsigned int MetaForm::
DoublePrecision() const
{
  return m_DoublePrecision;
}

void MetaForm::
DoublePrecision(unsigned int _doublePrecision)
{
  m_DoublePrecision = _doublePrecision;
}

MetaEvent * MetaForm::
Event()
{
  return m_Event;
}

void MetaForm::
Event(MetaEvent * _event)
{
  m_Event =_event;
}

//
// Clear UserFields
//
void MetaForm::
ClearUserFields()
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

// Get the user field
void* MetaForm::
GetUserField(const char* _name)
{
  FieldsContainerType::iterator  it  = m_UserDefinedWriteFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedWriteFields.end();
  while( it != end )
    {
    int eSize;
    MET_SizeOfType((*it)->type, &eSize);
    const unsigned int itLength =
                static_cast<unsigned int>( (*it)->length );
    char * out;
    if(!strcmp((*it)->name,_name))
      {
      if((*it)->type == MET_STRING)
        {
        out = new char[(itLength+1)*eSize];
        memcpy( out, (*it)->value, itLength * eSize );
        out[itLength]=0;
        }
      else if((*it)->type == MET_FLOAT_MATRIX)
        {
        const unsigned int numMatrixElements = itLength * itLength;
        out = new char[numMatrixElements*eSize];
        for( unsigned int i=0; i < numMatrixElements; i++ )
          {
          MET_DoubleToValue((*it)->value[i],(*it)->type,out,i);
          }
        }
      else
        {
        out = new char[itLength*eSize];
        for( unsigned int i=0; i < itLength; i++ )
          {
          MET_DoubleToValue((*it)->value[i],(*it)->type,out,i);
          }
        }
      return out;
      }
    ++it;
    }
  return nullptr;
}

bool MetaForm::
CanRead(const char *_fileName) const
{
  if(_fileName)
    {
    return false;
    }
  else
    {
    return false;
    }
}

bool MetaForm::
Read(const char *_fileName)
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: Read" << std::endl;
    }

  if(_fileName != nullptr)
    {
    m_FileName = _fileName;
    }

  std::cout << "Read FileName = _" << m_FileName << "_"
                      << std::endl;

  std::ifstream * tmpReadStream = new std::ifstream;
#ifdef __sgi
  tmpReadStream->open(m_FileName, std::ios::in);
#else
  tmpReadStream->open(m_FileName, std::ios::binary |
                                  std::ios::in);
#endif

  if(!tmpReadStream->rdbuf()->is_open())
    {
    std::cout << "MetaForm: Read: Cannot open file"
                        << std::endl;
    delete tmpReadStream;
    return false;
    }

  bool result = this->ReadStream(tmpReadStream);

  // ensure filename is not changed
  if(_fileName != nullptr)
    {
    m_FileName =_fileName;
    }

  tmpReadStream->close();

  delete tmpReadStream;

  return result;
}

bool MetaForm::
CanReadStream(std::ifstream * _stream) const
{
  if(_stream)
    {
    return false;
    }
  else
    {
    return false;
    }
}

bool MetaForm::
ReadStream(std::ifstream * _stream)
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: ReadStream" << std::endl;
    }

  M_Destroy();

  fflush(nullptr);

  Clear();

  M_SetupReadFields();

  delete m_ReadStream;

  m_ReadStream = _stream;

  bool result = M_Read();

  m_ReadStream= nullptr;

  return result;
}


bool MetaForm::
Write(const char *_fileName)
{
  if(_fileName != nullptr)
    {
    FileName(_fileName);
    }

  std::cout << "Write FileName = _" << m_FileName << "_"
                      << std::endl;

  std::ofstream * tmpWriteStream = new std::ofstream;

#ifdef __sgi
{
  // Create the file. This is required on some older sgi's
  std::ofstream tFile(m_FileName, std::ios::out);
  tFile.close();
}
  tmpWriteStream->open(m_FileName, std::ios::out);
#else
  tmpWriteStream->open(m_FileName, std::ios::binary |
                                  std::ios::out);
#endif

  if(!tmpWriteStream->rdbuf()->is_open())
    {
    delete tmpWriteStream;
    std::cout << "Write failed." << std::endl;
    return false;
    }

  bool result = WriteStream(tmpWriteStream);

  tmpWriteStream->close();

  delete tmpWriteStream;

  return result;
}

bool MetaForm::
WriteStream(std::ofstream * _stream)
{
  M_SetupWriteFields();

  m_WriteStream = _stream;

  bool result = M_Write();

  m_WriteStream = nullptr;

  return result;
}

void MetaForm::
M_Destroy()
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: Destroy" << std::endl;
    }
}

void MetaForm::
M_SetupReadFields()
{
  this->ClearFields();
  if(META_DEBUG)
    {
    std::cout << "MetaForm: M_SetupReadFields"
                        << std::endl;
    }

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Comment", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "FormTypeName", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Name", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "BinaryData", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "BinaryDataByteOrderMSB", MET_STRING, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "CompressedData", MET_STRING, false);
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


void MetaForm::
M_SetupWriteFields()
{
  if(META_DEBUG)
    {
    std::cout << "MetaForm: M_SetupWriteFields"
                        << std::endl;
    }

  this->ClearFields();

  if(META_DEBUG)
    {
    std::cout << "MetaForm: M_SetupWriteFields: Creating Fields"
                        << std::endl;
    }

  MET_FieldRecordType * mF;

  if(strlen(m_Comment)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Comment", MET_STRING, strlen(m_Comment), m_Comment);
    m_Fields.push_back(mF);
    }

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "FormTypeName", MET_STRING, strlen(m_FormTypeName),
                    m_FormTypeName);
  m_Fields.push_back(mF);

  if(strlen(m_Name)>0)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "Name", MET_STRING, strlen(m_Name),m_Name);
    m_Fields.push_back(mF);
    }

  if(m_CompressedData)
    {
    m_BinaryData = true;
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
    }
  else
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "BinaryData", MET_STRING, strlen("False"), "False");
    m_Fields.push_back(mF);
    }

  if(m_CompressedData)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "CompressedData", MET_STRING, strlen("True"),
                       "True");
    m_Fields.push_back(mF);
    }

  // Add User's field
  FieldsContainerType::iterator  it  = m_UserDefinedWriteFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedWriteFields.end();
  while( it != end )
    {
    m_Fields.push_back(*it);
    ++it;
    }
}

bool MetaForm::
M_Read()
{

  if(!MET_Read(*m_ReadStream, & m_Fields))
    {
    std::cout << "MetaForm: Read: MET_Read Failed"
                        << std::endl;
    return false;
    }

  MetaForm::InitializeEssential();

  MET_FieldRecordType * mF;

  mF = MET_GetFieldRecord("Comment", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_Comment, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("FormTypeName", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_FormTypeName, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("Name", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_Name, (char *)(mF->value));
    }

  mF = MET_GetFieldRecord("BinaryData",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' ||
       ((char *)(mF->value))[0] == 't' ||
       ((char *)(mF->value))[0] == '1')
      {
      m_BinaryData = true;
      }
    else
      {
      m_BinaryData = false;
      }
    }
  else
    {
    m_BinaryData = false;
    }

  mF = MET_GetFieldRecord("BinaryDataByteOrderMSB",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' ||
       ((char *)(mF->value))[0] == 't' ||
       ((char *)(mF->value))[0] == '1')
      {
      m_BinaryDataByteOrderMSB = true;
      }
    else
      {
      m_BinaryDataByteOrderMSB = false;
      }
    }

  mF = MET_GetFieldRecord("CompressedData",  &m_Fields);
  if(mF && mF->defined)
    {
    if(((char *)(mF->value))[0] == 'T' ||
       ((char *)(mF->value))[0] == 't' ||
       ((char *)(mF->value))[0] == '1')
      {
      m_CompressedData = true;
      }
    else
      {
      m_CompressedData = false;
      }
    }
  else
    {
    m_CompressedData = false;
    }

  // Set the read record field in the m_UserDefinedWriteFields
  FieldsContainerType::iterator  it  = m_UserDefinedReadFields.begin();
  FieldsContainerType::iterator  end = m_UserDefinedReadFields.end();
  while( it != end )
    {
    mF = MET_GetFieldRecord((*it)->name, &m_Fields);
    m_UserDefinedWriteFields.push_back(mF);
    ++it;
    }

  return true;
}

bool MetaForm::
M_Write()
{
  m_WriteStream->precision(m_DoublePrecision);

  if(!MET_Write(*m_WriteStream, & m_Fields))
    {
    std::cout << "MetaForm: Write: MET_Write Failed"
                        << std::endl;
    return false;
    }

  m_WriteStream->flush();

  return true;
}

#if (METAIO_USE_NAMESPACE)
};
#endif
