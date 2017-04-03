/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtk_expat.h"

#include <vtksys/SystemTools.hxx>

#include <cctype>

vtkStandardNewMacro(vtkXMLParser);

//----------------------------------------------------------------------------
vtkXMLParser::vtkXMLParser()
{
  this->Stream            = 0;
  this->Parser            = 0;
  this->FileName          = 0;
  this->Encoding          = 0;
  this->InputString       = 0;
  this->InputStringLength = 0;
  this->ParseError        = 0;
  this->IgnoreCharacterData = 0;
}

//----------------------------------------------------------------------------
vtkXMLParser::~vtkXMLParser()
{
  this->SetStream(0);
  this->SetFileName(0);
  this->SetEncoding(0);
}

//----------------------------------------------------------------------------
void vtkXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->Stream)
  {
    os << indent << "Stream: " << this->Stream << "\n";
  }
  else
  {
    os << indent << "Stream: (none)\n";
  }
  os << indent << "FileName: " << (this->FileName? this->FileName : "(none)")
     << "\n";
  os << indent << "IgnoreCharacterData: "
     << (this->IgnoreCharacterData?"On":"Off")
     << endl;
  os << indent << "Encoding: " << (this->Encoding? this->Encoding : "(none)")
     << "\n";
}

//----------------------------------------------------------------------------
static int vtkXMLParserFail(istream* stream)
{
  // The fail() method returns true if either the failbit or badbit is set.
#if defined(__HP_aCC)
  // The HP compiler sets the badbit too often, so we ignore it.
  return (stream->rdstate() & ios::failbit)? 1:0;
#else
  return stream->fail()? 1:0;
#endif
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkXMLParser::TellG()
{
  // Standard tellg returns -1 if fail() is true.
  if(!this->Stream || vtkXMLParserFail(this->Stream))
  {
    return -1;
  }
  return this->Stream->tellg();
}

//----------------------------------------------------------------------------
void vtkXMLParser::SeekG(vtkTypeInt64 position)
{
  // Standard seekg does nothing if fail() is true.
  if(!this->Stream || vtkXMLParserFail(this->Stream))
  {
    return;
  }
  this->Stream->seekg(std::streampos(position));
}

//----------------------------------------------------------------------------
int vtkXMLParser::Parse(const char* inputString)
{
  this->InputString = inputString;
  this->InputStringLength = -1;
  int result = this->Parse();
  this->InputString = 0;
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::Parse(const char* inputString, unsigned int length)
{
  this->InputString = inputString;
  this->InputStringLength = length;
  int result = this->Parse();
  this->InputString = 0;
  this->InputStringLength = -1;
  return result;
}


//----------------------------------------------------------------------------
int vtkXMLParser::Parse()
{
  // Select source of XML
  ifstream ifs;
  if ( !this->InputString && !this->Stream && this->FileName )
  {
    // If it is file, open it and set the appropriate stream
    vtksys::SystemTools::Stat_t fs;
    if (vtksys::SystemTools::Stat(this->FileName, &fs) != 0)
    {
      vtkErrorMacro("Cannot open XML file: " << this->FileName);
      return 0;
    }
#ifdef _WIN32
    ifs.open(this->FileName, ios::binary | ios::in);
#else
    ifs.open(this->FileName, ios::in);
#endif
    if ( !ifs )
    {
      vtkErrorMacro("Cannot open XML file: " << this->FileName);
      return 0;
    }
    this->Stream = &ifs;
  }

  // Create the expat XML parser.
  this->CreateParser();

  XML_SetElementHandler(static_cast<XML_Parser>(this->Parser),
                        &vtkXMLParserStartElement,
                        &vtkXMLParserEndElement);
  if (!this->IgnoreCharacterData)
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser),
                                &vtkXMLParserCharacterDataHandler);
  }
  else
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser), NULL);
  }
  XML_SetUserData(static_cast<XML_Parser>(this->Parser), this);

  // Parse the input.
  int result = this->ParseXML();

  if(result)
  {
    // Tell the expat XML parser about the end-of-input.
    if(!XML_Parse(static_cast<XML_Parser>(this->Parser), "", 0, 1))
    {
      this->ReportXmlParseError();
      result = 0;
    }
  }

  // Clean up the parser.
  XML_ParserFree(static_cast<XML_Parser>(this->Parser));
  this->Parser = 0;

  // If the source was a file, reset the stream
  if ( this->Stream == &ifs )
  {
    this->Stream = 0;
  }

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::CreateParser()
{
  if (this->Parser)
  {
    vtkErrorMacro("Parser already created");
    return 0;
  }
  // Create the expat XML parser.
  this->Parser = XML_ParserCreate(this->Encoding);
  return this->Parser ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkXMLParser::InitializeParser()
{
  // Create the expat XML parser.
  if (!this->CreateParser())
  {
    vtkErrorMacro("Parser already initialized");
    this->ParseError = 1;
    return 0;
  }

  XML_SetElementHandler(static_cast<XML_Parser>(this->Parser),
                        &vtkXMLParserStartElement,
                        &vtkXMLParserEndElement);
  if (!this->IgnoreCharacterData)
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser),
                                &vtkXMLParserCharacterDataHandler);
  }
  else
  {
    XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser), NULL);
  }
  XML_SetUserData(static_cast<XML_Parser>(this->Parser), this);
  this->ParseError = 0;
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseChunk(const char* inputString, unsigned int length)
{
  if ( !this->Parser )
  {
    vtkErrorMacro("Parser not initialized");
    this->ParseError = 1;
    return 0;
  }
  int res;
  res = this->ParseBuffer(inputString, length);
  if ( res == 0 )
  {
    this->ParseError = 1;
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkXMLParser::CleanupParser()
{
  if ( !this->Parser )
  {
    vtkErrorMacro("Parser not initialized");
    this->ParseError = 1;
    return 0;
  }
  int result = !this->ParseError;
  if(result)
  {
    // Tell the expat XML parser about the end-of-input.
    if(!XML_Parse(static_cast<XML_Parser>(this->Parser), "", 0, 1))
    {
      this->ReportXmlParseError();
      result = 0;
    }
  }

  // Clean up the parser.
  XML_ParserFree(static_cast<XML_Parser>(this->Parser));
  this->Parser = 0;

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseXML()
{
  // Parsing of message
  if ( this->InputString )
  {
    if ( this->InputStringLength >= 0 )
    {
      return this->ParseBuffer(this->InputString, this->InputStringLength);
    }
    else
    {
      return this->ParseBuffer(this->InputString);
    }
  }

  // Make sure we have input.
  if(!this->Stream)
  {
    vtkErrorMacro("Parse() called with no Stream set.");
    return 0;
  }

  // Default stream parser just reads a block at a time.
  istream& in = *(this->Stream);
  const int bufferSize = 4096;
  char buffer[bufferSize];

  // Read in the stream and send its contents to the XML parser.  This
  // read loop is very sensitive on certain platforms with slightly
  // broken stream libraries (like HPUX).  Normally, it is incorrect
  // to not check the error condition on the fin.read() before using
  // the data, but the fin.gcount() will be zero if an error occurred.
  // Therefore, the loop should be safe everywhere.
  while(!this->ParseError && !this->ParsingComplete() && in)
  {
    in.read(buffer, bufferSize);
    if(in.gcount())
    {
      if(!this->ParseBuffer(buffer, in.gcount()))
      {
        return 0;
      }
    }
  }

  // Clear the fail and eof bits on the input stream so we can later
  // seek back to read data.
  this->Stream->clear(this->Stream->rdstate() & ~ios::eofbit);
  this->Stream->clear(this->Stream->rdstate() & ~ios::failbit);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParsingComplete()
{
  // Default behavior is to parse to end of stream.
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLParser::StartElement(const char *name,
                                const char ** vtkNotUsed(atts))
{
  this->ReportUnknownElement(name);
}

//----------------------------------------------------------------------------
void vtkXMLParser::EndElement(const char * vtkNotUsed(name))
{
}

//----------------------------------------------------------------------------
void vtkXMLParser::CharacterDataHandler(const char* vtkNotUsed(inData),
                                        int vtkNotUsed(inLength))
{
}

//----------------------------------------------------------------------------
void vtkXMLParser::ReportStrayAttribute(const char* element, const char* attr,
                                        const char* value)
{
  vtkWarningMacro("Stray attribute in XML stream: Element " << element
                  << " has " << attr << "=\"" << value << "\"");
}

//----------------------------------------------------------------------------
void vtkXMLParser::ReportMissingAttribute(const char* element,
                                          const char* attr)
{
  vtkErrorMacro("Missing attribute in XML stream: Element " << element
                << " is missing " << attr);
}

//----------------------------------------------------------------------------
void vtkXMLParser::ReportBadAttribute(const char* element, const char* attr,
                                      const char* value)
{
  vtkErrorMacro("Bad attribute value in XML stream: Element " << element
                << " has " << attr << "=\"" << value << "\"");
}

//----------------------------------------------------------------------------
void vtkXMLParser::ReportUnknownElement(const char* element)
{
  vtkErrorMacro("Unknown element in XML stream: " << element);
}

//----------------------------------------------------------------------------
void vtkXMLParser::ReportXmlParseError()
{
  vtkErrorMacro(
    "Error parsing XML in stream at line "
    << XML_GetCurrentLineNumber(static_cast<XML_Parser>(this->Parser))
    << ", column "
    << XML_GetCurrentColumnNumber(static_cast<XML_Parser>(this->Parser))
    << ", byte index "
    << XML_GetCurrentByteIndex(static_cast<XML_Parser>(this->Parser))
    << ": "
    << XML_ErrorString(XML_GetErrorCode(static_cast<XML_Parser>(this->Parser))));
}

//----------------------------------------------------------------------------
vtkTypeInt64 vtkXMLParser::GetXMLByteIndex()
{
  return XML_GetCurrentByteIndex(static_cast<XML_Parser>(this->Parser));
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseBuffer(const char* buffer, unsigned int count)
{
  // Pass the buffer to the expat XML parser.
  if(!XML_Parse(static_cast<XML_Parser>(this->Parser), buffer, count, 0))
  {
    this->ReportXmlParseError();
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseBuffer(const char* buffer)
{
  return this->ParseBuffer(buffer, static_cast<int>(strlen(buffer)));
}

//----------------------------------------------------------------------------
int vtkXMLParser::IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
void vtkXMLParserStartElement(void* parser, const char *name,
                              const char **atts)
{
  // Begin element handler that is registered with the XML_Parser.
  // This just casts the user data to a vtkXMLParser and calls
  // StartElement.
  static_cast<vtkXMLParser*>(parser)->StartElement(name, atts);
}

//----------------------------------------------------------------------------
void vtkXMLParserEndElement(void* parser, const char *name)
{
  // End element handler that is registered with the XML_Parser.  This
  // just casts the user data to a vtkXMLParser and calls EndElement.
  static_cast<vtkXMLParser*>(parser)->EndElement(name);
}
