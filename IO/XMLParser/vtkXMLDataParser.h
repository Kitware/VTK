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
/**
 * @class   vtkXMLDataParser
 * @brief   Used by vtkXMLReader to parse VTK XML files.
 *
 * vtkXMLDataParser provides a subclass of vtkXMLParser that
 * constructs a representation of an XML data format's file using
 * vtkXMLDataElement to represent each XML element.  This
 * representation is then used by vtkXMLReader and its subclasses to
 * traverse the structure of the file and extract data.
 *
 * @sa
 * vtkXMLDataElement
*/

#ifndef vtkXMLDataParser_h
#define vtkXMLDataParser_h

#include "vtkIOXMLParserModule.h" // For export macro
#include "vtkXMLParser.h"
#include "vtkXMLDataElement.h"//For inline definition.

class vtkInputStream;
class vtkDataCompressor;

class VTKIOXMLPARSER_EXPORT vtkXMLDataParser : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkXMLDataParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLDataParser* New();

  /**
   * Get the root element from the XML document.
   */
  vtkXMLDataElement* GetRootElement();

  /**
   * Enumerate big and little endian byte order settings.
   */
  enum { BigEndian, LittleEndian };

  /**
   * Read inline data from inside the given element.  Returns the
   * number of words read.
   */
  size_t ReadInlineData(vtkXMLDataElement* element, int isAscii,
                        void* buffer, vtkTypeUInt64 startWord,
                        size_t numWords, int wordType);
  size_t ReadInlineData(vtkXMLDataElement* element, int isAscii,
                        char* buffer, vtkTypeUInt64 startWord,
                        size_t numWords)
    { return this->ReadInlineData(element, isAscii, buffer, startWord,
                                  numWords, VTK_CHAR); }

  /**
   * Read from an appended data section starting at the given appended
   * data offset.  Returns the number of words read.
   */
  size_t ReadAppendedData(vtkTypeInt64 offset, void* buffer,
                          vtkTypeUInt64 startWord,
                          size_t numWords, int wordType);
  size_t ReadAppendedData(vtkTypeInt64 offset, char* buffer,
                          vtkTypeUInt64 startWord,
                          size_t numWords)
  { return this->ReadAppendedData(offset, buffer, startWord, numWords,
                                    VTK_CHAR); }

  /**
   * Read from an ascii data section starting at the current position in
   * the stream.  Returns the number of words read.
   */
  size_t ReadAsciiData(void* buffer, vtkTypeUInt64 startWord,
                       size_t numWords, int wordType);

  /**
   * Read from a data section starting at the current position in the
   * stream.  Returns the number of words read.
   */
  size_t ReadBinaryData(void* buffer, vtkTypeUInt64 startWord,
                        size_t maxWords, int wordType);

  //@{
  /**
   * Get/Set the compressor used to decompress binary and appended data
   * after reading from the file.
   */
  virtual void SetCompressor(vtkDataCompressor*);
  vtkGetObjectMacro(Compressor, vtkDataCompressor);
  //@}

  /**
   * Get the size of a word of the given type.
   */
  size_t GetWordTypeSize(int wordType);

  /**
   * Parse the XML input and check that the file is safe to read.
   * Returns 1 for okay, 0 for error.
   */
  int Parse() override;

  //@{
  /**
   * Get/Set flag to abort reading of data.  This may be set by a
   * progress event observer.
   */
  vtkGetMacro(Abort, int);
  vtkSetMacro(Abort, int);
  //@}

  //@{
  /**
   * Get/Set progress of reading data.  This may be checked by a
   * progress event observer.
   */
  vtkGetMacro(Progress, float);
  vtkSetMacro(Progress, float);
  //@}

  //@{
  /**
   * Get/Set the character encoding that will be used to set the attributes's
   * encoding type of each vtkXMLDataElement created by this parser (i.e.,
   * the data element attributes will use that encoding internally).
   * If set to VTK_ENCODING_NONE (default), the attribute encoding type will
   * not be changed and will default to the vtkXMLDataElement default encoding
   * type (see vtkXMLDataElement::AttributeEncoding).
   */
  vtkSetClampMacro(AttributesEncoding,int,VTK_ENCODING_NONE,VTK_ENCODING_UNKNOWN);
  vtkGetMacro(AttributesEncoding, int);
  //@}

  /**
   * If you need the text inside XMLElements, turn IgnoreCharacterData off.
   * This method will then be called when the file is parsed, and the text
   * will be stored in each XMLDataElement. VTK XML Readers store the
   * information elsewhere, so the default is to ignore it.
   */
  void CharacterDataHandler(const char* data, int length) override;

  /**
   * Returns the byte index of where appended data starts (if the
   * file is using appended data). Valid after the XML is parsed.
   */
  vtkTypeInt64 GetAppendedDataPosition()
  {
    return this->AppendedDataPosition;
  }

