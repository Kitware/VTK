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

vtkCxxRevisionMacro(vtkXMLParser, "1.1");
vtkStandardNewMacro(vtkXMLParser);

//----------------------------------------------------------------------------
vtkXMLParser::vtkXMLParser()
{
  this->Stream = 0;
  this->Parser = 0;
}

//----------------------------------------------------------------------------
vtkXMLParser::~vtkXMLParser()
{
  this->SetStream(0);
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
}

//----------------------------------------------------------------------------
int vtkXMLParser::Parse()
{
  // Make sure we have input.
  if(!this->Stream)
    {
    vtkErrorMacro("Parse() called with no Stream set.");
    return 0;
    }
  
  // Create the expat XML parser.
  this->Parser = XML_ParserCreate(0);
  XML_SetElementHandler(this->Parser,
                        &vtkXMLParser::StartElementFunction,
                        &vtkXMLParser::EndElementFunction);
  XML_SetCharacterDataHandler(this->Parser,
                              &vtkXMLParser::CharacterDataHandlerFunction);
  XML_SetUserData(this->Parser, this);
  
  // Parse the input stream.
  int result = this->ParseStream();
  
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
  
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLParser::ParseStream()
{
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
  return this->ParseBuffer(buffer, strlen(buffer));
}

//----------------------------------------------------------------------------
void vtkXMLParser::StartElementFunction(void* parser, const char *name,
                                        const char **atts)
{
  static_cast<vtkXMLParser*>(parser)->StartElement(name, atts);
}

//----------------------------------------------------------------------------
void vtkXMLParser::EndElementFunction(void* parser, const char *name)
{
  static_cast<vtkXMLParser*>(parser)->EndElement(name);
}

//----------------------------------------------------------------------------
void vtkXMLParser::CharacterDataHandlerFunction(void* parser, const char* data,
                                                int length)
{
  static_cast<vtkXMLParser*>(parser)->CharacterDataHandler(data, length);
}

//----------------------------------------------------------------------------
int vtkXMLParser::IsSpace(char c)
{
  return isspace(c);
}
