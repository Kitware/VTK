/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLWriter.h"
#include "vtkBase64OutputStream.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkZLibDataCompressor.h"
#include "vtkDataSet.h"
#include "vtkOutputStream.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkXMLWriter, "1.16");
vtkCxxSetObjectMacro(vtkXMLWriter, Compressor, vtkDataCompressor);

//----------------------------------------------------------------------------
vtkXMLWriter::vtkXMLWriter()
{
  this->FileName = 0;
  this->Stream = 0;
  
  // Default binary data mode is base-64 encoding.
  this->DataStream = vtkBase64OutputStream::New();
  
  // Byte order defaults to that of machine.
#ifdef VTK_WORDS_BIGENDIAN
  this->ByteOrder = vtkXMLWriter::BigEndian;
#else
  this->ByteOrder = vtkXMLWriter::LittleEndian;
#endif  

  // Output vtkIdType size defaults to real size.
#ifdef VTK_USE_64BIT_IDS
  this->IdType = vtkXMLWriter::Int64;
#else
  this->IdType = vtkXMLWriter::Int32;
#endif

  // Initialize compression data.
  this->BlockSize = 32768;
  this->Compressor = vtkZLibDataCompressor::New();
  this->CompressionHeader = 0;
  
  this->EncodeAppendedData = 1;
  this->AppendedDataPosition = 0;
  this->DataMode = vtkXMLWriter::Appended;
}

