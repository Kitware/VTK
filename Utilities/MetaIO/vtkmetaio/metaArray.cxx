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

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring> // for memcpy
#include <string>

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
    std::cout << "MetaArray()" << std::endl;
    }

  m_ElementData = nullptr;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  m_ElementDataFileName = "";

  MetaArray::Clear();
}

//
MetaArray::
MetaArray(const char *_headerName)
:MetaForm()
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray()" << std::endl;
    }

  m_ElementData = nullptr;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  m_ElementDataFileName = "";

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
   std::cout << "MetaArray()" << std::endl;
   }

  m_ElementData = nullptr;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  m_ElementDataFileName = "";

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
   std::cout << "MetaArray()" << std::endl;
   }

  m_ElementData = nullptr;
  m_AutoFreeElementData = false;

  m_CompressedElementDataSize = 0;

  m_ElementDataFileName = "";

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

  std::cout << "Length = " << (int)m_Length << std::endl;

  std::cout << "BinaryData = "
                      << ((m_BinaryData)?"True":"False")
                      << std::endl;

  std::cout << "BinaryDataByteOrderMSB = "
                      << ((m_BinaryDataByteOrderMSB)?"True":"False")
                      << std::endl;

  char str[255];
  MET_TypeToString(m_ElementType, str);
  std::cout << "ElementType = " << str << std::endl;

  std::cout << "ElementNumberOfChannels = "
                      << m_ElementNumberOfChannels
                      << std::endl;

  std::cout << "AutoFreeElementData = "
                      << ((m_AutoFreeElementData)?"True":"False")
                      << std::endl;

  std::cout << "CompressedElementDataSize = "
                      << m_CompressedElementDataSize
                      << std::endl;

  std::cout << "ElementDataFileName = " << m_ElementDataFileName
                      << std::endl;

  std::cout << "ElementData = "
                      << ((m_ElementData==nullptr)?"NULL":"Valid")
                      << std::endl;
}

void MetaArray::
CopyInfo(const MetaForm * _form)
{
  MetaForm::CopyInfo(_form);
}

void MetaArray::
Clear()
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: Clear" << std::endl;
    }

  m_Length = 0;

  m_ElementType = MET_NONE;

  m_ElementNumberOfChannels = 1;

  m_CompressedElementDataSize = 0;

  m_ElementDataFileName = "";

  if(m_AutoFreeElementData)
    {
    if(m_ElementData != nullptr)
      {
      delete [] (char *)m_ElementData;
      }
    }
  m_ElementData = nullptr;
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
    std::cout << "MetaArray: Initialize" << std::endl;
    }

  MetaForm::InitializeEssential();

  bool result = true;

  if(m_Length != _length ||
     m_ElementType != _elementType ||
     m_ElementNumberOfChannels != _elementNumberOfChannels ||
     _elementData != nullptr ||
     _allocateElementData == true)
    {
    if(m_AutoFreeElementData)
      {
      if(m_ElementData != nullptr)
        {
        delete [] (char *)m_ElementData;
        }
      }
    m_ElementData = nullptr;

    m_Length = _length;
    m_ElementType = _elementType;
    m_ElementNumberOfChannels = _elementNumberOfChannels;

    if(_elementData != nullptr)
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
        m_ElementData = nullptr;
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
    if(m_ElementData != nullptr)
      {
      delete [] (char *)m_ElementData;
      }
    }
  m_ElementData = nullptr;

  m_AutoFreeElementData = _autoFreeElementData;

  int eSize;
  MET_SizeOfType(m_ElementType, &eSize);
  m_ElementData = new char[m_Length*m_ElementNumberOfChannels*eSize];

  if(m_ElementData != nullptr)
    {
    return true;
    }
  else
    {
    return false;
    }
}

int MetaArray::
Length() const
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
NDims() const
{
  return Length();
}

void MetaArray::
NDims(int _length)
{
  Length(_length);
}

