/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLDataParser - Used by vtkXMLReader to parse VTK XML files.
// .SECTION Description
// vtkXMLDataParser provides a subclass of vtkXMLParser that
// constructs a representation of an XML data format's file using
// vtkXMLDataElement to represent each XML element.  This
// representation is then used by vtkXMLReader and its subclasses to
// traverse the structure of the file and extract data.

// .SECTION See Also
// vtkXMLDataElement

#ifndef __vtkXMLDataParser_h
#define __vtkXMLDataParser_h

#include "vtkXMLParser.h"
#include "vtkXMLDataElement.h"//For inline definition.

class vtkInputStream;
class vtkDataCompressor;

class VTK_IO_EXPORT vtkXMLDataParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkXMLDataParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLDataParser* New();

  // Description:
  // Get the root element from the XML document.
  vtkXMLDataElement* GetRootElement();

  //BTX
  // Description:
  // Enumerate big and little endian byte order settings.
  enum { BigEndian, LittleEndian };

  // Description:
  // A type used for data sizes and offsets for stream i/o.  Using
  // vtkIdType should satisfy most users.  This could be streamoff if
  // it is deemed portable.  It could also be split into OffsetType
  // (streamoff) and PositionType (streampos).
  typedef vtkIdType OffsetType;

  // Description:
  // Read inline data from inside the given element.  Returns the
  // number of words read.
  OffsetType ReadInlineData(vtkXMLDataElement* element, int isAscii,
                            void* buffer, OffsetType startWord,
                            OffsetType numWords, int wordType);
  OffsetType ReadInlineData(vtkXMLDataElement* element, int isAscii,
                            char* buffer, OffsetType startWord,
                            OffsetType numWords)
    { return this->ReadInlineData(element, isAscii, buffer, startWord,
                                  numWords, VTK_CHAR); }

  // Description:
  // Read from an appended data section starting at the given appended
  // data offset.  Returns the number of words read.
  OffsetType ReadAppendedData(OffsetType offset, void* buffer,
                              OffsetType startWord,
                              OffsetType numWords, int wordType);
  OffsetType ReadAppendedData(OffsetType offset, char* buffer,
                              OffsetType startWord,
                              OffsetType numWords)
    { return this->ReadAppendedData(offset, buffer, startWord, numWords,
                                    VTK_CHAR); }

  // Description:
  // Read from an ascii data section starting at the current position in
  // the stream.  Returns the number of words read.
  OffsetType ReadAsciiData(void* buffer, OffsetType startWord,
                           OffsetType numWords, int wordType);

  // Description:
  // Read from a data section starting at the current position in the
  // stream.  Returns the number of words read.
  OffsetType ReadBinaryData(void* buffer, OffsetType startWord,
                            OffsetType maxWords, int wordType);
  //ETX

  // Description:
  // Get/Set the compressor used to decompress binary and appended data
  // after reading from the file.
  virtual void SetCompressor(vtkDataCompressor*);
  vtkGetObjectMacro(Compressor, vtkDataCompressor);

  // Description:
  // Get the size of a word of the given type.
  unsigned long GetWordTypeSize(int wordType);

  // Description:
  // Parse the XML input and check that the file is safe to read.
  // Returns 1 for okay, 0 for error.
  virtual int Parse();

  // Description:
  // Get/Set flag to abort reading of data.  This may be set by a
  // progress event observer.
  vtkGetMacro(Abort, int);
  vtkSetMacro(Abort, int);

  // Description:
  // Get/Set progress of reading data.  This may be checked by a
  // progress event observer.
  vtkGetMacro(Progress, float);
  vtkSetMacro(Progress, float);

  // Description:
  // Get/Set the character encoding that will be used to set the attributes's
  // encoding type of each vtkXMLDataElement created by this parser (i.e.,
  // the data element attributes will use that encoding internally).
  // If set to VTK_ENCODING_NONE (default), the attribute encoding type will
  // not be changed and will default to the vtkXMLDataElement default encoding
  // type (see vtkXMLDataElement::AttributeEncoding).
  vtkSetClampMacro(AttributesEncoding,int,VTK_ENCODING_NONE,VTK_ENCODING_UNKNOWN);
  vtkGetMacro(AttributesEncoding, int);

  // Description:
  // If you need the text inside XMLElements, turn IgnoreCharacterData off.
  // This method will then be called when the file is parsed, and the text
  // will be stored in each XMLDataElement. VTK XML Readers store the 
  // information elsewhere, so the default is to ignore it.
  virtual void CharacterDataHandler(const char* data, int length);

