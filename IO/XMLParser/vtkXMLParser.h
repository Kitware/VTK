/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLParser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLParser - Parse XML to handle element tags and attributes.
// .SECTION Description
// vtkXMLParser reads a stream and parses XML element tags and corresponding
// attributes.  Each element begin tag and its attributes are sent to
// the StartElement method.  Each element end tag is sent to the
// EndElement method.  Subclasses should replace these methods to actually
// use the tags.
// .SECTION ToDo: Add commands for parsing in Tcl.

#ifndef vtkXMLParser_h
#define vtkXMLParser_h

#include "vtkIOXMLParserModule.h" // For export macro
#include "vtkObject.h"

extern "C"
{
  void vtkXMLParserStartElement(void*, const char*, const char**);
  void vtkXMLParserEndElement(void*, const char*);
  void vtkXMLParserCharacterDataHandler(void*, const char*, int);
}

class VTKIOXMLPARSER_EXPORT vtkXMLParser : public vtkObject
{
public:
  vtkTypeMacro(vtkXMLParser,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkXMLParser* New();

  //BTX
  // Description:
  // Get/Set the input stream.
  vtkSetMacro(Stream, istream*);
  vtkGetMacro(Stream, istream*);

  // Description:
  // Used by subclasses and their supporting classes.  These methods
  // wrap around the tellg and seekg methods of the input stream to
  // work-around stream bugs on various platforms.
  vtkTypeInt64 TellG();
  void SeekG(vtkTypeInt64 position);
  //ETX

  // Description:
  // Parse the XML input.
  virtual int Parse();

  // Description:
  // Parse the XML message. If length is specified, parse only the
  // first "length" characters
  virtual int Parse(const char* inputString);
  virtual int Parse(const char* inputString, unsigned int length);

  // Description:
  // When parsing fragments of XML or streaming XML, use the following
  // three methods.  InitializeParser method initialize parser but
  // does not perform any actual parsing.  ParseChunk parses framgent
  // of XML. This has to match to what was already
  // parsed. CleanupParser finishes parsing. If there were errors,
  // CleanupParser will report them.
  virtual int InitializeParser();
  virtual int ParseChunk(const char* inputString, unsigned int length);
  virtual int CleanupParser();

  // Description:
  // Set and get file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // If this is off (the default), CharacterDataHandler will be called to
  // process text within XML Elements. If this is on, the text will be
  // ignored.
  vtkSetMacro(IgnoreCharacterData, int);
  vtkGetMacro(IgnoreCharacterData, int);

  // Description:
  // Set and get the encoding the parser should expect (NULL defaults to
  // Expat's own default encoder, i.e UTF-8).
  // This should be set before parsing (i.e. a call to Parse()) or
  // even initializing the parser (i.e. a call to InitializeParser())
  vtkSetStringMacro(Encoding);
  vtkGetStringMacro(Encoding);

protected:
  vtkXMLParser();
  ~vtkXMLParser();

  // Input stream.  Set by user.
  istream* Stream;

  // File name to parse
  char* FileName;

  // Encoding
  char* Encoding;

  // This variable is true if there was a parse error while parsing in
  // chunks.
  int ParseError;

  // Character message to parse
  const char* InputString;
  int InputStringLength;

  // Expat parser structure.  Exists only during call to Parse().
  void* Parser;

  // Create/Allocate the internal parser (can be overriden by subclasses).
  virtual int CreateParser();

  // Called by Parse() to read the stream and call ParseBuffer.  Can
  // be replaced by subclasses to change how input is read.
  virtual int ParseXML();

  // Called before each block of input is read from the stream to
  // check if parsing is complete.  Can be replaced by subclasses to
  // change the terminating condition for parsing.  Parsing always
  // stops when the end of file is reached in the stream.
  virtual int ParsingComplete();

  // Called when a new element is opened in the XML source.  Should be
  // replaced by subclasses to handle each element.
  //  name = Name of new element.
  //  atts = Null-terminated array of attribute name/value pairs.
  //         Even indices are attribute names, and odd indices are values.
  virtual void StartElement(const char* name, const char** atts);

  // Called at the end of an element in the XML source opened when
  // StartElement was called.
  virtual void EndElement(const char* name);

  // Called when there is character data to handle.
  virtual void CharacterDataHandler(const char* data, int length);

  // Called by begin handlers to report any stray attribute values.
  virtual void ReportStrayAttribute(const char* element, const char* attr,
                                    const char* value);

  // Called by begin handlers to report any missing attribute values.
  virtual void ReportMissingAttribute(const char* element, const char* attr);

  // Called by begin handlers to report bad attribute values.
  virtual void ReportBadAttribute(const char* element, const char* attr,
                                  const char* value);

  // Called by StartElement to report unknown element type.
  virtual void ReportUnknownElement(const char* element);

  // Called by Parse to report an XML syntax error.
  virtual void ReportXmlParseError();

  // Get the current byte index from the beginning of the XML stream.
  vtkTypeInt64 GetXMLByteIndex();

  // Send the given buffer to the XML parser.
  virtual int ParseBuffer(const char* buffer, unsigned int count);

  // Send the given c-style string to the XML parser.
  int ParseBuffer(const char* buffer);

  // Utility for convenience of subclasses.  Wraps isspace C library
  // routine.
  static int IsSpace(char c);

  //BTX
  friend void vtkXMLParserStartElement(void*, const char*, const char**);
  friend void vtkXMLParserEndElement(void*, const char*);
  friend void vtkXMLParserCharacterDataHandler(void*, const char*, int);
  //ETX

  int IgnoreCharacterData;

private:
  vtkXMLParser(const vtkXMLParser&);  // Not implemented.
  void operator=(const vtkXMLParser&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline
void vtkXMLParserCharacterDataHandler(
        void* parser,
        const char* data,
        int length)
{
  // Character data handler that is registered with the XML_Parser.
  // This just casts the user data to a vtkXMLParser and calls
  // CharacterDataHandler.
  static_cast<vtkXMLParser*>(parser)->CharacterDataHandler(data, length);
}

#endif
