/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataParser.h"

#include "vtkBase64InputStream.h"
#include "vtkByteSwap.h"
#include "vtkCommand.h"
#include "vtkDataCompressor.h"
#include "vtkInputStream.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"

#include <vtksys/ios/sstream>

#include "vtkXMLUtilities.h"


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

  this->Abort = 0;
  this->Progress = 0;

  // Default byte order to that of this machine.
#ifdef VTK_WORDS_BIGENDIAN
  this->ByteOrder = vtkXMLDataParser::BigEndian;
#else
  this->ByteOrder = vtkXMLDataParser::LittleEndian;
#endif

  this->AttributesEncoding = VTK_ENCODING_NONE;

  // Have specialized methods for reading array data both inline or
  // appended, however typical tags may use the more general CharacterData
  // methods.
  this->IgnoreCharacterData = 0;
}

//----------------------------------------------------------------------------
vtkXMLDataParser::~vtkXMLDataParser()
{
  this->FreeAllElements();
  delete [] this->OpenElements;
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
  os << indent << "Progress: " << this->Progress << "\n";
  os << indent << "Abort: " << this->Abort << "\n";
  os << indent << "AttributesEncoding: " << this->AttributesEncoding << "\n";
}

//----------------------------------------------------------------------------
int vtkXMLDataParser::Parse()
{
  // Delete any elements left from previous parsing.
  this->FreeAllElements();

  // Parse the input from the stream.
  int result = this->Superclass::Parse();

  // Check that the input is okay.
  if(result && !this->CheckPrimaryAttributes())
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
int vtkXMLDataParser::Parse(const char*, unsigned int)
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
  vtkXMLUtilities::ReadElementFromAttributeArray(element, atts, this->AttributesEncoding);

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
void vtkXMLDataParser::SeekInlineDataPosition(vtkXMLDataElement *element)
{
  istream* stream = this->GetStream();
  if(!element->GetInlineDataPosition())
    {
    // Scan for the start of the actual inline data.
    char c=0;
    stream->clear(stream->rdstate() & ~ios::eofbit);
    stream->clear(stream->rdstate() & ~ios::failbit);
    this->SeekG(element->GetXMLByteIndex());
    while(stream->get(c) && (c != '>'))
      {
      ;
      }
    while(stream->get(c) && element->IsSpace(c))
      {
      ;
      }
    unsigned long pos = this->TellG();
    element->SetInlineDataPosition(pos-1);
    }

  // Seek to the data position.
  this->SeekG(element->GetInlineDataPosition());
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
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::FindAppendedDataPosition()
{
  // Clear stream fail and eof bits.  We may have already read past
  // the end of the stream when processing the AppendedData element.
  this->Stream->clear(this->Stream->rdstate() & ~ios::failbit);
  this->Stream->clear(this->Stream->rdstate() & ~ios::eofbit);

  // Scan for the start of the actual appended data.
  char c=0;
  long returnPosition = this->TellG();
  this->SeekG(this->GetXMLByteIndex());
  while(this->Stream->get(c) && (c != '>'))
    {
    ;
    }
  while(this->Stream->get(c) && this->IsSpace(c))
    {
    ;
    }

  // Store the start of the appended data.  We skip the first
  // character because it is always a "_".
  this->AppendedDataPosition = this->TellG();

  // If first character was not an underscore, assume it is part of
  // the data.
  if(c != '_')
    {
    vtkWarningMacro("First character in AppendedData is ASCII value "
                    << int(c) << ", not '_'.  Scan for first character "
                    << "started from file position "
                    << this->GetXMLByteIndex()
                    << ".  The return position is " << returnPosition << ".");
    --this->AppendedDataPosition;
    }

  // Restore the stream position.
  this->SeekG(returnPosition);
}

//----------------------------------------------------------------------------
vtkXMLDataParser::OffsetType
vtkXMLDataParser::FindInlineDataPosition(OffsetType start)
{
  // Scan for the start of the actual inline data.
  char c=0;
  this->SeekG(start);
  while(this->Stream->get(c) && (c != '>'))
    {
    ;
    }
  while(this->Stream->get(c) && this->IsSpace(c))
    {
    ;
    }

  // Make sure some data were found.
  if(c == '<') { return 0; }
  OffsetType pos = this->TellG();
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
void vtkXMLDataParser::FreeAllElements()
{
  while(this->NumberOfOpenElements > 0)
    {
    --this->NumberOfOpenElements;
    this->OpenElements[this->NumberOfOpenElements]->Delete();
    this->OpenElements[this->NumberOfOpenElements] = 0;
    }
  if(this->RootElement)
    {
    this->RootElement->Delete();
    this->RootElement = 0;
    }
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
      char c=0;
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
template <class T>
unsigned long vtkXMLDataParserGetWordTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
unsigned long vtkXMLDataParser::GetWordTypeSize(int wordType)
{
  unsigned long size = 1;
  switch (wordType)
    {
    vtkTemplateMacro(
      size = vtkXMLDataParserGetWordTypeSize(static_cast<VTK_TT*>(0))
      );
    default:
      { vtkWarningMacro("Unsupported data type: " << wordType); } break;
    }
  return size;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::PerformByteSwap(void* data, OffsetType numWords,
                                       int wordSize)
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
  HeaderType headerBuffer[3];
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
  this->PerformByteSwap(headerBuffer, 3, sizeof(HeaderType));

  // Get the standard values.
  this->NumberOfBlocks = headerBuffer[0];
  this->BlockUncompressedSize = headerBuffer[1];
  this->PartialLastBlockUncompressedSize = headerBuffer[2];

  // Allocate the size and offset parts of the header.
  if(this->BlockCompressedSizes)
    {
    delete [] this->BlockCompressedSizes;
    this->BlockCompressedSizes = 0;
    }
  if(this->BlockStartOffsets)
    {
    delete [] this->BlockStartOffsets;
    this->BlockStartOffsets = 0;
    }
  if(this->NumberOfBlocks > 0)
    {
    this->BlockCompressedSizes = new HeaderType[this->NumberOfBlocks];
    this->BlockStartOffsets = new OffsetType[this->NumberOfBlocks];

    buffer = reinterpret_cast<unsigned char*>(&this->BlockCompressedSizes[0]);

    // Read the compressed block sizes.
    unsigned long len = this->NumberOfBlocks*sizeof(HeaderType);
    if(this->DataStream->Read(buffer, len) < len)
      {
      vtkErrorMacro("Error reading compression header.");
      return;
      }

    // Byte swap the sizes to make sure the values are correct.
    this->PerformByteSwap(buffer, this->NumberOfBlocks, sizeof(HeaderType));
    }

  this->DataStream->EndReading();

  // Use the compressed block sizes to calculate the starting offset
  // of each block.
  OffsetType offset = 0;
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
  OffsetType uncompressedSize = this->FindBlockSize(block);
  unsigned int compressedSize = this->BlockCompressedSizes[block];

  if(!this->DataStream->Seek(this->BlockStartOffsets[block]))
    {
    return 0;
    }
  
  unsigned char* readBuffer = new unsigned char[compressedSize];
  
  if(this->DataStream->Read(readBuffer, compressedSize) < compressedSize)
    {
    delete [] readBuffer;
    return 0;
    }

  OffsetType result =
    this->Compressor->Uncompress(readBuffer, compressedSize,
                                 buffer, uncompressedSize);

  delete [] readBuffer;
  return result > 0;
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
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadUncompressedData(unsigned char* data,
                                       OffsetType startWord,
                                       OffsetType numWords,
                                       int wordSize)
{
  // First read the length of the data.
  HeaderType rsize;
  const unsigned long len = sizeof(HeaderType);
  unsigned char* p = reinterpret_cast<unsigned char*>(&rsize);
  if(this->DataStream->Read(p, len) < len) { return 0; }
  this->PerformByteSwap(&rsize, 1, len);

  // Adjust the size to be a multiple of the wordSize by taking
  // advantage of integer division.  This will only change the value
  // when the input file is invalid.
  OffsetType size = (rsize/wordSize)*wordSize;

  // Convert the start/length into bytes.
  OffsetType offset = startWord*wordSize;
  OffsetType length = numWords*wordSize;

  // Make sure the begin/end offsets fall within total size.
  if(offset > size)
    {
    return 0;
    }
  OffsetType end = offset+length;
  if(end > size)
    {
    end = size;
    }
  length = end-offset;

  // Read the data.
  if(!this->DataStream->Seek(offset+len))
    {
    return 0;
    }

  // Read data in 2MB blocks and report progress.
  const long blockSize = 2097152;
  long left = length;
  p = data;
  this->UpdateProgress(0);
  while(left > 0 && !this->Abort)
    {
    // Read this block.
    long n = (blockSize < left)? blockSize:left;
    if(!this->DataStream->Read(p, n))
      {
      return 0;
      }

    // Byte swap this block.  Note that n will always be an integer
    // multiple of the word size.
    this->PerformByteSwap(p, n / wordSize, wordSize);

    // Update pointer and counter.
    p += n;
    left -= n;

    // Report progress.
    this->UpdateProgress(float(p-data)/length);
    }
  this->UpdateProgress(1);
  return length/wordSize;
}

//----------------------------------------------------------------------------
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadCompressedData(unsigned char* data,
                                     OffsetType startWord,
                                     OffsetType numWords,
                                     int wordSize)
{
  // Make sure there are data.
  if(numWords == 0)
    {
    return 0;
    }

  // Find the begin and end offsets into the data.
  OffsetType beginOffset = startWord*wordSize;
  OffsetType endOffset = beginOffset+numWords*wordSize;

  // Find the total size of the data.
  OffsetType totalSize = this->NumberOfBlocks*this->BlockUncompressedSize;
  if(this->PartialLastBlockUncompressedSize)
    {
    totalSize -= this->BlockUncompressedSize;
    totalSize += this->PartialLastBlockUncompressedSize;
    }

  // Adjust the size to be a multiple of the wordSize by taking
  // advantage of integer division.  This will only change the value
  // when the input file is invalid.
  totalSize = (totalSize/wordSize)*wordSize;

  // Make sure the begin/end offsets fall within the total size.
  if(beginOffset > totalSize)
    {
    return 0;
    }
  if(endOffset > totalSize)
    {
    endOffset = totalSize;
    }

  // Find the range of compression blocks to read.
  unsigned int firstBlock = beginOffset / this->BlockUncompressedSize;
  unsigned int lastBlock = endOffset / this->BlockUncompressedSize;

  // Find the offset into the first block where the data begin.
  unsigned int beginBlockOffset =
    beginOffset - firstBlock*this->BlockUncompressedSize;

  // Find the offset into the last block where the data end.
  unsigned int endBlockOffset =
    endOffset - lastBlock*this->BlockUncompressedSize;

  this->UpdateProgress(0);
  if(firstBlock == lastBlock)
    {
    // Everything fits in one block.
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer) { return 0; }
    long n = endBlockOffset - beginBlockOffset;
    memcpy(data, blockBuffer+beginBlockOffset, n);
    delete [] blockBuffer;

    // Byte swap this block.  Note that n will always be an integer
    // multiple of the word size.
    this->PerformByteSwap(data, n / wordSize, wordSize);
    }
  else
    {
    // Read all the complete blocks first.
    OffsetType length = endOffset - beginOffset;
    unsigned char* outputPointer = data;
    OffsetType blockSize = this->FindBlockSize(firstBlock);

    // Read the first block.
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer)
      {
      return 0;
      }
    long n = blockSize-beginBlockOffset;
    memcpy(outputPointer, blockBuffer+beginBlockOffset, n);
    delete [] blockBuffer;

    // Byte swap the first block.  Note that n will always be an
    // integer multiple of the word size.
    this->PerformByteSwap(outputPointer, n / wordSize, wordSize);

    // Advance the pointer to the beginning of the second block.
    outputPointer += blockSize-beginBlockOffset;

    // Report progress.
    this->UpdateProgress(float(outputPointer-data)/length);

    unsigned int currentBlock = firstBlock+1;
    for(;currentBlock != lastBlock && !this->Abort; ++currentBlock)
      {
      // Read this block.
      if(!this->ReadBlock(currentBlock, outputPointer)) { return 0; }

      // Byte swap this block.  Note that blockSize will always be an
      // integer multiple of the word size.
      this->PerformByteSwap(outputPointer, blockSize / wordSize, wordSize);

      // Advance the pointer to the beginning of the next block.
      outputPointer += this->FindBlockSize(currentBlock);

      // Report progress.
      this->UpdateProgress(float(outputPointer-data)/length);
      }

    // Now read the final block, which is incomplete if it exists.
    if(endBlockOffset > 0 && !this->Abort)
      {
      blockBuffer = this->ReadBlock(lastBlock);
      if(!blockBuffer)
        {
        return 0;
        }
      memcpy(outputPointer, blockBuffer, endBlockOffset);
      delete [] blockBuffer;

      // Byte swap the partial block.  Note that endBlockOffset will
      // always be an integer multiple of the word size.
      this->PerformByteSwap(outputPointer, endBlockOffset / wordSize, wordSize);
      }
    }
  this->UpdateProgress(1);

  // Return the total words actually read.
  return (endOffset - beginOffset)/wordSize;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataParser::GetRootElement()
{
  return this->RootElement;
}

//----------------------------------------------------------------------------
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadBinaryData(void* in_buffer,
                                 OffsetType startWord,
                                 OffsetType numWords,
                                 int wordType)
{
  // Skip real read if aborting.
  if(this->Abort)
    {
    return 0;
    }

  unsigned long wordSize = this->GetWordTypeSize(wordType);
  void* buffer = in_buffer;

  // Make sure our streams are setup correctly.
  this->DataStream->SetStream(this->Stream);

  // Read the data.
  unsigned char* d = reinterpret_cast<unsigned char*>(buffer);
  OffsetType actualWords;
  if(this->Compressor)
    {
    this->ReadCompressionHeader();
    this->DataStream->StartReading();
    actualWords = this->ReadCompressedData(d, startWord, numWords, wordSize);
    this->DataStream->EndReading();
    }
  else
    {
    this->DataStream->StartReading();
    actualWords = this->ReadUncompressedData(d, startWord, numWords, wordSize);
    this->DataStream->EndReading();
    }

  // Return the actual amount read.
  return this->Abort? 0:actualWords;
}

//----------------------------------------------------------------------------
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadAsciiData(void* buffer,
                                OffsetType startWord,
                                OffsetType numWords,
                                int wordType)
{
  // Skip real read if aborting.
  if(this->Abort)
    {
    return 0;
    }

  // We assume that ascii data are not very large and parse the entire
  // block into memory.
  this->UpdateProgress(0);

  // Parse the ascii data from the file.
  if(!this->ParseAsciiData(wordType)) { return 0; }

  // Make sure we don't read outside the range of data available.
  OffsetType endWord = startWord + numWords;
  if(this->AsciiDataBufferLength < startWord) { return 0; }
  if(endWord > this->AsciiDataBufferLength)
    {
    endWord = this->AsciiDataBufferLength;
    }
  unsigned long wordSize = this->GetWordTypeSize(wordType);
  OffsetType actualWords = endWord - startWord;
  OffsetType actualBytes = wordSize*actualWords;
  OffsetType startByte = wordSize*startWord;

  this->UpdateProgress(0.5);

  // Copy the data from the pre-parsed ascii data buffer.
  memcpy(buffer, this->AsciiDataBuffer+startByte, actualBytes);

  this->UpdateProgress(1);

  return this->Abort? 0:actualWords;
}

//----------------------------------------------------------------------------
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadInlineData(vtkXMLDataElement* element,
                                 int isAscii, void* buffer,
                                 OffsetType startWord,
                                 OffsetType numWords,
                                 int wordType)
{
  this->DataStream = this->InlineDataStream;
  this->SeekInlineDataPosition(element);
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
vtkXMLDataParser::OffsetType
vtkXMLDataParser::ReadAppendedData(OffsetType offset,
                                   void* buffer,
                                   OffsetType startWord,
                                   OffsetType numWords,
                                   int wordType)
{
  this->DataStream = this->AppendedDataStream;
  this->SeekG(this->AppendedDataPosition+offset);
  return this->ReadBinaryData(buffer, startWord, numWords, wordType);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Define a parsing function template.  The extra "long" argument is used
// to help broken compilers select the non-templates below for char and
// unsigned char by making them a better conversion than the template.
template <class T>
T* vtkXMLParseAsciiData(istream& is, int* length, T*, long)
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
char* vtkXMLParseAsciiData(istream& is, int* length, char*, int)
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
unsigned char* vtkXMLParseAsciiData(istream& is, int* length, unsigned char*,
                                    int)
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
signed char* vtkXMLParseAsciiData(istream& is, int* length, signed char*,
                                  int)
{
  int dataLength = 0;
  int dataBufferSize = 64;

  signed char* dataBuffer = new signed char[dataBufferSize];
  signed char element;
  short inElement;

  while(is >> inElement)
    {
    element = inElement;
    if(dataLength == dataBufferSize)
      {
      int newSize = dataBufferSize*2;
      signed char* newBuffer = new signed char[newSize];
      memcpy(newBuffer, dataBuffer, dataLength*sizeof(signed char));
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
  if(this->AsciiDataPosition == static_cast<OffsetType>(this->TellG()))
    {
    return (this->AsciiDataBuffer? 1:0);
    }

  // Prepare for new data.
  this->AsciiDataPosition = this->TellG();
  if(this->AsciiDataBuffer) { this->FreeAsciiBuffer(); }

  int length = 0;
  void* buffer = 0;
  switch (wordType)
    {
    vtkTemplateMacro(
      buffer = vtkXMLParseAsciiData(is, &length, static_cast<VTK_TT*>(0), 1)
      );
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
template <class T>
void vtkXMLDataParserFreeAsciiBuffer(T* buffer)
{
  delete [] buffer;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::FreeAsciiBuffer()
{
  void* buffer = this->AsciiDataBuffer;
  switch (this->AsciiDataWordType)
    {
    vtkTemplateMacro(
      vtkXMLDataParserFreeAsciiBuffer(static_cast<VTK_TT*>(buffer))
      );
    }
  this->AsciiDataBuffer = 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataParser::UpdateProgress(float progress)
{
  this->Progress = progress;
  double dProgress=progress;
  this->InvokeEvent(vtkCommand::ProgressEvent, &dProgress);
}