protected:
  vtkXMLDataParser();
  ~vtkXMLDataParser();

  // This parser does not support parsing from a string.
  virtual int Parse(const char*);
  virtual int Parse(const char*, unsigned int);

  // Implement parsing methods.
  virtual void StartElement(const char* name, const char** atts);
  virtual void EndElement(const char*);

  int ParsingComplete();
  int CheckPrimaryAttributes();
  void FindAppendedDataPosition();
  OffsetType FindInlineDataPosition(OffsetType start);
  int ParseBuffer(const char* buffer, unsigned int count);

  void AddElement(vtkXMLDataElement* element);
  void PushOpenElement(vtkXMLDataElement* element);
  vtkXMLDataElement* PopOpenElement();
  void FreeAllElements();
  void PerformByteSwap(void* data, OffsetType numWords, int wordSize);

  // Data reading methods.
  void ReadCompressionHeader();
  unsigned int FindBlockSize(unsigned int block);
  int ReadBlock(unsigned int block, unsigned char* buffer);
  unsigned char* ReadBlock(unsigned int block);
  OffsetType ReadUncompressedData(unsigned char* data,
                                  OffsetType startWord,
                                  OffsetType numWords,
                                  int wordSize);
  OffsetType ReadCompressedData(unsigned char* data,
                                OffsetType startWord,
                                OffsetType numWords,
                                int wordSize);

  // Go to the start of the inline data
  void SeekInlineDataPosition(vtkXMLDataElement *element);

  // Ascii data reading methods.
  int ParseAsciiData(int wordType);
  void FreeAsciiBuffer();

  // Progress update methods.
  void UpdateProgress(float progress);

  // The root XML element.
  vtkXMLDataElement* RootElement;

  // The stack of elements currently being parsed.
  vtkXMLDataElement** OpenElements;
  unsigned int NumberOfOpenElements;
  unsigned int OpenElementsSize;

  // The position of the appended data section, if found.
  OffsetType AppendedDataPosition;

  // How much of the string "<AppendedData" has been matched in input.
  int AppendedDataMatched;

  // The byte order of the binary input.
  int ByteOrder;

  // The input stream used to read data.  Set by ReadAppendedData and
  // ReadInlineData methods.
  vtkInputStream* DataStream;

  // The input stream used to read inline data.  May transparently
  // decode the data.
  vtkInputStream* InlineDataStream;

  // The stream to use for appended data.
  vtkInputStream* AppendedDataStream;

  //BTX
  // We need a 32 bit unsigned integer type for platform-independent
  // binary headers.  Note that this is duplicated in vtkXMLWriter.h.
#if VTK_SIZEOF_SHORT == 4
  typedef unsigned short HeaderType;
#elif VTK_SIZEOF_INT == 4
  typedef unsigned int HeaderType;
#elif VTK_SIZEOF_LONG == 4
  typedef unsigned long HeaderType;
#else
# error "No native data type can represent an unsigned 32-bit integer."
#endif
  //ETX

  // Decompression data.
  vtkDataCompressor* Compressor;
  unsigned int NumberOfBlocks;
  unsigned int BlockUncompressedSize;
  unsigned int PartialLastBlockUncompressedSize;
  HeaderType* BlockCompressedSizes;
  OffsetType* BlockStartOffsets;

  // Ascii data parsing.
  unsigned char* AsciiDataBuffer;
  OffsetType AsciiDataBufferLength;
  int AsciiDataWordType;
  OffsetType AsciiDataPosition;

  // Progress during reading of data.
  float Progress;

  // Abort flag checked during reading of data.
  int Abort;

  int AttributesEncoding;

private:
  vtkXMLDataParser(const vtkXMLDataParser&);  // Not implemented.
  void operator=(const vtkXMLDataParser&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline
void vtkXMLDataParser::CharacterDataHandler(const char* data, int length )
{
  const unsigned int eid=this->NumberOfOpenElements-1;
  this->OpenElements[eid]->AddCharacterData(data, length);
}


#endif
