/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLParser.cxx
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
#include "vtkXMLParser.h"
#include "vtkObjectFactory.h"

#include "expat.h"
#include <ctype.h>

#ifdef VTK_USE_ANSI_STDLIB
#define VTK_IOS_NOCREATE 
#else
#define VTK_IOS_NOCREATE | ios::nocreate
#endif

vtkCxxRevisionMacro(vtkXMLParser, "1.6");
vtkStandardNewMacro(vtkXMLParser);

//----------------------------------------------------------------------------
vtkXMLParser::vtkXMLParser()
{
  this->Stream = 0;
  this->Parser = 0;
  this->LegacyHack  = 0;
  this->FileName    = 0;
  this->InputString = 0;
}

//----------------------------------------------------------------------------
vtkXMLParser::~vtkXMLParser()
{
  this->SetStream(0);
  this->SetFileName(0);
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
}

//----------------------------------------------------------------------------
int vtkXMLParser::Parse(const char* inputString)
{
  this->InputString = inputString;
  int result = this->vtkXMLParser::Parse();
  this->InputString = 0;
  return result;
}


//----------------------------------------------------------------------------
int vtkXMLParser::Parse()
{
  ifstream ifs;
  if ( !this->InputString && !this->Stream && this->FileName )
    {
    ifs.open(this->FileName, ios::in VTK_IOS_NOCREATE);
    if ( !ifs )
      {
      return 0;
      }
    this->Stream = &ifs;
    }

  // Create the expat XML parser.
  this->Parser = XML_ParserCreate(0);
  XML_SetElementHandler(this->Parser,
                        &vtkXMLParserStartElement,
                        &vtkXMLParserEndElement);
  XML_SetCharacterDataHandler(this->Parser,
                              &vtkXMLParserCharacterDataHandler);
  XML_SetUserData(this->Parser, this);
  
  // Parse the input.
  int result = this->ParseXML();
  
  if(result)
    {
    // Tell the expat XML parser about the end-of-input.
    if(!XML_Parse(this->Parser, "", 0, 1))
      {
      this->ReportXmlParseError();
      result = 0;
      }
    }
  
  // Clean up the parser.
  XML_ParserFree(this->Parser);
  this->Parser = 0;
  
  if ( this->Stream == &ifs )
    {
    this->Stream = 0;
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseXML()
{
  // Parsing of message
  if ( this->InputString )
    {
    return this->ParseBuffer(this->InputString);
    }

  // Make sure we have input.
  if(!this->Stream)
    {
    vtkErrorMacro("Parse() called with no Stream set.");
    return 0;
    }
  
  this->LegacyHack = 1;
  int result = this->ParseStream();
  if(this->LegacyHack)
    {
    vtkWarningMacro("The ParseStream() method has been deprectated and "
                    "will soon be removed.  Use ParseXML() instead.");
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseStream()
{
  this->LegacyHack = 0;
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
  while(!this->ParsingComplete() && in)
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
  vtkErrorMacro("Error parsing XML in stream at line "
                << XML_GetCurrentLineNumber(this->Parser)
                << ": " << XML_ErrorString(XML_GetErrorCode(this->Parser)));
}

//----------------------------------------------------------------------------
unsigned long vtkXMLParser::GetXMLByteIndex()
{
  return XML_GetCurrentByteIndex(this->Parser);
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseBuffer(const char* buffer, unsigned int count)
{
  // Pass the buffer to the expat XML parser.
  if(!XML_Parse(this->Parser, buffer, count, 0))
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

//----------------------------------------------------------------------------
void vtkXMLParserCharacterDataHandler(void* parser, const char* data,
                                      int length)
{
  // Character data handler that is registered with the XML_Parser.
  // This just casts the user data to a vtkXMLParser and calls
  // CharacterDataHandler.
  static_cast<vtkXMLParser*>(parser)->CharacterDataHandler(data, length);
}
