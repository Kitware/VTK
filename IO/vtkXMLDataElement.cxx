/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLDataElement.cxx
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
#include "vtkXMLDataElement.h"
#include "vtkObjectFactory.h"
#include "vtkXMLDataParser.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkXMLDataElement, "1.1");
vtkStandardNewMacro(vtkXMLDataElement);

//----------------------------------------------------------------------------
vtkXMLDataElement::vtkXMLDataElement()
{
  this->Name = 0;
  this->Id = 0;
  this->Parent = 0;
  
  this->NumberOfAttributes = 0;
  this->AttributesSize = 5;
  this->AttributeNames = new char*[this->AttributesSize];
  this->AttributeValues = new char*[this->AttributesSize];
  
  this->NumberOfNestedElements = 0;
  this->NestedElementsSize = 10;
  this->NestedElements = new vtkXMLDataElement*[this->NestedElementsSize];
  
  this->InlineDataPosition = 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement::~vtkXMLDataElement()
{
  this->SetName(0);
  this->SetId(0);
  int i;
  for(i=0;i < this->NumberOfAttributes;++i)
    {
    delete [] this->AttributeNames[i];
    delete [] this->AttributeValues[i];
    }
  delete [] this->AttributeNames;
  delete [] this->AttributeValues;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    this->NestedElements[i]->UnRegister(this);
    }
  delete [] this->NestedElements;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLByteIndex: " << this->XMLByteIndex << "\n";
  os << indent << "Name: " << (this->Name? this->Name : "(none)") << "\n";
  os << indent << "Id: " << (this->Id? this->Id : "(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::ReadXMLAttributes(const char** atts)
{
  if(this->NumberOfAttributes > 0)
    {
    int i;
    for(i=0;i < this->NumberOfAttributes;++i)
      {
      delete [] this->AttributeNames[i];
      delete [] this->AttributeValues[i];
      }
    this->NumberOfAttributes = 0;
    }
  if(atts)
    {
    const char** attsIter = atts;
    int count=0;
    while(*attsIter++) { ++count; }
    this->NumberOfAttributes = count/2;
    this->AttributesSize = this->NumberOfAttributes;
    
    delete [] this->AttributeNames;
    delete [] this->AttributeValues;
    this->AttributeNames = new char* [this->AttributesSize];
    this->AttributeValues = new char* [this->AttributesSize];
    
    int i;
    for(i=0;i < this->NumberOfAttributes; ++i)
      {
      this->AttributeNames[i] = new char[strlen(atts[i*2])+1];
      strcpy(this->AttributeNames[i], atts[i*2]);
      
      this->AttributeValues[i] = new char[strlen(atts[i*2+1])+1];
      strcpy(this->AttributeValues[i], atts[i*2+1]);
      }
    }  
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::AddNestedElement(vtkXMLDataElement* element)
{
  if(this->NumberOfNestedElements == this->NestedElementsSize)
    {
    int i;
    int newSize = this->NestedElementsSize*2;
    vtkXMLDataElement** newNestedElements = new vtkXMLDataElement*[newSize];
    for(i=0;i < this->NumberOfNestedElements;++i)
      {
      newNestedElements[i] = this->NestedElements[i];
      }
    delete [] this->NestedElements;
    this->NestedElements = newNestedElements;
    this->NestedElementsSize = newSize;
    }
  
  int index = this->NumberOfNestedElements++;
  this->NestedElements[index] = element;
  element->Register(this);
  element->SetParent(this);
}

//----------------------------------------------------------------------------
const char* vtkXMLDataElement::GetAttribute(const char* name)
{
  int i;
  for(i=0; i < this->NumberOfAttributes;++i)
    {
    if(strcmp(this->AttributeNames[i], name) == 0)
      {
      return this->AttributeValues[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::PrintXML(ostream& os, vtkIndent indent)
{
  os << indent << "<" << this->Name;
  int i;
  for(i=0;i < this->NumberOfAttributes;++i)
    {
    os << " " << this->AttributeNames[i]
       << "=\"" << this->AttributeValues[i] << "\"";
    }
  if(this->NumberOfNestedElements > 0)
    {
    os << ">\n";
    for(i=0;i < this->NumberOfNestedElements;++i)
      {
      vtkIndent nextIndent = indent.GetNextIndent();
      this->NestedElements[i]->PrintXML(os, nextIndent);
      }
    os << indent << "</" << this->Name << ">\n";
    }
  else
    {
    os << "/>\n";
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetParent(vtkXMLDataElement* parent)
{
  this->Parent = parent;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::GetParent()
{
  return this->Parent;
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetNumberOfNestedElements()
{
  return this->NumberOfNestedElements;
}
  
//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::GetNestedElement(int index)
{
  if(index < this->NumberOfNestedElements)
    {
    return this->NestedElements[index];
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::LookupElement(const char* id)
{
  return this->LookupElementUpScope(id);
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::FindNestedElement(const char* id)
{
  int i;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    const char* nid = this->NestedElements[i]->GetId();
    if(nid && (strcmp(nid, id) == 0))
      {
      return this->NestedElements[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::LookupElementInScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while(*end && (*end != '.')) ++end;
  int len = end - id;
  char* name = new char[len+1];
  strncpy(name, id, len);
  name[len] = '\0';
  
  // Find the qualifier in this scope.
  vtkXMLDataElement* next = this->FindNestedElement(name);  
  if(next && (*end == '.'))
    {
    // Lookup rest of qualifiers in nested scope.
    next = next->LookupElementInScope(end+1);
    }
  
  delete [] name;
  return next;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::LookupElementUpScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while(*end && (*end != '.')) ++end;
  int len = end - id;
  char* name = new char[len+1];
  strncpy(name, id, len);
  name[len] = '\0';
  
  // Find most closely nested occurrence of first qualifier.
  vtkXMLDataElement* curScope = this;
  vtkXMLDataElement* start = 0;
  while(curScope && !start)
    {
    start = curScope->FindNestedElement(name);
    curScope = curScope->GetParent();
    }
  if(start && (*end == '.'))
    {
    start = start->LookupElementInScope(end+1);
    }
  
  delete [] name;
  return start;
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetScalarAttribute(const char* name, int& value)
{
  return this->GetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetScalarAttribute(const char* name, float& value)
{
  return this->GetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetScalarAttribute(const char* name,
                                          unsigned long& value)
{
  return this->GetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
int vtkXMLDataElement::GetScalarAttribute(const char* name, vtkIdType& value)
{
  return this->GetVectorAttribute(name, 1, &value);
}
#endif

//----------------------------------------------------------------------------
template <class T>
static int vtkXMLVectorAttributeParse(const char* str, int length, T* data)
{
  if(!str || !length) { return 0; }
  strstream vstr;
  vstr << str << ends;  
  int i;
  for(i=0;i < length;++i)
    {
    vstr >> data[i];
    if(!vstr) { return i; }
    }
  return length;
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          int* data)
{
  return vtkXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          float* data)
{
  return vtkXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          unsigned long* data)
{
  return vtkXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
#ifdef VTK_ID_TYPE_IS_NOT_BASIC_TYPE
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          vtkIdType* data)
{
  return vtkXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}
#endif

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetWordTypeAttribute(const char* name, int& value)
{
  // These string values must match vtkXMLWriter::GetWordTypeName().
  const char* v = this->GetAttribute(name);
  if(!v)
    {
    return 0;
    }
  else if(strcmp(v, "vtkIdType") == 0)
    {
    value = VTK_ID_TYPE;
    return 1;
    }
  else if(strcmp(v, "float") == 0)
    {
    value = VTK_FLOAT;
    return 1;
    }
  else if(strcmp(v, "double") == 0)
    {
    value = VTK_DOUBLE;
    return 1;
    }
  else if(strcmp(v, "int") == 0)
    {
    value = VTK_INT;
    return 1;
    }
  else if(strcmp(v, "unsigned int") == 0)
    {
    value = VTK_UNSIGNED_INT;
    return 1;
    }
  else if(strcmp(v, "long") == 0)
    {
    value = VTK_LONG;
    return 1;
    }
  else if(strcmp(v, "unsigned long") == 0)
    {
    value = VTK_UNSIGNED_LONG;
    return 1;
    }
  else if(strcmp(v, "short") == 0)
    {
    value = VTK_SHORT;
    return 1;
    }
  else if(strcmp(v, "unsigned short") == 0)
    {
    value = VTK_UNSIGNED_SHORT;
    return 1;
    }
  else if(strcmp(v, "unsigned char") == 0)
    {
    value = VTK_UNSIGNED_CHAR;
    return 1;
    }
  else if(strcmp(v, "char") == 0)
    {
    value = VTK_CHAR;
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SeekInlineDataPosition(vtkXMLDataParser* parser)
{
  istream* stream = parser->GetStream();
  if(!this->InlineDataPosition)
    {
    // Scan for the start of the actual inline data.
    char c;
    stream->seekg(this->GetXMLByteIndex());
    stream->clear(stream->rdstate() & ~ios::eofbit);
    stream->clear(stream->rdstate() & ~ios::failbit);
    while(stream->get(c) && (c != '>'));
    while(stream->get(c) && this->IsSpace(c));
    unsigned long pos = stream->tellg();
    this->InlineDataPosition = pos-1;
    }
  
  // Seek to the data position.
  stream->seekg(this->InlineDataPosition);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::IsSpace(char c)
{
  return isspace(c);
}
