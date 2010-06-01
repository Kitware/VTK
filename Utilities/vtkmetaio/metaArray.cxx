/*============================================================================
  MetaIO
  Copyright 2000-2010 Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "metaArray.h"

#ifdef _MSC_VER
#pragma warning(disable:4702)
#pragma warning(disable:4996)
#endif

#include <stdio.h>
#include <ctype.h>
#include <string>
#include <string.h> // for memcpy
#include <math.h>

#if (METAIO_USE_NAMESPACE)
namespace METAIO_NAMESPACE {
#endif


//
// MetaArray Constructors
//
MetaArray::
MetaArray()
:MetaForm()
  {
  if(META_DEBUG) 
    {
    METAIO_STREAM::cout << "MetaArray()" << METAIO_STREAM::endl;
    }

  m_ElementData = NULL;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  strcpy(m_ElementDataFileName, "");

  MetaArray::Clear();
  }

//
MetaArray::
MetaArray(const char *_headerName)
:MetaForm()
  {
  if(META_DEBUG) 
    {
    METAIO_STREAM::cout << "MetaArray()" << METAIO_STREAM::endl;
    }

  m_ElementData = NULL;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  strcpy(m_ElementDataFileName, "");

  MetaArray::Clear();

  MetaArray::Read(_headerName);
  }

//
MetaArray::
MetaArray(MetaArray *_vector,
          bool _allocateElementData,
          bool _autoFreeElementData)
:MetaForm()
  {
  if(META_DEBUG)
   {
   METAIO_STREAM::cout << "MetaArray()" << METAIO_STREAM::endl;
   }

  m_ElementData = NULL;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  strcpy(m_ElementDataFileName, "");

  MetaArray::Clear();

  InitializeEssential(_vector->Length(), 
                      _vector->ElementType(),
                      _vector->ElementNumberOfChannels(),
                      _vector->ElementData(),
                      _allocateElementData,
                      _autoFreeElementData);

  CopyInfo(_vector);
  }

//
MetaArray::
MetaArray(int _length, 
          MET_ValueEnumType _elementType,
          int _elementNumberOfChannels,
          void *_elementData,
          bool _allocateElementData,
          bool _autoFreeElementData)
:MetaForm()
  {
  if(META_DEBUG)
   {
   METAIO_STREAM::cout << "MetaArray()" << METAIO_STREAM::endl;
   }

  m_ElementData = NULL;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  strcpy(m_ElementDataFileName, "");

  MetaArray::Clear();

  InitializeEssential(_length, 
                      _elementType,
                      _elementNumberOfChannels,
                      _elementData,
                      _allocateElementData,
                      _autoFreeElementData);
  }

//
MetaArray::
~MetaArray()
  {
  M_Destroy();
  }

//
void MetaArray::
PrintInfo() const
  {
  MetaForm::PrintInfo();

  METAIO_STREAM::cout << "Length = " << (int)m_Length << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "BinaryData = " 
                      << ((m_BinaryData)?"True":"False") 
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "BinaryDataByteOrderMSB = " 
                      << ((m_BinaryDataByteOrderMSB)?"True":"False") 
                      << METAIO_STREAM::endl;

  char str[255];
  MET_TypeToString(m_ElementType, str);
  METAIO_STREAM::cout << "ElementType = " << str << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementNumberOfChannels = " 
                      << m_ElementNumberOfChannels
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "AutoFreeElementData = " 
                      << ((m_AutoFreeElementData)?"True":"False") 
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "CompressedElementDataSize = " 
                      << m_CompressedElementDataSize
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementDataFileName = " << m_ElementDataFileName
                      << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "ElementData = " 
                      << ((m_ElementData==NULL)?"NULL":"Valid")
                      << METAIO_STREAM::endl;
  }

void MetaArray::
CopyInfo(const MetaForm * _form)
  {
  MetaForm::CopyInfo(_form);
  }

void MetaArray::
Clear(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: Clear" << METAIO_STREAM::endl;
    }

  m_Length = 0;

  m_ElementType = MET_NONE;

  m_ElementNumberOfChannels = 1;

  m_CompressedElementDataSize = 0;

  strcpy(m_ElementDataFileName, "");

  if(m_AutoFreeElementData)
    {
    if(m_ElementData != NULL)
      {
      delete [] (char *)m_ElementData;
      }
    }
  m_ElementData = NULL;
  m_AutoFreeElementData = true;

  MetaForm::Clear();
  }

bool MetaArray::
InitializeEssential(int _length,
                    MET_ValueEnumType _elementType,
                    int _elementNumberOfChannels,
                    void * _elementData,
                    bool _allocateElementData,
                    bool _autoFreeElementData)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: Initialize" << METAIO_STREAM::endl;
    }

  MetaForm::InitializeEssential();

  bool result = true;

  if(m_Length != _length ||
     m_ElementType != _elementType ||
     m_ElementNumberOfChannels != _elementNumberOfChannels ||
     _elementData != NULL ||
     _allocateElementData == true)
    {
    if(m_AutoFreeElementData)
      {
      if(m_ElementData != NULL)
        {
        delete [] (char *)m_ElementData; 
        }
      }
    m_ElementData = NULL;

    m_Length = _length;
    m_ElementType = _elementType;
    m_ElementNumberOfChannels = _elementNumberOfChannels;

    if(_elementData != NULL)
      {
      m_ElementData = (void *)_elementData;
      }
    else
      {
      if(_allocateElementData)
        {
        result = AllocateElementData(_autoFreeElementData);
        }
      else
        {
        m_ElementData = NULL;
        }
      }

    m_AutoFreeElementData = _autoFreeElementData;
    }

  return result;
  }

bool MetaArray::
AllocateElementData(bool _autoFreeElementData)
  {
  if(m_AutoFreeElementData)
    {
    if(m_ElementData != NULL)
      {
      delete [] (char *)m_ElementData;
      }
    }
  m_ElementData = NULL;

  m_AutoFreeElementData = _autoFreeElementData;

  int eSize;
  MET_SizeOfType(m_ElementType, &eSize);
  m_ElementData = new char[m_Length*m_ElementNumberOfChannels*eSize];

  if(m_ElementData != NULL)
    {
    return true;
    }
  else
    {
    return false;
    }
  }
        
int MetaArray::
Length(void) const
  {
  return m_Length;
  }

void MetaArray::
Length(int _length)
  {
  if(m_Length != _length)
    {
    InitializeEssential(_length, m_ElementType, m_ElementNumberOfChannels);
    }
  }

int MetaArray::
NDims(void) const
  {
  return Length();
  }

void MetaArray::
NDims(int _length)
  {
  Length(_length);
  }

MET_ValueEnumType MetaArray::
ElementType(void) const
  {
  return m_ElementType;
  }

void MetaArray::
ElementType(MET_ValueEnumType _elementType)
  {
  if(m_ElementType != _elementType)
    {
    InitializeEssential(m_Length, _elementType, m_ElementNumberOfChannels);
    }
  }

int MetaArray::
ElementNumberOfChannels(void) const
  {
  return m_ElementNumberOfChannels;
  }

void MetaArray::
ElementNumberOfChannels(int _elementNumberOfChannels)
  {
  if(m_ElementNumberOfChannels != _elementNumberOfChannels)
    {
    InitializeEssential(m_Length, m_ElementType, _elementNumberOfChannels);
    }
  }

void MetaArray::
ElementByteOrderSwap(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: ElementByteOrderSwap" 
                        << METAIO_STREAM::endl;
    }

  int eSize;
  MET_SizeOfType(m_ElementType, &eSize);    
  switch(eSize)
    {
    default:
    case 0:
    case 1: 
      {
      break;
      }
    case 2:
      {
      int i;
      for(i=0; i<m_Length*m_ElementNumberOfChannels; i++)
        {
        ((MET_USHORT_TYPE *)m_ElementData)[i] = 
              MET_ByteOrderSwapShort(((MET_USHORT_TYPE *)m_ElementData)[i]);
        }
      break;
      }
    case 4:
      {
      int i;
      for(i=0; i<m_Length*m_ElementNumberOfChannels; i++)
        {
        ((MET_UINT_TYPE *)m_ElementData)[i] =
              MET_ByteOrderSwapLong(((MET_UINT_TYPE *)m_ElementData)[i]);
        }
      break;
      }
    case 8:
      {
      int i;
      char* data = (char*)m_ElementData;
      for(i=0; i<m_Length*m_ElementNumberOfChannels; i++)
        {
        MET_ByteOrderSwap8(data);
        data += 8;
        }
      break;
      }
    }
  m_BinaryDataByteOrderMSB = !m_BinaryDataByteOrderMSB;
  }

bool MetaArray::
ElementByteOrderFix(void)
  {
  if(m_BinaryDataByteOrderMSB != MET_SystemByteOrderMSB())
    {
    ElementByteOrderSwap();
    return true;
    }
  return true;
  }

bool MetaArray::
ConvertElementDataTo(MET_ValueEnumType _toElementType,
                     double _fromMin, double _fromMax,
                     double _toMin, double _toMax)
  {
  if(m_ElementData == NULL)
    {
    return false;
    }

  ElementByteOrderFix();

  void *            curBuffer              = m_ElementData;
  MET_ValueEnumType curElementType         = m_ElementType;
  bool              curAutoFreeElementData = m_AutoFreeElementData;

  if(m_ElementType != _toElementType)
    {
    m_ElementData = NULL;
    m_ElementType = _toElementType;
    }

  ImportBufferToElementData(curBuffer, curElementType,
                            _fromMin, _fromMax,
                            _toMin, _toMax );

  if(m_ElementType != _toElementType)
    {
    if(curAutoFreeElementData)
      {
      delete [] (char *)curBuffer;
      }
    }

  return true;
  }

bool MetaArray::
ImportBufferToElementData(const void * _fromBuffer,
                          MET_ValueEnumType _fromElementType,
                          double _fromMin, double _fromMax,
                          double _toMin, double _toMax)
  {
  if(m_ElementData == NULL)
    {
    AllocateElementData(true);
    }

  if(_fromMin == 0 && _fromMax == 0)
    {
    MET_ValueToDouble(_fromElementType, _fromBuffer, 0, &_fromMin);
    _fromMax = _fromMin;

    int i;
    for(i=0; i<m_Length*m_ElementNumberOfChannels; i++)
      {
      double tf;
      MET_ValueToDouble(_fromElementType, _fromBuffer, i, &tf);
      if(tf < _fromMin)
        {
        _fromMin = tf;
        }
      else if(tf > _fromMax)
        {
        _fromMax = tf;
        }
      }
    }

  if(_toMin == 0 && _toMax == 0)
    {
    _toMin = _fromMin;
    _toMax = _fromMax;
    }

  for(int i=0; i<m_Length*m_ElementNumberOfChannels; i++)
    {
    MET_ValueToValue(_fromElementType, _fromBuffer, i,
                     m_ElementType, m_ElementData,
                     _fromMin, _fromMax,
                     _toMin, _toMax);
    }

  return true;
  }


//
//
//
bool MetaArray::
AutoFreeElementData(void) const
  {
  return m_AutoFreeElementData;
  }

void MetaArray::
AutoFreeElementData(bool _autoFreeElementData)
  {
  m_AutoFreeElementData = _autoFreeElementData;
  }


//
//
//
const char * MetaArray::
ElementDataFileName(void) const
  {
  return m_ElementDataFileName;
  }

void MetaArray::
ElementDataFileName(const char * _elementDataFileName)
  {
  strcpy(m_ElementDataFileName, _elementDataFileName);
  }


//
//
//
void * MetaArray::
ElementData(void)
  {
  return m_ElementData;
  }

double MetaArray::
ElementData(int _i) const
  {
  double tf = 0;
  MET_ValueToDouble(m_ElementType, m_ElementData, _i, &tf);

  return tf;
  }

void MetaArray::
ElementData(void * _elementData, bool _arrayControlsElementData)
  {
  if(m_AutoFreeElementData)
    {
    delete [] (char *)m_ElementData;
    }
  m_ElementData = _elementData;
  m_AutoFreeElementData = _arrayControlsElementData;
  }

bool MetaArray::
ElementData(int _i, double _v)
  {
  if(_i<m_Length*m_ElementNumberOfChannels)
    {
    MET_DoubleToValue(_v, m_ElementType, m_ElementData, _i);
    return true;
    }
  return false;
  }


//
//
//
bool MetaArray::
CanRead(const char *_headerName) const
  {
  // First check the extension
  METAIO_STL::string fname = _headerName;
  if( fname == "" )
    {
    return false;
    }

  bool extensionFound = false;

  METAIO_STL::string::size_type stringPos = fname.rfind(".mva");
  if ((stringPos != METAIO_STL::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }

  stringPos = fname.rfind(".mvh");
  if ((stringPos != METAIO_STL::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }
  
  if( !extensionFound )
    {
    return false;
    }

  // Now check the file content
  METAIO_STREAM::ifstream inputStream;

#ifdef __sgi
  inputStream.open( _headerName, METAIO_STREAM::ios::in );
#else
  inputStream.open( _headerName, METAIO_STREAM::ios::in |
                                 METAIO_STREAM::ios::binary );
#endif

  if( !inputStream.rdbuf()->is_open() )
    {
    return false;
    }

  bool result = !strncmp(MET_ReadForm(inputStream).c_str(), "Array", 5);

  inputStream.close();

  return result;
  }


bool MetaArray::
Read(const char *_headerName, bool _readElements,
     void * _elementDataBuffer, bool _autoFreeElementData)
  {
  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  METAIO_STREAM::ifstream * tmpStream = new METAIO_STREAM::ifstream;

#ifdef __sgi
  tmpStream->open(m_FileName, METAIO_STREAM::ios::in );
#else
  tmpStream->open(m_FileName, METAIO_STREAM::ios::in |
                              METAIO_STREAM::ios::binary);
#endif

  if(!tmpStream->rdbuf()->is_open())
    {
    METAIO_STREAM::cout << "MetaArray: Read: Cannot open file _" 
                        << m_FileName << "_" << METAIO_STREAM::endl;
    delete tmpStream;
    return false;
    }

  bool result = ReadStream(tmpStream, _readElements, 
                           _elementDataBuffer, _autoFreeElementData);

  if(_headerName != NULL)
    {
    strcpy(m_FileName, _headerName);
    }

  tmpStream->close();

  delete tmpStream;

  return result;
  }


bool MetaArray::
CanReadStream(METAIO_STREAM::ifstream * _stream) const
  {
  if(!strncmp(MET_ReadForm(*_stream).c_str(), "Array", 5))
    {
    return true;
    }
  return false;
  }

bool MetaArray::
ReadStream(METAIO_STREAM::ifstream * _stream, bool _readElements,
           void * _elementDataBuffer, bool _autoFreeElementData)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: ReadStream" << METAIO_STREAM::endl;
    }

  M_Destroy();

  Clear();

  M_SetupReadFields();

  if(m_ReadStream)
    {
    METAIO_STREAM::cout << "MetaArray: ReadStream: two files open?" 
                        << METAIO_STREAM::endl;
    delete m_ReadStream;
    }

  m_ReadStream = _stream;

  if(!M_Read())
    {
    METAIO_STREAM::cout << "MetaArray: Read: Cannot parse file" 
                        << METAIO_STREAM::endl;
    m_ReadStream = NULL;
    return false;
    }

  InitializeEssential(m_Length, 
                      m_ElementType, 
                      m_ElementNumberOfChannels, 
                      _elementDataBuffer,
                      true,
                      _autoFreeElementData);

  bool usePath;
  char pathName[255];
  char fName[255];
  usePath = MET_GetFilePath(m_FileName, pathName);

  if(_readElements)
    {
    if(!strcmp("Local", m_ElementDataFileName) || 
       !strcmp("LOCAL", m_ElementDataFileName) ||
       !strcmp("local", m_ElementDataFileName))
      {
      M_ReadElements(m_ReadStream, m_ElementData, m_Length);
      }
    else
      {
      if(usePath)
        {
        sprintf(fName, "%s%s", pathName, m_ElementDataFileName);
        }
      else
        {
        strcpy(fName, m_ElementDataFileName);
        }
      METAIO_STREAM::ifstream* readStreamTemp = new METAIO_STREAM::ifstream;

#ifdef __sgi
      readStreamTemp->open(fName, METAIO_STREAM::ios::in);
#else
      readStreamTemp->open(fName, METAIO_STREAM::ios::binary |
                                  METAIO_STREAM::ios::in);
#endif
      if(!readStreamTemp->rdbuf()->is_open())
        {
        METAIO_STREAM::cout << "MetaArray: Read: Cannot open data file" 
                            << METAIO_STREAM::endl;
        m_ReadStream = NULL;
        return false;
        }
      if(_readElements)
        {
        M_ReadElements(readStreamTemp, m_ElementData, m_Length);
        }
      readStreamTemp->close();
      delete readStreamTemp;
      }
    }

  m_ReadStream = NULL;

  return true;
  }

//
//
//
//

bool MetaArray::
Write(const char *_headName, const char *_dataName, bool _writeElements,
      const void *_constElementData)
  {
  if(_headName != NULL && strlen(_headName)>1)
    {
    FileName(_headName);
    }

  bool tmpDataFileName = false;
  if( _dataName != NULL &&
      strlen(_dataName) > 1 )
    {
    tmpDataFileName = true;
    ElementDataFileName(_dataName);
    }
  else
    {
    if( strlen(m_ElementDataFileName) == 0 )
      {
      tmpDataFileName = true;
      }
    }

  int sPtr = 0;
  MET_GetFileSuffixPtr(m_FileName, &sPtr);
  if( !strcmp(&(m_FileName[sPtr]), "mvh") )
    {
    MET_SetFileSuffix(m_FileName, "mvh");
    if( strlen(m_ElementDataFileName) == 0 ||
        !strcmp(m_ElementDataFileName, "LOCAL") )
      {
      ElementDataFileName(m_FileName);
      }
    if(m_CompressedData)
      {
      MET_SetFileSuffix(m_ElementDataFileName, "zmvd");
      }
    else
      {
      MET_SetFileSuffix(m_ElementDataFileName, "mvd");
      }
    }
  else
    {
    MET_SetFileSuffix(m_FileName, "mva");
    ElementDataFileName("LOCAL");
    }

  char pathName[255];
  bool usePath = MET_GetFilePath(m_FileName, pathName);
  if(usePath)
    {
    char elementPathName[255];
    MET_GetFilePath(m_ElementDataFileName, elementPathName);
    if(!strcmp(pathName, elementPathName))
      {
      strcpy(elementPathName, &m_ElementDataFileName[strlen(pathName)]);
      strcpy(m_ElementDataFileName, elementPathName);
      }
    }

  METAIO_STREAM::ofstream * tmpWriteStream = new METAIO_STREAM::ofstream;

// Some older sgi compilers have a error in the ofstream constructor
// that requires a file to exist for output
#ifdef __sgi
  {
  METAIO_STREAM::ofstream tFile(m_FileName, METAIO_STREAM::ios::out);
  tFile.close();                    
  }
  tmpWriteStream->open(m_FileName, METAIO_STREAM::ios::out);
#else
  tmpWriteStream->open(m_FileName, METAIO_STREAM::ios::binary |
                                   METAIO_STREAM::ios::out);
#endif

  if(!tmpWriteStream->rdbuf()->is_open())
    {
    if(tmpDataFileName)
      {
      ElementDataFileName("");
      }
    delete tmpWriteStream;
    return false;
    }

  bool result = WriteStream(tmpWriteStream, _writeElements, _constElementData);

  if(tmpDataFileName)
    {
    ElementDataFileName("");
    }

  tmpWriteStream->close();

  delete tmpWriteStream;

  return result;
  }

bool MetaArray::
WriteStream(METAIO_STREAM::ofstream * _stream, bool _writeElements,
            const void *_constElementData)
  {
  if(m_WriteStream != NULL)
    {
    METAIO_STREAM::cout << "MetaArray: WriteStream: two files open?" 
                        << METAIO_STREAM::endl;
    delete m_WriteStream;
    }

  m_WriteStream = _stream;

  unsigned char * compressedElementData = NULL;
  if(m_CompressedData)
    {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

    if(_constElementData == NULL)
      {
      compressedElementData = MET_PerformCompression( 
                                    (const unsigned char *)m_ElementData,
                                    m_Length * elementNumberOfBytes,
                                    & m_CompressedElementDataSize);
      }
    else
      {
      compressedElementData = MET_PerformCompression( 
                                    (const unsigned char *)_constElementData,
                                    m_Length * elementNumberOfBytes,
                                    & m_CompressedElementDataSize);
      }
    }

  M_SetupWriteFields();

  M_Write();

  if(_writeElements)
    {
    if(m_CompressedData)
      {
      M_WriteElements(m_WriteStream,
                      compressedElementData,
                      m_CompressedElementDataSize);

      delete [] compressedElementData;
      }
    else
      {
      int elementSize;
      MET_SizeOfType(m_ElementType, &elementSize);
      int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

      if(_constElementData != NULL)
        {
        M_WriteElements(m_WriteStream,
                        _constElementData,
                        elementNumberOfBytes * m_Length);
        }
      else
        {
        M_WriteElements(m_WriteStream,
                        m_ElementData,
                        elementNumberOfBytes * m_Length);
        }
      }
    }

  m_WriteStream->flush();

  m_WriteStream = NULL;

  return true;
  }
        
void MetaArray::
M_Destroy(void)
  {
  if(m_AutoFreeElementData && m_ElementData != NULL)
    {
    delete [] (char *)m_ElementData; 
    }
  m_ElementData = NULL;

  MetaForm::M_Destroy();
  }

void MetaArray::
M_SetupReadFields(void)
  {
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: M_SetupReadFields" 
                        << METAIO_STREAM::endl;
    }

  MetaForm::M_SetupReadFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "Length", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "NDims", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementNumberOfChannels", MET_INT, false);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementType", MET_STRING, true);
  mF->required = true;
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitReadField(mF, "ElementDataFile", MET_STRING, true);
  mF->required = true;
  mF->terminateRead = true;
  m_Fields.push_back(mF);
  }

void MetaArray::
M_SetupWriteFields(void)
  {
  strcpy(m_FormTypeName, "Array");
  MetaForm::M_SetupWriteFields();

  MET_FieldRecordType * mF;

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "Length", MET_INT, m_Length);
  m_Fields.push_back(mF);

  if(m_ElementNumberOfChannels>1)
    {
    mF = new MET_FieldRecordType;
    MET_InitWriteField(mF, "ElementNumberOfChannels", MET_INT, 
                       m_ElementNumberOfChannels);
    m_Fields.push_back(mF);
    }

  char s[80];
  mF = new MET_FieldRecordType;
  MET_TypeToString(m_ElementType, s);
  MET_InitWriteField(mF, "ElementType", MET_STRING, strlen(s), s);
  m_Fields.push_back(mF);

  mF = new MET_FieldRecordType;
  MET_InitWriteField(mF, "ElementDataFile", MET_STRING, 
                     strlen(m_ElementDataFileName),
                     m_ElementDataFileName);
  mF->terminateRead = true;
  m_Fields.push_back(mF);
  }


bool MetaArray::
M_Read(void)
  {
  if(META_DEBUG) 
    {
    METAIO_STREAM::cout << "MetaArray: M_Read: Loading Header" 
                        << METAIO_STREAM::endl;
    }
  if(!MetaForm::M_Read())
    {
    METAIO_STREAM::cout << "MetaArray: M_Read: Error parsing file" 
                        << METAIO_STREAM::endl;
    return false;
    }

  if(META_DEBUG) 
    {
    METAIO_STREAM::cout << "MetaArray: M_Read: Parsing Header" 
                        << METAIO_STREAM::endl;
    }
  MET_FieldRecordType * mF;
     
  mF = MET_GetFieldRecord("Length", &m_Fields);
  if(mF && mF->defined)
    {
    m_Length = (int)mF->value[0];
    }
  else
    {
    mF = MET_GetFieldRecord("NDims", &m_Fields);
    if(mF && mF->defined)
      {
      m_Length = (int)mF->value[0];
      }
    else
      {
      METAIO_STREAM::cout << "MetaArray: M_Read: Error: Length required"
                          << METAIO_STREAM::endl;
      return false;
      }
    }

  mF = MET_GetFieldRecord("ElementNumberOfChannels", &m_Fields);
  if(mF && mF->defined)
    {
    m_ElementNumberOfChannels = (int)mF->value[0];
    }

  mF = MET_GetFieldRecord("ElementType", &m_Fields);
  if(mF && mF->defined)
    {
    MET_StringToType((char *)(mF->value), &m_ElementType);
    }

  mF = MET_GetFieldRecord("ElementDataFile", &m_Fields);
  if(mF && mF->defined)
    {
    strcpy(m_ElementDataFileName, (char *)(mF->value));
    }

  return true;
  }

//
//
//
bool MetaArray::
M_ReadElements(METAIO_STREAM::ifstream * _fstream, void * _data,
               int _dataQuantity)
  {
  if(META_DEBUG) 
    {
    METAIO_STREAM::cout << "MetaArray: M_ReadElements" << METAIO_STREAM::endl;
    }

  int elementSize;
  MET_SizeOfType(m_ElementType, &elementSize);
  int readSize = _dataQuantity*m_ElementNumberOfChannels*elementSize;
  if(META_DEBUG)
    {
    METAIO_STREAM::cout << "MetaArray: M_ReadElements: ReadSize = " 
                        << readSize << METAIO_STREAM::endl;
    }

  // If compressed we inflate
  if(m_CompressedData)
    {
    // if m_CompressedElementDataSize is not defined we assume the size of the
    // file is the size of the compressed data
    if(m_CompressedElementDataSize==0)
      {
      _fstream->seekg(0, METAIO_STREAM::ios::end);
      m_CompressedElementDataSize = _fstream->tellg();
      _fstream->seekg(0, METAIO_STREAM::ios::beg);
      }

    unsigned char* compr = new unsigned char[m_CompressedElementDataSize];
    _fstream->read((char *)compr, (size_t)m_CompressedElementDataSize);
    
    MET_PerformUncompression(compr, m_CompressedElementDataSize,
                             (unsigned char *)_data, readSize);
    }
  else // if not compressed
    {
    if(!m_BinaryData)
      {
      double tf;
      for(int i=0; i<_dataQuantity*m_ElementNumberOfChannels; i++)
        {
        *_fstream >> tf;
        MET_DoubleToValue(tf, m_ElementType, _data, i);
        _fstream->get();
        }
      }
    else
      {
      _fstream->read((char *)_data, readSize);
      int gc = _fstream->gcount();
      if(gc != readSize)
        {
        METAIO_STREAM::cout
                  << "MetaArray: M_ReadElements: data not read completely" 
                  << METAIO_STREAM::endl;
        METAIO_STREAM::cout << "   ideal = " << readSize 
                            << " : actual = " << gc 
                            << METAIO_STREAM::endl;
        return false;
        }
      }
    }

  return true;
  }

//
//
//
bool MetaArray::
M_WriteElements(METAIO_STREAM::ofstream * _fstream, const void * _data,
               METAIO_STL::streamoff _dataQuantity)
  {
  bool localData = false;
  METAIO_STREAM::ofstream* tmpWriteStream;
  if(!strcmp(m_ElementDataFileName, "LOCAL"))
    {
    localData = true;
    tmpWriteStream = _fstream;
    }
  else
    {
    localData = false;
    tmpWriteStream = new METAIO_STREAM::ofstream;

    char dataFileName[255];
    char pathName[255];
    bool usePath = MET_GetFilePath(m_FileName, pathName);
    if(usePath)
      {
      sprintf(dataFileName, "%s%s", pathName, m_ElementDataFileName);
      }
    else
      {
      strcpy(dataFileName, m_ElementDataFileName);
      }

// Some older sgi compilers have a error in the ofstream constructor
// that requires a file to exist for output
#ifdef __sgi
    {
    METAIO_STREAM::ofstream tFile(dataFileName, METAIO_STREAM::ios::out);
    tFile.close();                    
    }
    tmpWriteStream->open(dataFileName, METAIO_STREAM::ios::out);
#else
    tmpWriteStream->open(dataFileName, METAIO_STREAM::ios::binary |
                                       METAIO_STREAM::ios::out);
#endif
    }

  if(!m_BinaryData)
    {
    double tf;
    for(int i=0; i<m_Length*m_ElementNumberOfChannels; i++)
      {
      MET_ValueToDouble(m_ElementType, _data, i, &tf);
      if((i+1)/10 == (double)(i+1.0)/10.0)
        {
        (*tmpWriteStream) << tf << METAIO_STREAM::endl;
        }
      else
        {
        (*tmpWriteStream) << tf << " ";
        }
      }
    }
   else
    {
    tmpWriteStream->write( (const char *)_data, (size_t)_dataQuantity );  
    } 

  if(!localData)
    {
    tmpWriteStream->close();
    delete tmpWriteStream; 
    }

  return true;
  }


#if (METAIO_USE_NAMESPACE)
};
#endif

