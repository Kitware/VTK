/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLDataParser.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXMLDataParser.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkByteSwap.h"
#include "vtkInputStream.h"
#include "vtkBase64InputStream.h"
#include "vtkDataCompressor.h"

vtkCxxRevisionMacro(vtkXMLDataParser, "1.3");
vtkStandardNewMacro(vtkXMLDataParser);
vtkCxxSetObjectMacro(vtkXMLDataParser, Compressor, vtkDataCompressor);

//----------------------------------------------------------------------------
vtkXMLDataParser::vtkXMLDataParser()
{
  this->NumberOfOpenElements = 0;
  this->OpenElementsSize = 10;
  this->OpenElements = new vtkXMLDataElement*[this->OpenElementsSize];
  this->RootElement = 0;
  this->AppendedDataPosition = 0;
  this->AppendedDataMatched = 0;
  this->DataStream = 0;
  this->InlineDataStream = vtkBase64InputStream::New();
  this->AppendedDataStream = vtkBase64InputStream::New();
  
  this->BlockCompressedSizes = 0;
  this->BlockStartOffsets = 0;
  this->Compressor = 0;

  this->AsciiDataBuffer = 0;
  this->AsciiDataBufferLength = 0;
  this->AsciiDataPosition = 0;
  
  // Default byte order to that of this machine.
#ifdef VTK_WORDS_BIGENDIAN
  this->ByteOrder = vtkXMLDataParser::BigEndian;
#else
  this->ByteOrder = vtkXMLDataParser::LittleEndian;
#endif  

  // Input id type defaults to that compiled in.
#ifdef VTK_USE_64BIT_IDS
  this->IdType = vtkXMLDataParser::Int64;
#else
  this->IdType = vtkXMLDataParser::Int32;
#endif
}