MET_ValueEnumType MetaArray::
ElementType() const
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
ElementNumberOfChannels() const
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
ElementByteOrderSwap()
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: ElementByteOrderSwap"
                        << std::endl;
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
ElementByteOrderFix()
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
  if(m_ElementData == nullptr)
    {
    return false;
    }

  ElementByteOrderFix();

  void *            curBuffer              = m_ElementData;
  MET_ValueEnumType curElementType         = m_ElementType;
  bool              curAutoFreeElementData = m_AutoFreeElementData;

  if(m_ElementType != _toElementType)
    {
    m_ElementData = nullptr;
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
  if(m_ElementData == nullptr)
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


bool MetaArray::
AutoFreeElementData() const
{
  return m_AutoFreeElementData;
}

void MetaArray::
AutoFreeElementData(bool _autoFreeElementData)
{
  m_AutoFreeElementData = _autoFreeElementData;
}


const char * MetaArray::
ElementDataFileName() const
{
  return m_ElementDataFileName.c_str();
}

void MetaArray::
ElementDataFileName(const char * _elementDataFileName)
{
  m_ElementDataFileName = _elementDataFileName;
}


void * MetaArray::
ElementData()
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


bool MetaArray::
CanRead(const char *_headerName) const
{
  // First check the extension
  std::string fname = _headerName;
  if( fname == "" )
    {
    return false;
    }

  bool extensionFound = false;

  std::string::size_type stringPos = fname.rfind(".mva");
  if ((stringPos != std::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }

  stringPos = fname.rfind(".mvh");
  if ((stringPos != std::string::npos)
      && (stringPos == fname.length() - 4))
    {
    extensionFound = true;
    }

  if( !extensionFound )
    {
    return false;
    }

  // Now check the file content
  std::ifstream inputStream;

#ifdef __sgi
  inputStream.open( _headerName, std::ios::in );
#else
  inputStream.open( _headerName, std::ios::in |
                                 std::ios::binary );
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
  if(_headerName != nullptr)
    {
    m_FileName = _headerName;
    }

  std::ifstream * tmpStream = new std::ifstream;

#ifdef __sgi
  tmpStream->open(m_FileName, std::ios::in );
#else
  tmpStream->open(m_FileName, std::ios::in |
                              std::ios::binary);
#endif

  if(!tmpStream->rdbuf()->is_open())
    {
    std::cout << "MetaArray: Read: Cannot open file _"
                        << m_FileName << "_" << std::endl;
    delete tmpStream;
    return false;
    }

  bool result = ReadStream(tmpStream, _readElements,
                           _elementDataBuffer, _autoFreeElementData);

  if(_headerName != nullptr)
    {
    m_FileName = _headerName;
    }

  tmpStream->close();

  delete tmpStream;

  return result;
}


bool MetaArray::
CanReadStream(std::ifstream * _stream) const
{
  if(!strncmp(MET_ReadForm(*_stream).c_str(), "Array", 5))
    {
    return true;
    }
  return false;
}

bool MetaArray::
ReadStream(std::ifstream * _stream, bool _readElements,
           void * _elementDataBuffer, bool _autoFreeElementData)
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: ReadStream" << std::endl;
    }

  M_Destroy();

  Clear();

  M_SetupReadFields();

  if(m_ReadStream)
    {
    std::cout << "MetaArray: ReadStream: two files open?"
                        << std::endl;
    delete m_ReadStream;
    }

  m_ReadStream = _stream;

  if(!M_Read())
    {
    std::cout << "MetaArray: Read: Cannot parse file"
                        << std::endl;
    m_ReadStream = nullptr;
    return false;
    }

  InitializeEssential(m_Length,
                      m_ElementType,
                      m_ElementNumberOfChannels,
                      _elementDataBuffer,
                      true,
                      _autoFreeElementData);

  bool usePath;
  std::string pathName;
  std::string fName;
  usePath = MET_GetFilePath(m_FileName, pathName);

  if(_readElements)
    {
    if("Local" == m_ElementDataFileName ||
       "LOCAL" == m_ElementDataFileName ||
       "local" == m_ElementDataFileName)
      {
      M_ReadElements(m_ReadStream, m_ElementData, m_Length);
      }
    else
      {
      if(usePath)
        {
        fName = pathName + m_ElementDataFileName;
        }
      else
        {
        fName = m_ElementDataFileName;
        }
      std::ifstream* readStreamTemp = new std::ifstream;

#ifdef __sgi
      readStreamTemp->open(fName, std::ios::in);
#else
      readStreamTemp->open(fName, std::ios::binary |
                                  std::ios::in);
#endif
      if(!readStreamTemp->rdbuf()->is_open())
        {
        std::cout << "MetaArray: Read: Cannot open data file"
                            << std::endl;
        m_ReadStream = nullptr;
        return false;
        }
      M_ReadElements(readStreamTemp, m_ElementData, m_Length);
      readStreamTemp->close();
      delete readStreamTemp;
      }
    }

  m_ReadStream = nullptr;

  return true;
}

//

