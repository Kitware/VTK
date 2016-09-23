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
#define vtkXMLDataHeaderPrivate_DoNotInclude
#include "vtkXMLDataHeaderPrivate.h"
#undef vtkXMLDataHeaderPrivate_DoNotInclude

#include <memory>
#include <sstream>

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
  this->HeaderType = 32;

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
  delete [] this->BlockCompressedSizes;
  delete [] this->BlockStartOffsets;
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
    vtkTypeInt64 pos = this->TellG();
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
  if(const char* header_type = this->RootElement->GetAttribute("header_type"))
  {
    if(strcmp(header_type, "UInt32") == 0)
    {
      this->HeaderType = 32;
    }
    else if(strcmp(header_type, "UInt64") == 0)
    {
      this->HeaderType = 64;
    }
    else
    {
      vtkErrorMacro("Unsupported header_type=\"" << header_type << "\"");
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
  vtkTypeInt64 returnPosition = this->TellG();
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
size_t vtkXMLDataParserGetWordTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------
size_t vtkXMLDataParser::GetWordTypeSize(int wordType)
{
  size_t size = 1;
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
void vtkXMLDataParser::PerformByteSwap(void* data, size_t numWords,
                                       size_t wordSize)
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
int vtkXMLDataParser::ReadCompressionHeader()
{
#if defined(VTK_HAS_STD_UNIQUE_PTR)
  std::unique_ptr<vtkXMLDataHeader>
    ch(vtkXMLDataHeader::New(this->HeaderType, 3));
#else
  std::auto_ptr<vtkXMLDataHeader>
    ch(vtkXMLDataHeader::New(this->HeaderType, 3));
#endif

  this->DataStream->StartReading();

  // Read the standard part of the header.
  size_t const headerSize = ch->DataSize();
  size_t r = this->DataStream->Read(ch->Data(), headerSize);
  if(r < headerSize)
  {
    vtkErrorMacro("Error reading beginning of compression header.  Read "
                  << r << " of " << headerSize << " bytes.");
    return 0;
  }

  // Byte swap the header to make sure the values are correct.
  this->PerformByteSwap(ch->Data(), ch->WordCount(), ch->WordSize());

  // Get the standard values.
  this->NumberOfBlocks = size_t(ch->Get(0));
  this->BlockUncompressedSize = size_t(ch->Get(1));
  this->PartialLastBlockUncompressedSize = size_t(ch->Get(2));

  // Allocate the size and offset parts of the header.
  ch->Resize(this->NumberOfBlocks);
  delete [] this->BlockCompressedSizes;
  this->BlockCompressedSizes = 0;
  delete [] this->BlockStartOffsets;
  this->BlockStartOffsets = 0;
  if(this->NumberOfBlocks > 0)
  {
    this->BlockCompressedSizes = new size_t[this->NumberOfBlocks];
    this->BlockStartOffsets = new vtkTypeInt64[this->NumberOfBlocks];

    // Read the compressed block sizes.
    size_t len = ch->DataSize();
    if(this->DataStream->Read(ch->Data(), len) < len)
    {
      vtkErrorMacro("Error reading compression header.");
      return 0;
    }

    // Byte swap the sizes to make sure the values are correct.
    this->PerformByteSwap(ch->Data(), ch->WordCount(), ch->WordSize());
  }

  this->DataStream->EndReading();

  // Use the compressed block sizes to calculate the starting offset
  // of each block.
  vtkTypeInt64 offset = 0;
  for(size_t i=0;i < this->NumberOfBlocks;++i)
  {
    size_t const sz = size_t(ch->Get(i));
    this->BlockCompressedSizes[i] = sz;
    this->BlockStartOffsets[i] = offset;
    offset += sz;
  }
  return 1;
}

//----------------------------------------------------------------------------
size_t vtkXMLDataParser::FindBlockSize(vtkTypeUInt64 block)
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
int vtkXMLDataParser::ReadBlock(vtkTypeUInt64 block, unsigned char* buffer)
{
  size_t uncompressedSize = this->FindBlockSize(block);
  size_t compressedSize = this->BlockCompressedSizes[block];

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

  size_t result =
    this->Compressor->Uncompress(readBuffer, compressedSize,
                                 buffer, uncompressedSize);

  delete [] readBuffer;
  return result > 0;
}

//----------------------------------------------------------------------------
unsigned char* vtkXMLDataParser::ReadBlock(vtkTypeUInt64 block)
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
size_t vtkXMLDataParser::ReadUncompressedData(unsigned char* data,
                                              vtkTypeUInt64 startWord,
                                              size_t numWords,
                                              size_t wordSize)
{
  // First read the length of the data.
#if defined(VTK_HAS_STD_UNIQUE_PTR)
  std::unique_ptr<vtkXMLDataHeader>
    uh(vtkXMLDataHeader::New(this->HeaderType, 1));
#else
  std::auto_ptr<vtkXMLDataHeader>
    uh(vtkXMLDataHeader::New(this->HeaderType, 1));
#endif
  size_t const headerSize = uh->DataSize();
  size_t r = this->DataStream->Read(uh->Data(), headerSize);
  if(r < headerSize)
  {
    vtkErrorMacro("Error reading uncompressed binary data header.  "
                  "Read " << r << " of " << headerSize << " bytes.");
    return 0;
  }
  this->PerformByteSwap(uh->Data(), uh->WordCount(), uh->WordSize());
  vtkTypeUInt64 rsize = uh->Get(0);

  // Adjust the size to be a multiple of the wordSize by taking
  // advantage of integer division.  This will only change the value
  // when the input file is invalid.
  vtkTypeUInt64 size = (rsize/wordSize)*wordSize;

  // Convert the start/length into bytes.
  vtkTypeUInt64 offset = startWord*wordSize;
  size_t length = numWords*wordSize;

  // Make sure the begin/end offsets fall within total size.
  if(offset > size)
  {
    return 0;
  }
  vtkTypeUInt64 end = offset+length;
  if(end > size)
  {
    end = size;
  }
  length = end-offset;

  // Read the data.
  if(!this->DataStream->Seek(headerSize+offset))
  {
    return 0;
  }

  // Read data in 2MB blocks and report progress.
  size_t const blockSize = 2097152;
  size_t left = length;
  unsigned char* p = data;
  this->UpdateProgress(0);
  while(left > 0 && !this->Abort)
  {
    // Read this block.
    size_t n = (blockSize < left)? blockSize:left;
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
size_t vtkXMLDataParser::ReadCompressedData(unsigned char* data,
                                            vtkTypeUInt64 startWord,
                                            size_t numWords,
                                            size_t wordSize)
{
  // Make sure there are data.
  if(numWords == 0)
  {
    return 0;
  }

  // Find the begin and end offsets into the data.
  vtkTypeUInt64 beginOffset = startWord*wordSize;
  vtkTypeUInt64 endOffset = beginOffset+numWords*wordSize;

  // Find the total size of the data.
  vtkTypeUInt64 totalSize = this->NumberOfBlocks*this->BlockUncompressedSize;
  if(this->PartialLastBlockUncompressedSize)
  {
    totalSize -= this->BlockUncompressedSize;
    totalSize += this->PartialLastBlockUncompressedSize;
  }

  // Make sure there's even data to be read
  if(totalSize == 0)
  {
    return 0;
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
  vtkTypeUInt64 firstBlock = beginOffset / this->BlockUncompressedSize;
  vtkTypeUInt64 lastBlock = endOffset / this->BlockUncompressedSize;

  // Find the offset into the first block where the data begin.
  size_t beginBlockOffset =
    beginOffset - firstBlock*this->BlockUncompressedSize;

  // Find the offset into the last block where the data end.
  size_t endBlockOffset =
    endOffset - lastBlock*this->BlockUncompressedSize;

  this->UpdateProgress(0);
  if(firstBlock == lastBlock)
  {
    // Everything fits in one block.
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer) { return 0; }
    size_t n = endBlockOffset - beginBlockOffset;
    memcpy(data, blockBuffer+beginBlockOffset, n);
    delete [] blockBuffer;

    // Byte swap this block.  Note that n will always be an integer
    // multiple of the word size.
    this->PerformByteSwap(data, n / wordSize, wordSize);
  }
  else
  {
    // Read all the complete blocks first.
    size_t length = endOffset - beginOffset;
    unsigned char* outputPointer = data;
    size_t blockSize = this->FindBlockSize(firstBlock);

    // Read the first block.
    unsigned char* blockBuffer = this->ReadBlock(firstBlock);
    if(!blockBuffer)
    {
      return 0;
    }
    size_t n = blockSize-beginBlockOffset;
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
size_t vtkXMLDataParser::ReadBinaryData(void* in_buffer,
                                        vtkTypeUInt64 startWord,
                                        size_t numWords,
                                        int wordType)
{
  // Skip real read if aborting.
  if(this->Abort)
  {
    return 0;
  }

  size_t wordSize = this->GetWordTypeSize(wordType);
  void* buffer = in_buffer;

  // Make sure our streams are setup correctly.
  this->DataStream->SetStream(this->Stream);

  // Read the data.
  unsigned char* d = reinterpret_cast<unsigned char*>(buffer);
  size_t actualWords;
  if(this->Compressor)
  {
    if (!this->ReadCompressionHeader())
    {
      vtkErrorMacro("ReadCompressionHeader failed. Aborting read.");
      return 0;
    }
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
size_t vtkXMLDataParser::ReadAsciiData(void* buffer,
                                       vtkTypeUInt64 startWord,
                                       size_t numWords,
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
  vtkTypeUInt64 endWord = startWord + numWords;
  if(this->AsciiDataBufferLength < startWord) { return 0; }
  if(endWord > this->AsciiDataBufferLength)
  {
    endWord = this->AsciiDataBufferLength;
  }
  size_t wordSize = this->GetWordTypeSize(wordType);
  size_t actualWords = endWord - startWord;
  size_t actualBytes = wordSize*actualWords;
  size_t startByte = wordSize*startWord;

  this->UpdateProgress(0.5);

  // Copy the data from the pre-parsed ascii data buffer.
  memcpy(buffer, this->AsciiDataBuffer+startByte, actualBytes);

  this->UpdateProgress(1);

  return this->Abort? 0:actualWords;
}

//----------------------------------------------------------------------------
size_t vtkXMLDataParser::ReadInlineData(vtkXMLDataElement* element,
                                        int isAscii, void* buffer,
                                        vtkTypeUInt64 startWord,
                                        size_t numWords,
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
size_t vtkXMLDataParser::ReadAppendedData(vtkTypeInt64 offset,
                                          void* buffer,
                                          vtkTypeUInt64 startWord,
                                          size_t numWords,
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
static char* vtkXMLParseAsciiData(istream& is, int* length, char*, int)
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
static unsigned char* vtkXMLParseAsciiData(istream& is,
                                           int* length,
                                           unsigned char*,
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
static signed char* vtkXMLParseAsciiData(istream& is,
                                         int* length,
                                         signed char*,
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
  if(this->AsciiDataPosition == this->TellG())
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