//----------------------------------------------------------------------------
vtkXMLDataParser::~vtkXMLDataParser()
{
  unsigned int i;
  for(i=0;i < this->NumberOfOpenElements;++i)
    {
    this->OpenElements[i]->Delete();
    }
  delete [] this->OpenElements;
  if(this->RootElement)
    {
    this->RootElement->Delete();
    }
  this->InlineDataStream->Delete();
  this->AppendedDataStream->Delete();
  if(this->BlockCompressedSizes) { delete [] this->BlockCompressedSizes; }
  if(this->BlockStartOffsets) { delete [] this->BlockStartOffsets; }
  this->SetCompressor(0);
  if(this->AsciiDataBuffer) { this->FreeAsciiBuffer(); }
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendedDataPosition: "
     << this->AppendedDataPosition << "\n";
  if(this->RootElement)
    {
    this->RootElement->PrintXML(os, indent);
    }
  if(this->Compressor)
    {
    os << indent << "Compressor: " << this->Compressor << "\n";
    }
  else
    {
    os << indent << "Compressor: (none)\n";
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::Parse()
{
  // Parse the input from the stream.
  int result = this->Superclass::Parse();
  
  // Check that the input is okay.
  if(!this->CheckPrimaryAttributes())
    {
    result = 0;
    }
  
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::Parse(const char*)
{
  vtkErrorMacro("Parsing from a string is not supported.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::StartElement(const char* name, const char** atts)
{
  vtkXMLDataElement* element = vtkXMLDataElement::New();
  element->SetName(name);
  element->SetXMLByteIndex(this->GetXMLByteIndex());
  element->ReadXMLAttributes(atts);
  const char* id = element->GetAttribute("id");
  if(id)
    {
    element->SetId(id);
    }
  this->PushOpenElement(element);
  
  if(strcmp(name, "AppendedData") == 0)
    {
    // This is the AppendedData element.
    this->FindAppendedDataPosition();
    
    // Switch to raw decoder if necessary.
    const char* encoding = element->GetAttribute("encoding");
    if(encoding && (strcmp(encoding, "raw") == 0))
      {
      this->AppendedDataStream->Delete();
      this->AppendedDataStream = vtkInputStream::New();
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::EndElement(const char*)
{
  vtkXMLDataElement* finished = this->PopOpenElement();
  unsigned int numOpen = this->NumberOfOpenElements;
  if(numOpen > 0)
    {
    this->OpenElements[numOpen-1]->AddNestedElement(finished);
    finished->Delete();
    }
  else
    {
    this->RootElement = finished;
    }  
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::ParsingComplete()
{
  // If we have reached the appended data section, we stop parsing.
  // This prevents the XML parser from having to walk over the entire
  // appended data section.
  if(this->AppendedDataPosition) { return 1; }
  return this->Superclass::ParsingComplete();
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::ClearStreamEOF()
{
  // Clear the fail bit on the input stream.  This allows code to go
  // back to read more data after the end-of-file has been reached.
  this->Stream->clear(this->Stream->rdstate() & ~ios::eofbit);
  this->Stream->clear(this->Stream->rdstate() & ~ios::failbit);
} 

//----------------------------------------------------------------------------
int vtkXMLDataParser::CheckPrimaryAttributes()
{
  const char* byte_order = this->RootElement->GetAttribute("byte_order");
  if(byte_order)
    {
    if(strcmp(byte_order, "BigEndian") == 0)
      {
      this->ByteOrder = vtkXMLDataParser::BigEndian;
      }
    else if(strcmp(byte_order, "LittleEndian") == 0)
      {
      this->ByteOrder = vtkXMLDataParser::LittleEndian;
      }
    else
      {
      vtkErrorMacro("Unsupported byte_order=\"" << byte_order << "\"");
      return 0;
      }
    }
  const char* id_type = this->RootElement->GetAttribute("id_type");
  if(id_type)
    {
    if(strcmp(id_type, "Int32") == 0)
      {
      this->IdType = vtkXMLDataParser::Int32;
      }
    else if(strcmp(id_type, "Int64") == 0)
      {
#ifdef VTK_USE_64BIT_IDS
      this->IdType = vtkXMLDataParser::Int64;
#else
      vtkErrorMacro("Int64 support not compiled in VTK.");
      return 0;
#endif
      }
    else
      {
      vtkErrorMacro("Unsupported id_type=\"" << id_type << "\"");
      return 0;
      }    
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::FindAppendedDataPosition()
{
  // Scan for the start of the actual appended data.
  char c;
  unsigned long returnPosition = this->Stream->tellg();
  this->ClearStreamEOF();
  this->Stream->seekg(this->GetXMLByteIndex());
  while(this->Stream->get(c) && (c != '>'));
  while(this->Stream->get(c) && this->IsSpace(c));
  
  // Store the start of the appended data.  We skip the first
  // character because it is always a "_".
  this->AppendedDataPosition = this->Stream->tellg();
  
  // If first character was not an underscore, assume it is part of
  // the data.
  if(c != '_')
    {
    vtkWarningMacro("First character in AppendedData is "
                    << int(c) << ", not '_'");
    --this->AppendedDataPosition;
    }
  
  // Restore the stream position.
  this->Stream->seekg(returnPosition);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::FindInlineDataPosition(unsigned long start)
{
  // Scan for the start of the actual inline data.
  char c;
  this->Stream->seekg(start);
  this->ClearStreamEOF();
  while(this->Stream->get(c) && (c != '>'));
  while(this->Stream->get(c) && this->IsSpace(c));
  
  // Make sure some data were found.
  if(c == '<') { return 0; }
  unsigned long pos = this->Stream->tellg();
  return (pos-1);
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::PushOpenElement(vtkXMLDataElement* element)
{
  if(this->NumberOfOpenElements == this->OpenElementsSize)
    {
    unsigned int newSize = this->OpenElementsSize*2;
    vtkXMLDataElement** newOpenElements = new vtkXMLDataElement*[newSize];
    unsigned int i;
    for(i=0; i < this->NumberOfOpenElements;++i)
      {
      newOpenElements[i] = this->OpenElements[i];
      }
    delete [] this->OpenElements;
    this->OpenElements = newOpenElements;
    this->OpenElementsSize = newSize;
    }
  
  unsigned int pos = this->NumberOfOpenElements++;
  this->OpenElements[pos] = element;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataParser::PopOpenElement()
{
  if(this->NumberOfOpenElements > 0)
    {
    --this->NumberOfOpenElements;
    return this->OpenElements[this->NumberOfOpenElements];
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::ParseBuffer(const char* buffer, unsigned int count)
{
  // Parsing must stop when "<AppendedData" is reached.  Use a search
  // similar to the KMP string search algorithm.
  const char pattern[] = "<AppendedData";
  const int length = sizeof(pattern)-1;
  
  const char* s = buffer;
  const char* end = buffer + count;
  int matched = this->AppendedDataMatched;
  while(s != end)
    {
    char c = *s++;
    if(c == pattern[matched]) { if(++matched == length) { break; } }
    else { matched = (c == pattern[0])? 1:0; }
    }
  this->AppendedDataMatched = matched;
  
  // Parse as much of the buffer as is safe.
  if(!this->Superclass::ParseBuffer(buffer, s - buffer)) { return 0; }
  
  // If we have reached the appended data, artificially finish the
  // document.
  if(matched == length)
    {
    // Parse the rest of the element's opening tag.
    const char* t = s;
    char prev = 0;
    while((t != end) && (*t != '>')) { ++t; }
    if(!this->Superclass::ParseBuffer(s, t-s)) { return 0; }
    if(t > s) { prev = *(t-1); }
    
    if(t == end)
      {
      // Scan for the real end of the element's opening tag.
      char c;
      while(this->Stream->get(c) && (c != '>'))
        {
        prev = c;
        if(!this->Superclass::ParseBuffer(&c, 1)) { return 0; }
        }
      }
    
    // Artificially end the AppendedData element.
    if(prev != '/')
      {
      if(!this->Superclass::ParseBuffer("/", 1)) { return 0; }
      }
    if(!this->Superclass::ParseBuffer(">", 1)) { return 0; }
    
    // Artificially end the VTKFile element.
    const char finish[] = "\n</VTKFile>\n";
    if(!this->Superclass::ParseBuffer(finish, sizeof(finish)-1)) { return 0; }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::GetWordTypeSize(int wordType)
{
  unsigned long size = 1;
  switch (wordType)
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
      { vtkWarningMacro("Unsupported data type: " << wordType); } break;
    }
  return size;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::PerformByteSwap(void* data, int numWords, int wordSize)
{
  char* ptr = static_cast<char*>(data);
  if(this->ByteOrder == vtkXMLDataParser::BigEndian)
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
void vtkXMLDataParser::ReadCompressionHeader()
{  
  unsigned int headerBuffer[3];
  const int headerSize = sizeof(headerBuffer);
  unsigned char* buffer = reinterpret_cast<unsigned char*>(&headerBuffer[0]);
  
  this->DataStream->StartReading();
  
  // Read the standard part of the header.
  int r = this->DataStream->Read(buffer, headerSize);
  if(r < headerSize)
    {
    vtkErrorMacro("Error reading beginning of compression header.  Read "
                  << r << " of " << headerSize << " bytes.");
    return;
    }
  
  // Byte swap the header to make sure the values are correct.
  this->PerformByteSwap(headerBuffer, 3, sizeof(unsigned int));
  
  // Get the standard values.
  this->NumberOfBlocks = headerBuffer[0];
  this->BlockUncompressedSize = headerBuffer[1];
  this->PartialLastBlockUncompressedSize = headerBuffer[2];
  
  // Allocate the size and offset parts of the header.
  if(this->BlockCompressedSizes) { delete [] this->BlockCompressedSizes; }
  this->BlockCompressedSizes = new unsigned int[this->NumberOfBlocks];
  
  if(this->BlockStartOffsets) { delete [] this->BlockStartOffsets; }
  this->BlockStartOffsets = new unsigned long[this->NumberOfBlocks];
  
  buffer = reinterpret_cast<unsigned char*>(&this->BlockCompressedSizes[0]);
  
  // Read the compressed block sizes.
  unsigned long len = this->NumberOfBlocks*sizeof(unsigned int);
  if(this->DataStream->Read(buffer, len) < len)
    {
    vtkErrorMacro("Error reading compression header.");
    return;
    }
  
  // Byte swap the sizes to make sure the values are correct.
  this->PerformByteSwap(buffer, this->NumberOfBlocks, sizeof(unsigned int));
  
  this->DataStream->EndReading();
  
  // Use the compressed block sizes to calculate the starting offset
  // of each block.
  unsigned long offset = 0;
  unsigned int i;
  for(i=0;i < this->NumberOfBlocks;++i)
    {
    this->BlockStartOffsets[i] = offset;
    offset += this->BlockCompressedSizes[i];
    }
}

//----------------------------------------------------------------------------
unsigned int vtkXMLDataParser::FindBlockSize(unsigned int block)
{
  if(block < this->NumberOfBlocks-(this->PartialLastBlockUncompressedSize?1:0))
    {
    return this->BlockUncompressedSize;
    }
  else
    {
    return this->PartialLastBlockUncompressedSize;
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::ReadBlock(unsigned int block, unsigned char* buffer)
{
  unsigned long uncompressedSize = this->FindBlockSize(block);
  unsigned int compressedSize = this->BlockCompressedSizes[block];
  unsigned char* readBuffer = new unsigned char[compressedSize];
  
  if(!this->DataStream->Seek(this->BlockStartOffsets[block])) { return 0; }
  if(this->DataStream->Read(readBuffer, compressedSize) < compressedSize)
    { return 0; }
  
  unsigned long result =
    this->Compressor->Uncompress(readBuffer, compressedSize,
                                 buffer, uncompressedSize);
  
  delete [] readBuffer;
  return (result > 0)? 1:0;
}

//----------------------------------------------------------------------------
unsigned char* vtkXMLDataParser::ReadBlock(unsigned int block)
{
  unsigned char* decompressBuffer =
    new unsigned char[this->FindBlockSize(block)];
  if(!this->ReadBlock(block, decompressBuffer))
    {
    delete [] decompressBuffer;
    return 0;
    }
  return decompressBuffer;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadUncompressedData(unsigned char* data,
                                                     unsigned long offset,
                                                     unsigned long length)
{
  // First read the length of the data.
  unsigned int size;
  const unsigned long len = sizeof(unsigned int);
  unsigned char* p = reinterpret_cast<unsigned char*>(&size);
  if(this->DataStream->Read(p, len) < len) { return 0; }
  this->PerformByteSwap(&size, 1, len);
  
  // Make sure the begin/end offsets fall within total size.
  if(offset > size) { return 0; }
  unsigned long end = offset+length;
  if(end > size) { end = size; }
  
  // Read the data.
  if(!this->DataStream->Seek(offset+len)) { return 0; }
  return this->DataStream->Read(data, end-offset);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadCompressedData(unsigned char* data,
                                                   unsigned long offset,
                                                   unsigned long length)
{
  // Find the begin and end offsets into the data.
  unsigned long beginOffset = offset;
  unsigned long endOffset = offset+length;
  
  // Find the total size of the data and make sure the begin/end offsets
  // fall within it.
  unsigned long totalSize = this->NumberOfBlocks*this->BlockUncompressedSize;
  if(this->PartialLastBlockUncompressedSize)
    {
    totalSize -= this->BlockUncompressedSize;
    totalSize += this->PartialLastBlockUncompressedSize;
    }
  if(beginOffset > totalSize) { return 0; }
  if(endOffset > totalSize) { endOffset = totalSize; }
  
  // Find the range of compression blocks to read.
  unsigned int firstBlock = beginOffset / this->BlockUncompressedSize;
  unsigned int lastBlock = endOffset / this->BlockUncompressedSize;
  
  unsigned int beginBlockOffset =
    beginOffset - firstBlock*this->BlockUncompressedSize;
  unsigned int endBlockOffset =
    endOffset - lastBlock*this->BlockUncompressedSize;
  
  if(firstBlock == lastBlock)
    {
    // Everything fits in one block.
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer) { return 0; }
    memcpy(data, blockBuffer+beginBlockOffset,
           endBlockOffset - beginBlockOffset);
    delete [] blockBuffer;
    }
  else
    {
    // Read all the complete blocks first.
    unsigned char* outputPointer = data;
    unsigned long blockSize = this->FindBlockSize(firstBlock);
    
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer) { return 0; }
    memcpy(outputPointer, blockBuffer+beginBlockOffset,
           blockSize-beginBlockOffset);
    delete [] blockBuffer;
    
    outputPointer += blockSize-beginBlockOffset;
    
    unsigned int currentBlock = firstBlock+1;
    for(;currentBlock != lastBlock; ++currentBlock)
      {
      if(!this->ReadBlock(currentBlock, outputPointer)) { return 0; }
      outputPointer += this->FindBlockSize(currentBlock);
      }
    
    // Now read the final block, which is incomplete if it exists.
    if(endBlockOffset > 0)
      {
      blockBuffer = this->ReadBlock(lastBlock);
      if(!blockBuffer) { return 0; }
      memcpy(outputPointer, blockBuffer, endBlockOffset);
      delete [] blockBuffer;
      }
    }
  
  // Return the total size actually read.
  return (endOffset - beginOffset);
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataParser::GetRootElement()
{
  return this->RootElement;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadBinaryData(void* in_buffer, int startWord,
                                               int numWords, int wordType)
{
  unsigned long wordSize = this->GetWordTypeSize(wordType);  
  void* buffer = in_buffer;
  
  // Make sure our streams are setup correctly.
  this->DataStream->SetStream(this->Stream);
  this->ClearStreamEOF();
  
#ifdef VTK_USE_64BIT_IDS
  // If the type is vtkIdType, it may need to be converted from the type
  // in the file to the real vtkIdType.
  int* intBuffer = 0;
  if((wordType == VTK_ID_TYPE) && (this->IdType == vtkXMLDataParser::Int32))
    {
    intBuffer = new int[numWords];
    wordSize = this->GetWordTypeSize(VTK_INT);
    buffer = intBuffer;
    }
#endif
  
  // Read the data.
  unsigned char* d = reinterpret_cast<unsigned char*>(buffer);
  unsigned long startByte = startWord*wordSize;
  unsigned long numBytes = numWords*wordSize;
  unsigned long actualBytes = 0;
  if(this->Compressor)
    {
    this->ReadCompressionHeader();
    this->DataStream->StartReading();
    actualBytes = this->ReadCompressedData(d, startByte, numBytes);
    this->DataStream->EndReading();
    }
  else
    {
    this->DataStream->StartReading();
    actualBytes = this->ReadUncompressedData(d, startByte, numBytes);
    this->DataStream->EndReading();
    }
  
  // Byte swap.
  unsigned long actualWords = actualBytes / wordSize;
  this->PerformByteSwap(d, actualWords, wordSize);
  
#ifdef VTK_USE_64BIT_IDS
  if(intBuffer)
    {
    // The data were read as integers.  Convert to vtkIdType.
    vtkIdType* idBuffer = static_cast<vtkIdType*>(in_buffer);
    
    int i;
    for(i=0;i < numWords; ++i)
      {
      idBuffer[i] = static_cast<vtkIdType>(intBuffer[i]);
      }
    
    delete [] intBuffer;
    }
#endif  
  
  // Return the actual amount read.
  return actualWords;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadAsciiData(void* buffer, int startWord,
                                              int numWords, int wordType)
{
  // Parse the ascii data from the file.
  if(!this->ParseAsciiData(wordType)) { return 0; }
  
  // Make sure we don't read outside the range of data available.
  int endWord = startWord + numWords;
  if(this->AsciiDataBufferLength < startWord) { return 0; }
  if(endWord > this->AsciiDataBufferLength)
    {
    endWord = this->AsciiDataBufferLength;
    }
  int wordSize = this->GetWordTypeSize(wordType);
  int actualWords = endWord - startWord;
  int actualBytes = wordSize*actualWords;
  int startByte = wordSize*startWord;
  
  // Copy the data from the pre-parsed ascii data buffer.
  memcpy(buffer, this->AsciiDataBuffer+startByte, actualBytes);
  return actualWords;
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadInlineData(vtkXMLDataElement* element,
                                               int isAscii, void* buffer,
                                               int startWord, int numWords,
                                               int wordType)
{
  this->DataStream = this->InlineDataStream;
  element->SeekInlineDataPosition(this);
  if(isAscii)
    {
    return this->ReadAsciiData(buffer, startWord, numWords, wordType);
    }
  else
    {
    return this->ReadBinaryData(buffer, startWord, numWords, wordType);
    }
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::ReadAppendedData(unsigned long offset,
                                                 void* buffer, int startWord,
                                                 int numWords, int wordType)
{
  this->DataStream = this->AppendedDataStream;
  this->Stream->seekg(this->AppendedDataPosition+offset);
  return this->ReadBinaryData(buffer, startWord, numWords, wordType);
}

//----------------------------------------------------------------------------
template <class T>
T* vtkXMLParseAsciiData(istream& is, int* length, T* vtkNotUsed(dummy))
{
  int dataLength = 0;
  int dataBufferSize = 64;
  
  T* dataBuffer = new T[dataBufferSize];
  T element;
  
  while(is >> element)
    {
    if(dataLength == dataBufferSize)
      {
      int newSize = dataBufferSize*2;
      T* newBuffer = new T[newSize];
      memcpy(newBuffer, dataBuffer, dataLength*sizeof(T));
      delete [] dataBuffer;
      dataBuffer = newBuffer;
      dataBufferSize = newSize;
      }
    dataBuffer[dataLength++] = element;
    }
  
  if(length)
    {  
    *length = dataLength;
    }
  
  return dataBuffer;
}

//----------------------------------------------------------------------------
char* vtkXMLParseAsciiDataChar(istream& is, int* length)
{
  int dataLength = 0;
  int dataBufferSize = 64;
  
  char* dataBuffer = new char[dataBufferSize];
  char element;
  short inElement;
  
  while(is >> inElement)
    {
    element = inElement;
    if(dataLength == dataBufferSize)
      {
      int newSize = dataBufferSize*2;
      char* newBuffer = new char[newSize];
      memcpy(newBuffer, dataBuffer, dataLength*sizeof(char));
      delete [] dataBuffer;
      dataBuffer = newBuffer;
      dataBufferSize = newSize;
      }
    dataBuffer[dataLength++] = element;
    }
  
  if(length)
    {  
    *length = dataLength;
    }
  
  return dataBuffer;
}

//----------------------------------------------------------------------------
unsigned char* vtkXMLParseAsciiDataUnsignedChar(istream& is, int* length)
{
  int dataLength = 0;
  int dataBufferSize = 64;
  
  unsigned char* dataBuffer = new unsigned char[dataBufferSize];
  unsigned char element;
  short inElement;
  
  while(is >> inElement)
    {
    element = inElement;
    if(dataLength == dataBufferSize)
      {
      int newSize = dataBufferSize*2;
      unsigned char* newBuffer = new unsigned char[newSize];
      memcpy(newBuffer, dataBuffer, dataLength*sizeof(unsigned char));
      delete [] dataBuffer;
      dataBuffer = newBuffer;
      dataBufferSize = newSize;
      }
    dataBuffer[dataLength++] = element;
    }
  
  if(length)
    {  
    *length = dataLength;
    }
  
  return dataBuffer;
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::ParseAsciiData(int wordType)
{
  istream& is = *(this->Stream);
  
  // Don't re-parse the same ascii data.
  if(this->AsciiDataPosition == (unsigned long)(is.tellg()))
    {
    return (this->AsciiDataBuffer? 1:0);
    }
  
  // Prepare for new data.
  this->AsciiDataPosition = is.tellg();
  if(this->AsciiDataBuffer) { this->FreeAsciiBuffer(); }
  
  int length = 0;
  void* buffer = 0;
  switch (wordType)
    {
    case VTK_ID_TYPE:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<vtkIdType*>(0)); break;
    case VTK_DOUBLE:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<double*>(0)); break;
    case VTK_FLOAT:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<float*>(0)); break;
    case VTK_LONG:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<long*>(0)); break;
    case VTK_UNSIGNED_LONG:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<unsigned long*>(0)); break;
    case VTK_INT:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<int*>(0)); break;
    case VTK_UNSIGNED_INT:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<unsigned int*>(0)); break;
    case VTK_SHORT:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<short*>(0)); break;
    case VTK_UNSIGNED_SHORT:
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<unsigned short*>(0)); break;
    case VTK_CHAR:
      buffer = vtkXMLParseAsciiDataChar(is, &length); break;
    case VTK_UNSIGNED_CHAR:
      buffer = vtkXMLParseAsciiDataUnsignedChar(is, &length); break;
    }
  
  // Read terminated from failure.  Clear the fail bit so another read
  // can take place later.  
  is.clear(is.rdstate() & ~ios::failbit);
  
  // Save the buffer.
  this->AsciiDataBuffer = reinterpret_cast<unsigned char*>(buffer);
  this->AsciiDataBufferLength = length;
  this->AsciiDataWordType = wordType;
  return (this->AsciiDataBuffer? 1:0);
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::FreeAsciiBuffer()
{
  void* buffer = this->AsciiDataBuffer;
  switch (this->AsciiDataWordType)
    {
    case VTK_ID_TYPE:
      delete [] reinterpret_cast<vtkIdType*>(buffer); break;
    case VTK_FLOAT:
      delete [] reinterpret_cast<float*>(buffer); break;
    case VTK_DOUBLE:
      delete [] reinterpret_cast<double*>(buffer); break;
    case VTK_INT:
      delete [] reinterpret_cast<int*>(buffer); break;
    case VTK_UNSIGNED_INT:
      delete [] reinterpret_cast<unsigned int*>(buffer); break;
    case VTK_LONG:
      delete [] reinterpret_cast<long*>(buffer); break;
    case VTK_UNSIGNED_LONG:
      delete [] reinterpret_cast<unsigned long*>(buffer); break;
    case VTK_SHORT:
      delete [] reinterpret_cast<short*>(buffer); break;
    case VTK_UNSIGNED_SHORT:
      delete [] reinterpret_cast<unsigned short*>(buffer); break;
    case VTK_UNSIGNED_CHAR:
      delete [] reinterpret_cast<unsigned char*>(buffer); break;
    case VTK_CHAR:
      delete [] reinterpret_cast<char*>(buffer); break;
    }
  this->AsciiDataBuffer = 0;
}