bool MetaArray::
Write(const char *_headName, const char *_dataName, bool _writeElements,
      const void *_constElementData)
{
  if(_headName != nullptr && strlen(_headName)>1)
    {
    FileName(_headName);
    }

  bool tmpDataFileName = false;
  if( _dataName != nullptr &&
      strlen(_dataName) > 1 )
    {
    tmpDataFileName = true;
    ElementDataFileName(_dataName);
    }
  else
    {
    if( m_ElementDataFileName.empty() )
      {
      tmpDataFileName = true;
      }
    }

  int sPtr = 0;
  MET_GetFileSuffixPtr(m_FileName, &sPtr);
  if( !strcmp(&(m_FileName[sPtr]), "mvh") )
    {
    MET_SetFileSuffix(m_FileName, "mvh");
    if( m_ElementDataFileName.empty() ||
        m_ElementDataFileName == "LOCAL" )
      {
      ElementDataFileName(m_FileName.c_str());
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

  std::string pathName;
  bool usePath = MET_GetFilePath(m_FileName, pathName);
  if(usePath)
    {
    std::string elementPathName;
    MET_GetFilePath(m_ElementDataFileName, elementPathName);
    if(pathName == elementPathName)
      {
      elementPathName = m_ElementDataFileName.substr(pathName.length());
      m_ElementDataFileName = elementPathName;
      }
    }

  std::ofstream * tmpWriteStream = new std::ofstream;

// Some older sgi compilers have a error in the ofstream constructor
// that requires a file to exist for output
#ifdef __sgi
{
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
WriteStream(std::ofstream * _stream, bool _writeElements,
            const void *_constElementData)
{
  if(m_WriteStream != nullptr)
    {
    std::cout << "MetaArray: WriteStream: two files open?"
                        << std::endl;
    delete m_WriteStream;
    }

  m_WriteStream = _stream;

  unsigned char * compressedElementData = nullptr;
  if(m_CompressedData)
    {
    int elementSize;
    MET_SizeOfType(m_ElementType, &elementSize);
    int elementNumberOfBytes = elementSize*m_ElementNumberOfChannels;

    if(_constElementData == nullptr)
      {
      compressedElementData = MET_PerformCompression(
                                    (const unsigned char *)m_ElementData,
                                    m_Length * elementNumberOfBytes,
                                    & m_CompressedElementDataSize,
                                    2 );
      }
    else
      {
      compressedElementData = MET_PerformCompression(
                                    (const unsigned char *)_constElementData,
                                    m_Length * elementNumberOfBytes,
                                    & m_CompressedElementDataSize,
                                    2 );
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

      if(_constElementData != nullptr)
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

  m_WriteStream = nullptr;

  return true;
}

void MetaArray::
M_Destroy()
{
  if(m_AutoFreeElementData && m_ElementData != nullptr)
    {
    delete [] (char *)m_ElementData;
    }
  m_ElementData = nullptr;

  MetaForm::M_Destroy();
}

void MetaArray::
M_SetupReadFields()
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: M_SetupReadFields"
                        << std::endl;
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
M_SetupWriteFields()
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
                     m_ElementDataFileName.length(),
                     m_ElementDataFileName.c_str());
  mF->terminateRead = true;
  m_Fields.push_back(mF);
}


bool MetaArray::
M_Read()
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: M_Read: Loading Header"
                        << std::endl;
    }
  if(!MetaForm::M_Read())
    {
    std::cout << "MetaArray: M_Read: Error parsing file"
                        << std::endl;
    return false;
    }

  if(META_DEBUG)
    {
    std::cout << "MetaArray: M_Read: Parsing Header"
                        << std::endl;
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
      std::cout << "MetaArray: M_Read: Error: Length required"
                          << std::endl;
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
    m_ElementDataFileName = (char *)(mF->value);
    }

  return true;
}

bool MetaArray::
M_ReadElements(std::ifstream * _fstream, void * _data,
               int _dataQuantity)
{
  if(META_DEBUG)
    {
    std::cout << "MetaArray: M_ReadElements" << std::endl;
    }

  int elementSize;
  MET_SizeOfType(m_ElementType, &elementSize);
  int readSize = _dataQuantity*m_ElementNumberOfChannels*elementSize;
  if(META_DEBUG)
    {
    std::cout << "MetaArray: M_ReadElements: ReadSize = "
                        << readSize << std::endl;
    }

  // If compressed we inflate
  if(m_CompressedData)
    {
    // if m_CompressedElementDataSize is not defined we assume the size of the
    // file is the size of the compressed data
    if(m_CompressedElementDataSize==0)
      {
      _fstream->seekg(0, std::ios::end);
      m_CompressedElementDataSize = _fstream->tellg();
      _fstream->seekg(0, std::ios::beg);
      }

    unsigned char* compr = new unsigned char[static_cast<size_t>(m_CompressedElementDataSize)];
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
      int gc = static_cast<int>(_fstream->gcount());
      if(gc != readSize)
        {
        std::cout
                  << "MetaArray: M_ReadElements: data not read completely"
                  << std::endl;
        std::cout << "   ideal = " << readSize
                            << " : actual = " << gc
                            << std::endl;
        return false;
        }
      }
    }

  return true;
}

bool MetaArray::
M_WriteElements(std::ofstream * _fstream, const void * _data,
               std::streamoff _dataQuantity)
{
  bool localData = false;
  std::ofstream* tmpWriteStream;
  if (m_ElementDataFileName == "LOCAL")
    {
    localData = true;
    tmpWriteStream = _fstream;
    }
  else
    {
    localData = false;
    tmpWriteStream = new std::ofstream;

    std::string dataFileName;
    std::string pathName;
    bool usePath = MET_GetFilePath(m_FileName, pathName);
    if(usePath)
      {
      dataFileName = pathName + m_ElementDataFileName;
      }
    else
      {
      dataFileName = m_ElementDataFileName;
      }

// Some older sgi compilers have a error in the ofstream constructor
// that requires a file to exist for output
#ifdef __sgi
    {
    std::ofstream tFile(dataFileName, std::ios::out);
    tFile.close();
    }
    tmpWriteStream->open(dataFileName, std::ios::out);
#else
    tmpWriteStream->open(dataFileName, std::ios::binary |
                                       std::ios::out);
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
        (*tmpWriteStream) << tf << std::endl;
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