//----------------------------------------------------------------------------
vtkXMLWriter::~vtkXMLWriter()
{
  this->SetFileName(0);
  this->DataStream->Delete();
  this->SetCompressor(0);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    os << indent << "ByteOrder: BigEndian\n";
    }
  else
    {
    os << indent << "ByteOrder: LittleEndian\n";
    }
  if(this->IdType == vtkXMLWriter::Int32)
    {
    os << indent << "IdType: Int32\n";
    }
  else
    {
    os << indent << "IdType: Int64\n";
    }
  if(this->DataMode == vtkXMLWriter::Ascii)
    {
    os << indent << "DataMode: Ascii\n";
    }
  else if(this->DataMode == vtkXMLWriter::Binary)
    {
    os << indent << "DataMode: Binary\n";
    }
  else
    {
    os << indent << "DataMode: Appended\n";
    }
  if(this->Compressor)
    {
    os << indent << "Compressor: " << this->Compressor << "\n";
    }
  else
    {
    os << indent << "Compressor: (none)\n";
    }
  os << indent << "EncodeAppendedData: " << this->EncodeAppendedData << "\n";
  os << indent << "BlockSize: " << this->BlockSize << "\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetByteOrderToBigEndian()
{
  this->SetByteOrder(vtkXMLWriter::BigEndian);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetByteOrderToLittleEndian()
{
  this->SetByteOrder(vtkXMLWriter::LittleEndian);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdType(int t)
{
#if !defined(VTK_USE_64BIT_IDS)
  if(t == vtkXMLWriter::Int64)
    {
    vtkErrorMacro("Support for Int64 vtkIdType not compiled in VTK.");
    return;
    }
#endif
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting IdType to " << t);
  if(this->IdType != t)
    {
    this->IdType = t;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdTypeToInt32()
{
  this->SetIdType(vtkXMLWriter::Int32);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetIdTypeToInt64()
{
  this->SetIdType(vtkXMLWriter::Int64);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToAscii()
{
  this->SetDataMode(vtkXMLWriter::Ascii);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToBinary()
{
  this->SetDataMode(vtkXMLWriter::Binary);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataModeToAppended()
{
  this->SetDataMode(vtkXMLWriter::Appended);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::Write()
{
  // Make sure there are enough settings to write (Input, FileName, etc).
  if(!this->IsSafeToWrite())
    {
    return 0;
    }
  
  // Try to open the output file for writing.
#ifdef _WIN32
  ofstream outFile(this->FileName, ios::out | ios::binary);
#else
  ofstream outFile(this->FileName, ios::out);
#endif
  if(!outFile)
    {
    vtkErrorMacro("Error opening output file \"" << this->FileName << "\"");
    return 0;
    }
  
  // Setup the output streams.
  this->Stream = &outFile;
  this->DataStream->SetStream(this->Stream);
  
  // Tell the subclass to write the data.
  int result = this->WriteData();
  
  // Cleanup the output streams.
  this->DataStream->SetStream(0);
  this->Stream = 0;
  
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::GetDataSetMajorVersion()
{
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::GetDataSetMinorVersion()
{
  return 1;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLWriter::GetInputAsDataSet()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkDataSet*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::StartFile()
{
  ostream& os = *(this->Stream);
  
  // If this will really be a valid XML file, put the XML header at
  // the top.
  if(this->EncodeAppendedData)
    {
    os << "<?xml version=\"1.0\"?>\n";
    }
  
  // Open the document-level element.  This will contain the rest of
  // the elements.
  os << "<VTKFile";
  this->WriteFileAttributes();
  os << ">\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteFileAttributes()
{
  ostream& os = *(this->Stream);
  
  // Write the file's type.
  this->WriteStringAttribute("type", this->GetDataSetName());
  
  // Write the version number of the file.
  os << " version=\""
     << this->GetDataSetMajorVersion()
     << "."
     << this->GetDataSetMinorVersion()
     << "\"";
  
  // Write the byte order for the file.
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    os << " byte_order=\"BigEndian\"";
    }
  else
    {
    os << " byte_order=\"LittleEndian\"";
    }
  
  // Write the compressor that will be used for the file.
  if(this->Compressor)
    {
    os << " compressor=\"" << this->Compressor->GetClassName() << "\"";
    }  
}

//----------------------------------------------------------------------------
void vtkXMLWriter::EndFile()
{
  ostream& os = *(this->Stream);
  
  // Close the document-level element.
  os << "</VTKFile>\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::StartAppendedData()
{
  ostream& os = *(this->Stream);
  os << "  <AppendedData encoding=\""
     << (this->EncodeAppendedData? "base64" : "raw")
     << "\">\n";
  os << "   _";
  this->AppendedDataPosition = os.tellp();
  
  // Setup proper output encoding.
  if(this->EncodeAppendedData)
    {
    vtkBase64OutputStream* base64 = vtkBase64OutputStream::New();
    this->SetDataStream(base64);
    base64->Delete();
    }
  else
    {
    vtkOutputStream* raw = vtkOutputStream::New();
    this->SetDataStream(raw);
    raw->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::EndAppendedData()
{
  ostream& os = *(this->Stream);
  os << "\n";
  os << "  </AppendedData>\n";
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::ReserveAttributeSpace(const char* attr)
{
  // Write enough space to go back and write the given attribute with
  // an appended data offset value.  Returns the stream position at
  // which attribute should be later written with
  // WriteAppendedDataOffset().  If attr is 0, writes space only for
  // the double quotes and value.
  ostream& os = *(this->Stream);
  unsigned long startPosition = os.tellp();
  if(attr) { os << " " << attr; }
  os << "               ";
  return startPosition;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::GetAppendedDataOffset()
{
  unsigned long pos = this->Stream->tellp();
  return (pos - this->AppendedDataPosition);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::WriteAppendedDataOffset(unsigned long streamPos,
                                                    const char* attr)
{
  // Write an XML attribute with the given name.  The value is the
  // current appended data offset.  Starts writing at the given stream
  // position, and returns the ending position.  If attr is 0, writes
  // only the double quotes.  In all cases, the final stream position
  // is left the same as before the call.
  ostream& os = *(this->Stream);
  unsigned long returnPos = os.tellp();
  unsigned long offset = returnPos - this->AppendedDataPosition;
  os.seekp(streamPos);
  if(attr) { os << " " << attr << "="; }
  os << "\"" << offset << "\"";
  unsigned long endPos = os.tellp();
  os.seekp(returnPos);
  return endPos;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryData(void* in_data, int numWords, int wordType)
{
  // Find the total size of the data.
  unsigned long wordSize = this->GetWordTypeSize(wordType);
  int result;
  void* data = in_data;
  
#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted to the type
  // requested for output.
# if VTK_SIZEOF_SHORT == 4
  typedef short vtkXMLWriterInt32IdType;
# elif VTK_SIZEOF_INT == 4
  typedef int vtkXMLWriterInt32IdType;
# elif VTK_SIZEOF_LONG == 4
  typedef long vtkXMLWriterInt32IdType;
# else
#  error "No native data type can represent a signed 32-bit integer."
# endif
  vtkXMLWriterInt32IdType* tmpBuffer = 0;
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLWriter::Int32))
    {
    vtkIdType* idBuffer = static_cast<vtkIdType*>(in_data);
    tmpBuffer = new vtkXMLWriterInt32IdType[numWords];
    
    int i;
    for(i=0;i < numWords; ++i)
      {
      tmpBuffer[i] = static_cast<vtkXMLWriterInt32IdType>(idBuffer[i]);
      }
    
    wordSize = 4;
    data = tmpBuffer;
    }
#endif
  
  if(wordSize > 1)
    {
    // Byte swap first.
    unsigned long binaryDataSize = wordSize*numWords;
    char* byteSwapBuffer = new char[binaryDataSize];
    memcpy(byteSwapBuffer, static_cast<char*>(data), binaryDataSize);
    this->PerformByteSwap(byteSwapBuffer, numWords, wordSize);
    
    // Now write the data.
    result = this->WriteBinaryDataInternal(byteSwapBuffer, binaryDataSize);
    delete [] byteSwapBuffer;
    }
  else
    {
    // Don't need to byte swap.  Just write the data.
    result = this->WriteBinaryDataInternal(data, numWords);
    }
  
  // Destroy the compression header if it was used.
  if(this->CompressionHeader)
    {
    delete [] this->CompressionHeader;
    this->CompressionHeader = 0;
    }
  
#ifdef VTK_USE_64BIT_IDS
  if(tmpBuffer)
    {
    delete [] tmpBuffer;
    }
#endif
  
  return result;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::PerformByteSwap(void* data, int numWords, int wordSize)
{
  char* ptr = static_cast<char*>(data);
  if(this->ByteOrder == vtkXMLWriter::BigEndian)
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2BERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4BERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8BERange(ptr, numWords); break;
      default:
        vtkErrorMacro("Unsupported data type size " << wordSize);
      }
    }
  else
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2LERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4LERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8LERange(ptr, numWords); break;
      default:
        vtkErrorMacro("Unsupported data type size " << wordSize);
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::SetDataStream(vtkOutputStream* arg)
{
  if(this->DataStream != arg)
    {
    if(this->DataStream != NULL)
      {
      this->DataStream->UnRegister(this);
      }
    this->DataStream = arg;
    if(this->DataStream != NULL)
      {
      this->DataStream->Register(this);
      this->DataStream->SetStream(this->Stream);
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteBinaryDataInternal(void* data, unsigned long size)
{
  unsigned char* dataBuffer = static_cast<unsigned char*>(data);
  if(this->Compressor)
    {
    // Need to compress the data.
    // Create compression header.  This saves enough space in the output.
    if(!this->CreateCompressionHeader(size)) { return 0; }
    
    // Start writing the data.
    if(!this->DataStream->StartWriting()) { return 0; }
    
    // Break into blocks and compress each one separately.
    // This allows for better random access when reading.
    unsigned char* ptr = dataBuffer;
    unsigned long dataLeft = size;
    
    // Do the complete blocks.
    while(dataLeft >= this->BlockSize)
      {
      if(!this->WriteCompressionBlock(ptr, this->BlockSize)) { return 0; }
      ptr += this->BlockSize;
      dataLeft -= this->BlockSize;
      }
    
    // Do the last partial block if any.
    if(dataLeft > 0)
      {
      if(!this->WriteCompressionBlock(ptr, dataLeft)) { return 0; }      
      }
    
    // Finish writing the data.
    if(!this->DataStream->EndWriting()) { return 0; }
    
    // Go back and write the real compression header in its proper place.
    if(!this->WriteCompressionHeader()) { return 0; }
    return 1;
    }
  else
    {
    // No data compression.  Just write it.  Store the length first.
    HeaderType length = size;
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    this->PerformByteSwap(p, 1, sizeof(HeaderType));
    if(!this->DataStream->StartWriting()) { return 0; }
    if(!this->DataStream->Write(p, sizeof(HeaderType))) { return 0; }
    return (this->DataStream->Write(dataBuffer, size) &&
            this->DataStream->EndWriting());
    }
}

//----------------------------------------------------------------------------
int vtkXMLWriter::CreateCompressionHeader(unsigned long size)
{
  // Allocate and initialize the compression header.
  // The format is this:
  //  struct header {
  //    HeaderType number_of_blocks;
  //    HeaderType uncompressed_block_size;
  //    HeaderType uncompressed_last_block_size;
  //    HeaderType compressed_block_sizes[number_of_blocks]; 
  //  }
 
  // Find the size and number of blocks.
  unsigned long numFullBlocks = size / this->BlockSize;
  unsigned long lastBlockSize = size % this->BlockSize;
  unsigned int numBlocks = numFullBlocks + (lastBlockSize?1:0);
  
  unsigned int headerLength = numBlocks+3;
  this->CompressionHeaderLength = headerLength;
  
  this->CompressionHeader = new HeaderType[headerLength];
  
  // Write out dummy header data.
  unsigned int i;
  for(i=0; i < headerLength; ++i) { this->CompressionHeader[i] = 0; }  
  
  this->CompressionHeaderPosition = this->Stream->tellp();
  unsigned char* ch =
    reinterpret_cast<unsigned char*>(this->CompressionHeader);
  unsigned int chSize = (this->CompressionHeaderLength*sizeof(HeaderType));
  
  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(ch, chSize) &&
                this->DataStream->EndWriting());
  
  // Fill in known header data now.
  this->CompressionHeader[0] = numBlocks;
  this->CompressionHeader[1] = this->BlockSize;
  this->CompressionHeader[2] = lastBlockSize;
  
  // Initialize counter for block writing.
  this->CompressionBlockNumber = 0;
  
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionBlock(unsigned char* data,
                                        unsigned long size)
{
  // Compress the data.
  vtkUnsignedCharArray* outputArray = this->Compressor->Compress(data, size);
  
  HeaderType outputSize = outputArray->GetNumberOfTuples();
  unsigned char* outputPointer = outputArray->GetPointer(0);
  
  // Write the compressed data.
  int result = this->DataStream->Write(outputPointer, outputSize);
  
  // Store the resulting compressed size in the compression header.
  this->CompressionHeader[3+this->CompressionBlockNumber++] = outputSize;
  
  outputArray->Delete();
  
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteCompressionHeader()
{
  // Write real compression header back into stream.
  unsigned long returnPosition = this->Stream->tellp();
  
  // Need to byte-swap header.
  this->PerformByteSwap(this->CompressionHeader, this->CompressionHeaderLength,
                        sizeof(HeaderType));
  
  if(!this->Stream->seekp(this->CompressionHeaderPosition)) { return 0; }
  unsigned char* ch =
    reinterpret_cast<unsigned char*>(this->CompressionHeader);
  unsigned int chSize = (this->CompressionHeaderLength*sizeof(HeaderType));
  int result = (this->DataStream->StartWriting() &&
                this->DataStream->Write(ch, chSize) &&
                this->DataStream->EndWriting());
  if(!this->Stream->seekp(returnPosition)) { return 0; }
  return result;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::GetWordTypeSize(int dataType)
{
  unsigned long size = 1;
  switch (dataType)
    {
    case VTK_ID_TYPE:        size = sizeof(vtkIdType); break;
    case VTK_FLOAT:          size = sizeof(float); break;
    case VTK_DOUBLE:         size = sizeof(double); break;
    case VTK_INT:            size = sizeof(int); break;
    case VTK_UNSIGNED_INT:   size = sizeof(unsigned int); break;
    case VTK_LONG:           size = sizeof(long); break;
    case VTK_UNSIGNED_LONG:  size = sizeof(unsigned long); break;
    case VTK_SHORT:          size = sizeof(short); break;
    case VTK_UNSIGNED_SHORT: size = sizeof(unsigned short); break;
    case VTK_UNSIGNED_CHAR:  size = sizeof(unsigned char); break;
    case VTK_CHAR:           size = sizeof(char); break;
    default:
      { vtkWarningMacro("Unsupported data type: " << dataType); } break;
    }
  return size;
}

//----------------------------------------------------------------------------
const char* vtkXMLWriter::GetWordTypeName(int dataType)
{
  char isSigned = 0;
  int size = 0;
  
  // These string values must match vtkXMLDataElement::GetWordTypeAttribute().
  switch (dataType)
    {
    case VTK_FLOAT:          return "Float32";
    case VTK_DOUBLE:         return "Float64";
    case VTK_ID_TYPE:
      {
      switch (this->IdType)
        {
        case vtkXMLWriter::Int32: return "Int32";
        case vtkXMLWriter::Int64: return "Int64";
        default: return 0;
        }
      } 
    case VTK_CHAR:           isSigned = 1; size = sizeof(char); break;
    case VTK_INT:            isSigned = 1; size = sizeof(int); break;
    case VTK_LONG:           isSigned = 1; size = sizeof(long); break;
    case VTK_SHORT:          isSigned = 1; size = sizeof(short); break;
    case VTK_UNSIGNED_CHAR:  isSigned = 0; size = sizeof(unsigned char); break;
    case VTK_UNSIGNED_INT:   isSigned = 0; size = sizeof(unsigned int); break;
    case VTK_UNSIGNED_LONG:  isSigned = 0; size = sizeof(unsigned long); break;
    case VTK_UNSIGNED_SHORT: isSigned = 0; size = sizeof(unsigned short); break;
    default:
    {
    vtkWarningMacro("Unsupported data type: " << dataType); } break;
    }
  const char* type = 0;
  switch (size)
    {
    case 1: type = isSigned? "Int8"  : "UInt8";  break;
    case 2: type = isSigned? "Int16" : "UInt16"; break;
    case 4: type = isSigned? "Int32" : "UInt32"; break;
    case 8: type = isSigned? "Int64" : "UInt64"; break;
    default:
      {
      vtkErrorMacro("Data type size " << size
                    << " not supported by VTK XML format.");
      }
    }
  return type;
}

//----------------------------------------------------------------------------
template <class T>
int vtkXMLWriterWriteVectorAttribute(ostream& os, const char* name,
                                     int length, T* data)
{
  os << " " << name << "=\"";
  if(length)
    {
    int i;
    os << data[0];
    for(i=1;i < length; ++i)
      {
      os << " " << data[i];
      }
    }
  os << "\"";
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteScalarAttribute(const char* name, int data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteScalarAttribute(const char* name, float data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}

//----------------------------------------------------------------------------
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
int vtkXMLWriter::WriteScalarAttribute(const char* name, vtkIdType data)
{
  return this->WriteVectorAttribute(name, 1, &data);
}
#endif

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       int* data)
{
  return vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       float* data)
{
  return vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);
}

//----------------------------------------------------------------------------
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
int vtkXMLWriter::WriteVectorAttribute(const char* name, int length,
                                       vtkIdType* data)
{
  return vtkXMLWriterWriteVectorAttribute(*(this->Stream), name, length, data);
}
#endif

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteDataModeAttribute(const char* name)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"";
  if(this->DataMode == vtkXMLWriter::Appended)
    {
    os << "appended";
    }
  else if(this->DataMode == vtkXMLWriter::Binary)
    {
    os << "binary";
    }
  else
    {
    os << "ascii";
    }
  os << "\"";
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteWordTypeAttribute(const char* name, int dataType)
{
  ostream& os = *(this->Stream);
  const char* value = this->GetWordTypeName(dataType);
  if(!value)
    {
    return 0;
    }
  os << " " << name << "=\"" << value << "\"";
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteStringAttribute(const char* name, const char* value)
{
  ostream& os = *(this->Stream);
  os << " " << name << "=\"" << value << "\"";
  return (os? 1:0);
}

//----------------------------------------------------------------------------
template <class T>
int vtkXMLWriteAsciiData(ostream& os, T* data, int length, vtkIndent indent)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  for(r=0;r < rows;++r)
    {
    os << indent << data[pos++];
    for(c=1;c < columns;++c)
      {
      os << " " << data[pos++];
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << data[pos++];
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << data[pos++];
      }
    os << "\n";
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriteAsciiDataChar(ostream& os, char* data, int length,
                             vtkIndent indent)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  for(r=0;r < rows;++r)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < columns;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << short(data[pos++]);
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << short(data[pos++]);
      }
    os << "\n";  
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriteAsciiDataUnsignedChar(ostream& os, unsigned char* data,
                                     unsigned long length, vtkIndent indent)
{
  int columns = 6;
  int rows = length/columns;
  int lastRowLength = length%columns;
  int r,c;
  int pos=0;
  typedef unsigned short ushort;
  for(r=0;r < rows;++r)
    {
    os << indent << ushort(data[pos++]);
    for(c=1;c < columns;++c)
      {
      os << " " << ushort(data[pos++]);
      }
    os << "\n";
    }
  if(lastRowLength > 0)
    {
    os << indent << ushort(data[pos++]);
    for(c=1;c < lastRowLength;++c)
      {
      os << " " << ushort(data[pos++]);
      }
    os << "\n";  
    }
  return (os? 1:0);
}

//----------------------------------------------------------------------------
int vtkXMLWriter::WriteAsciiData(void* data, int numWords, int wordType,
                                 vtkIndent indent)
{
  void* b = data;
  int nw = numWords;
  vtkIndent i = indent;
  ostream& os = *(this->Stream);
  switch(wordType)
    {
    case VTK_ID_TYPE:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<vtkIdType*>(b), nw, i);
    case VTK_DOUBLE:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<double*>(b), nw, i);
    case VTK_FLOAT:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<float*>(b), nw, i);
    case VTK_LONG:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<long*>(b), nw, i);
    case VTK_UNSIGNED_LONG:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<unsigned long*>(b), nw, i);
    case VTK_INT:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<int*>(b), nw, i);
    case VTK_UNSIGNED_INT:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<unsigned int*>(b), nw, i);
    case VTK_SHORT:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<short*>(b), nw, i);
    case VTK_UNSIGNED_SHORT:
      return vtkXMLWriteAsciiData(os, reinterpret_cast<unsigned short*>(b), nw, i);
    case VTK_CHAR:
      return vtkXMLWriteAsciiDataChar(os, reinterpret_cast<char*>(b), nw, i);
    case VTK_UNSIGNED_CHAR:
      return vtkXMLWriteAsciiDataUnsignedChar(
        os, reinterpret_cast<unsigned char*>(b), nw, i);
    default:
      return 0;
    }
}

//----------------------------------------------------------------------------
unsigned long vtkXMLWriter::WriteDataArrayAppended(vtkDataArray* a,
                                                   vtkIndent indent,
                                                   const char* alternateName)
{
  ostream& os = *(this->Stream);
  os << indent << "<DataArray";
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if(alternateName)
    {
    this->WriteStringAttribute("Name", alternateName);
    }
  else
    {
    const char* arrayName = a->GetName();
    if(arrayName) { this->WriteStringAttribute("Name", arrayName); }
    }
  if(a->GetNumberOfComponents() > 1)
    {
    this->WriteScalarAttribute("NumberOfComponents",
                               a->GetNumberOfComponents());
    }
  this->WriteDataModeAttribute("format");
  unsigned long pos = this->ReserveAttributeSpace("offset");
  os << "/>\n";
  return pos;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteDataArrayAppendedData(vtkDataArray* a,
                                              unsigned long pos)
{
  this->WriteAppendedDataOffset(pos, "offset");
  this->WriteBinaryData(a->GetVoidPointer(0),
                        a->GetNumberOfTuples()*a->GetNumberOfComponents(),
                        a->GetDataType());
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteDataArrayInline(vtkDataArray* a, vtkIndent indent,
                                        const char* alternateName)
{
  ostream& os = *(this->Stream);
  os << indent << "<DataArray";
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if(alternateName)
    {
    this->WriteStringAttribute("Name", alternateName);
    }
  else
    {
    const char* arrayName = a->GetName();
    if(arrayName) { this->WriteStringAttribute("Name", arrayName); }
    }
  if(a->GetNumberOfComponents() > 1)
    {
    this->WriteScalarAttribute("NumberOfComponents",
                               a->GetNumberOfComponents());
    }
  this->WriteDataModeAttribute("format");
  os << ">\n";
  this->WriteInlineData(a->GetVoidPointer(0),
                        a->GetNumberOfTuples()*a->GetNumberOfComponents(),
                        a->GetDataType(), indent.GetNextIndent());
  os << indent << "</DataArray>\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteInlineData(void* data, int numWords, int wordType,
                                   vtkIndent indent)
{
  if(this->DataMode == vtkXMLWriter::Binary)
    {
    ostream& os = *(this->Stream);
    os << indent;
    this->WriteBinaryData(data, numWords, wordType);
    os << "\n";
    }
  else
    {
    this->WriteAsciiData(data, numWords, wordType, indent);
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointDataInline(vtkPointData* pd, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());
  
  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);
  os << ">\n";
  
  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    vtkDataArray* a = this->CreateArrayForPoints(pd->GetArray(i));
    this->WriteDataArrayInline(a, indent.GetNextIndent(), names[i]);
    a->Delete();
    }
  
  os << indent << "</PointData>\n";
  
  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCellDataInline(vtkCellData* cd, vtkIndent indent)
{  
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());
  
  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);
  os << ">\n";

  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    vtkDataArray* a = this->CreateArrayForCells(cd->GetArray(i));
    this->WriteDataArrayInline(a, indent.GetNextIndent(), names[i]);
    a->Delete();
    }
  
  os << indent << "</CellData>\n";
  
  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
unsigned long* vtkXMLWriter::WritePointDataAppended(vtkPointData* pd,
                                                    vtkIndent indent)
{
  ostream& os = *(this->Stream);
  unsigned long* pdPositions = new unsigned long[pd->GetNumberOfArrays()];
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());
  
  os << indent << "<PointData";
  this->WriteAttributeIndices(pd, names);
  os << ">\n";
  
  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    pdPositions[i] = this->WriteDataArrayAppended(pd->GetArray(i),
                                                  indent.GetNextIndent(),
                                                  names[i]);
    }
  
  os << indent << "</PointData>\n";
  
  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
  
  return pdPositions;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointDataAppendedData(vtkPointData* pd,
                                              unsigned long* pdPositions)
{
  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    vtkDataArray* a = this->CreateArrayForPoints(pd->GetArray(i));
    this->WriteDataArrayAppendedData(a, pdPositions[i]);
    a->Delete();
    }
  delete [] pdPositions;
}

//----------------------------------------------------------------------------
unsigned long* vtkXMLWriter::WriteCellDataAppended(vtkCellData* cd,
                                                   vtkIndent indent)
{
  ostream& os = *(this->Stream);
  unsigned long* cdPositions = new unsigned long[cd->GetNumberOfArrays()];
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());
  
  os << indent << "<CellData";
  this->WriteAttributeIndices(cd, names);
  os << ">\n";
  
  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    cdPositions[i] = this->WriteDataArrayAppended(cd->GetArray(i),
                                                  indent.GetNextIndent(),
                                                  names[i]);
    }
  
  os << indent << "</CellData>\n";
  
  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
  
  return cdPositions;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCellDataAppendedData(vtkCellData* cd,
                                             unsigned long* cdPositions)
{
  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    vtkDataArray* a = this->CreateArrayForCells(cd->GetArray(i));
    this->WriteDataArrayAppendedData(a, cdPositions[i]);
    a->Delete();
    }
  delete [] cdPositions;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteAttributeIndices(vtkDataSetAttributes* dsa,
                                         char** names)
{
  int i;
  int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
  dsa->GetAttributeIndices(attributeIndices);
  for(i=0;i < vtkDataSetAttributes::NUM_ATTRIBUTES;++i)
    {
    if(attributeIndices[i] >= 0)
      {
      const char* attrName = dsa->GetAttributeTypeAsString(i);
      vtkDataArray* a = dsa->GetArray(attributeIndices[i]);
      const char* arrayName = a->GetName();
      if(!arrayName)
        {
        // Assign a name to the array.
        names[attributeIndices[i]] = new char[strlen(attrName)+2];
        strcpy(names[attributeIndices[i]], attrName);
        strcat(names[attributeIndices[i]], "_");
        arrayName = names[attributeIndices[i]];
        }
      this->WriteStringAttribute(attrName, arrayName);
      }
    }  
}

//----------------------------------------------------------------------------
unsigned long
vtkXMLWriter::WritePointsAppended(vtkPoints* points, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  unsigned long pointsPosition = 0;
  
  // Only write points if they exist.
  os << indent << "<Points>\n";
  if(points)
    {
    pointsPosition =
      this->WriteDataArrayAppended(points->GetData(), indent.GetNextIndent());
    }
  os << indent << "</Points>\n";
  
  return pointsPosition;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointsAppendedData(vtkPoints* points,
                                           unsigned long pointsPosition)
{
  // Only write points if they exist.
  if(points)
    {
    vtkDataArray* outPoints = this->CreateArrayForPoints(points->GetData());
    this->WriteDataArrayAppendedData(outPoints, pointsPosition);
    outPoints->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePointsInline(vtkPoints* points, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<Points>\n";
  if(points)
    {
    vtkDataArray* outPoints = this->CreateArrayForPoints(points->GetData());
    this->WriteDataArrayInline(outPoints, indent.GetNextIndent());
    outPoints->Delete();
    }
  os << indent << "</Points>\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCoordinatesInline(vtkDataArray* xc, vtkDataArray* yc,
                                          vtkDataArray* zc, vtkIndent indent)
{  
  ostream& os = *(this->Stream);
  
  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  if(xc && yc && zc)
    {
    vtkDataArray* oxc = this->CreateExactCoordinates(xc, 0);
    vtkDataArray* oyc = this->CreateExactCoordinates(yc, 1);
    vtkDataArray* ozc = this->CreateExactCoordinates(zc, 2);
    
    this->WriteDataArrayInline(oxc, indent.GetNextIndent());
    this->WriteDataArrayInline(oyc, indent.GetNextIndent());
    this->WriteDataArrayInline(ozc, indent.GetNextIndent());
    
    oxc->Delete();
    oyc->Delete();
    ozc->Delete();
    }
  os << indent << "</Coordinates>\n";
}

//----------------------------------------------------------------------------
unsigned long*
vtkXMLWriter::WriteCoordinatesAppended(vtkDataArray* xc, vtkDataArray* yc,
                                       vtkDataArray* zc, vtkIndent indent)
{
  unsigned long* cPositions = new unsigned long[3];
  ostream& os = *(this->Stream);
  
  // Only write coordinates if they exist.
  os << indent << "<Coordinates>\n";
  if(xc && yc && zc)
    {
    cPositions[0] = this->WriteDataArrayAppended(xc, indent.GetNextIndent());
    cPositions[1] = this->WriteDataArrayAppended(yc, indent.GetNextIndent());
    cPositions[2] = this->WriteDataArrayAppended(zc, indent.GetNextIndent());
    }
  os << indent << "</Coordinates>\n";
  
  return cPositions;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WriteCoordinatesAppendedData(vtkDataArray* xc,
                                                vtkDataArray* yc,
                                                vtkDataArray* zc,
                                                unsigned long* cPositions)
{
  // Only write coordinates if they exist.
  if(xc && yc && zc)
    {
    vtkDataArray* oxc = this->CreateExactCoordinates(xc, 0);
    vtkDataArray* oyc = this->CreateExactCoordinates(yc, 1);
    vtkDataArray* ozc = this->CreateExactCoordinates(zc, 2);
    
    this->WriteDataArrayAppendedData(oxc, cPositions[0]);
    this->WriteDataArrayAppendedData(oyc, cPositions[1]);
    this->WriteDataArrayAppendedData(ozc, cPositions[2]);
    
    oxc->Delete();
    oyc->Delete();
    ozc->Delete();
    }
  delete [] cPositions;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkXMLWriter::CreateArrayForPoints(vtkDataArray* inArray)
{
  // Only some subclasses need to do anything.  By default, just
  // return the array as given.
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkXMLWriter::CreateArrayForCells(vtkDataArray* inArray)
{
  // Only some subclasses need to do anything.  By default, just
  // return the array as given.
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
vtkDataArray* vtkXMLWriter::CreateExactCoordinates(vtkDataArray* inArray, int)
{
  // This method is just a dummy because we don't want a pure virtual.
  // Subclasses that need it should define the real version.
  vtkErrorMacro("vtkXMLWriter::CreateExactCoordinates should never be called.");
  inArray->Register(0);
  return inArray;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePPointData(vtkPointData* pd, vtkIndent indent)
{
  if(pd->GetNumberOfArrays() == 0)
    {
    return;
    }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(pd->GetNumberOfArrays());
  
  os << indent << "<PPointData";
  this->WriteAttributeIndices(pd, names);
  os << ">\n";
  
  int i;
  for(i=0; i < pd->GetNumberOfArrays(); ++i)
    {
    this->WritePDataArray(pd->GetArray(i), indent.GetNextIndent(), names[i]);
    }  
  
  os << indent << "</PPointData>\n";
  
  this->DestroyStringArray(pd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePCellData(vtkCellData* cd, vtkIndent indent)
{
  if(cd->GetNumberOfArrays() == 0)
    {
    return;
    }
  ostream& os = *(this->Stream);
  char** names = this->CreateStringArray(cd->GetNumberOfArrays());
  
  os << indent << "<PCellData";
  this->WriteAttributeIndices(cd, names);
  os << ">\n";
  
  int i;
  for(i=0; i < cd->GetNumberOfArrays(); ++i)
    {
    this->WritePDataArray(cd->GetArray(i), indent.GetNextIndent(), names[i]);
    }
  
  os << indent << "</PCellData>\n";
  
  this->DestroyStringArray(cd->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePPoints(vtkPoints* points, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  // Only write points if they exist.
  os << indent << "<PPoints>\n";
  if(points)
    {
    this->WritePDataArray(points->GetData(), indent.GetNextIndent());
    }
  os << indent << "</PPoints>\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePDataArray(vtkDataArray* a, vtkIndent indent,
                                   const char* alternateName)
{
  ostream& os = *(this->Stream);
  os << indent << "<PDataArray";
  this->WriteWordTypeAttribute("type", a->GetDataType());
  if(alternateName)
    {
    this->WriteStringAttribute("Name", alternateName);
    }
  else
    {
    const char* arrayName = a->GetName();
    if(arrayName) { this->WriteStringAttribute("Name", arrayName); }
    }
  if(a->GetNumberOfComponents() > 1)
    {
    this->WriteScalarAttribute("NumberOfComponents",
                               a->GetNumberOfComponents());
    }
  os << "/>\n";
}

//----------------------------------------------------------------------------
void vtkXMLWriter::WritePCoordinates(vtkDataArray* xc, vtkDataArray* yc,
                                     vtkDataArray* zc, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  
  // Only write coordinates if they exist.
  os << indent << "<PCoordinates>\n";
  if(xc && yc && zc)
    {
    this->WritePDataArray(xc, indent.GetNextIndent());
    this->WritePDataArray(yc, indent.GetNextIndent());
    this->WritePDataArray(zc, indent.GetNextIndent());
    }
  os << indent << "</PCoordinates>\n";  
}

//----------------------------------------------------------------------------
char** vtkXMLWriter::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  int i;
  for(i=0; i < numStrings; ++i)
    {
    strings[i] = 0;
    }
  return strings;
}

//----------------------------------------------------------------------------
void vtkXMLWriter::DestroyStringArray(int numStrings, char** strings)
{
  int i;
  for(i=0; i < numStrings; ++i)
    {
    if(strings[i])
      {
      delete [] strings[i];
      }
    }
  delete strings;
}

//----------------------------------------------------------------------------
int vtkXMLWriter::IsSafeToWrite()
{
  // Make sure we have input.
  if(!this->Inputs || !this->Inputs[0])
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }
  
  // Make sure we have a file to write.
  if(!this->FileName)
    {
    vtkErrorMacro("Write() called with no FileName set.");
    return 0;
    }
  return 1;
}