protected:
  vtkXMLDataParser();
  ~vtkXMLDataParser() override;

  // This parser does not support parsing from a string.
  int Parse(const char*) override;
  int Parse(const char*, unsigned int) override;

  // Implement parsing methods.
  void StartElement(const char* name, const char** atts) override;
  void EndElement(const char*) override;

  int ParsingComplete() override;
  int CheckPrimaryAttributes();
  void FindAppendedDataPosition();
  int ParseBuffer(const char* buffer, unsigned int count) override;

  void AddElement(vtkXMLDataElement* element);
  void PushOpenElement(vtkXMLDataElement* element);
  vtkXMLDataElement* PopOpenElement();
  void FreeAllElements();
  void PerformByteSwap(void* data, size_t numWords, size_t wordSize);

  // Data reading methods.
  int ReadCompressionHeader();
  size_t FindBlockSize(vtkTypeUInt64 block);
  int ReadBlock(vtkTypeUInt64 block, unsigned char* buffer);
  unsigned char* ReadBlock(vtkTypeUInt64 block);
  size_t ReadUncompressedData(unsigned char* data,
                              vtkTypeUInt64 startWord,
                              size_t numWords,
                              size_t wordSize);
  size_t ReadCompressedData(unsigned char* data,
                            vtkTypeUInt64 startWord,
                            size_t numWords,
                            size_t wordSize);

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
  vtkTypeInt64 AppendedDataPosition;

  // How much of the string "<AppendedData" has been matched in input.
  int AppendedDataMatched;

  // The byte order of the binary input.
  int ByteOrder;

  // The word type of binary input headers.
  int HeaderType;

  // The input stream used to read data.  Set by ReadAppendedData and
  // ReadInlineData methods.
  vtkInputStream* DataStream;

  // The input stream used to read inline data.  May transparently
  // decode the data.
  vtkInputStream* InlineDataStream;

  // The stream to use for appended data.
  vtkInputStream* AppendedDataStream;

  // Decompression data.
  vtkDataCompressor* Compressor;
  size_t NumberOfBlocks;
  size_t BlockUncompressedSize;
  size_t PartialLastBlockUncompressedSize;
  size_t* BlockCompressedSizes;
  vtkTypeInt64* BlockStartOffsets;

  // Ascii data parsing.
  unsigned char* AsciiDataBuffer;
  size_t AsciiDataBufferLength;
  int AsciiDataWordType;
  vtkTypeInt64 AsciiDataPosition;

  // Progress during reading of data.
  float Progress;

  // Abort flag checked during reading of data.
  int Abort;

  int AttributesEncoding;

private:
  vtkXMLDataParser(const vtkXMLDataParser&) = delete;
  void operator=(const vtkXMLDataParser&) = delete;
};

//----------------------------------------------------------------------------
inline
void vtkXMLDataParser::CharacterDataHandler(const char* data, int length )
{
  const unsigned int eid=this->NumberOfOpenElements-1;
  this->OpenElements[eid]->AddCharacterData(data, length);
}


#endif
