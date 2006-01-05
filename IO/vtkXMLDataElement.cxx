/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataElement.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataElement.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLUtilities.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkXMLDataElement, "1.26");
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
  this->XMLByteIndex = 0;

  this->AttributeEncoding = VTK_ENCODING_UTF_8;
  this->CharacterData = 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement::~vtkXMLDataElement()
{
  this->SetName(0);
  this->SetId(0);

  this->RemoveAllAttributes();
  delete [] this->AttributeNames;
  delete [] this->AttributeValues;

  this->RemoveAllNestedElements();
  delete [] this->NestedElements;
  this->SetCharacterData(0, 0);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::RemoveAllAttributes()
{
  for(int i = 0; i < this->NumberOfAttributes; ++i)
    {
    delete [] this->AttributeNames[i];
    delete [] this->AttributeValues[i];
    }
  this->NumberOfAttributes = 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::RemoveAllNestedElements()
{
  for(int i = 0; i < this->NumberOfNestedElements; ++i)
    {
    this->NestedElements[i]->UnRegister(this);
    }
  this->NumberOfNestedElements = 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::ReadXMLAttributes(const char** atts, int encoding)
{
  if(atts)
    {
    // If the target encoding is VTK_ENCODING_NONE or VTK_ENCODING_UNKNOWN, 
    // then keep the internal/default encoding, otherwise encode each
    // attribute using that new format

    if (encoding != VTK_ENCODING_NONE && encoding != VTK_ENCODING_UNKNOWN)
      {
      this->SetAttributeEncoding(encoding);
      }

    // Process each attributes returned by Expat in UTF-8 encoding, and
    // convert them to our encoding

    for (int i = 0; atts[i] && atts[i + 1]; i += 2)
      {
      if (this->GetAttributeEncoding() == VTK_ENCODING_UTF_8)
        {
        this->SetAttribute(atts[i], atts[i + 1]);
        }
      else
        {
        ostrstream str;
        vtkXMLUtilities::EncodeString(
          atts[i+1], VTK_ENCODING_UTF_8, str, this->GetAttributeEncoding(), 0);
        str << ends;
        this->SetAttribute(atts[i], str.str());
        str.rdbuf()->freeze(0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetCharacterData(const char* c, int length)
{
  if (this->CharacterData)
    {
    delete [] this->CharacterData;
    this->CharacterData = 0;
    }
  if (c && length > 0)
    {
    this->CharacterData = new char[length + 1];
    strncpy(this->CharacterData, c, length);
    this->CharacterData[length] = 0;
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::AddCharacterData(const char* c, int length)
{
  if (!c || length <= 0)
    {
    return;
    }
  
  char* old_data = this->CharacterData;
  int old_length = (old_data)? strlen(old_data): 0;
  int total_length = old_length + length;
  
  this->CharacterData = new char[total_length + 1];
  this->CharacterData[0] = 0;

  if (old_length > 0)
    {
    strncpy(this->CharacterData, old_data, old_length);
    this->CharacterData[old_length] = 0;
    }
  strncat(this->CharacterData, c, length);
  this->CharacterData[total_length] = 0;
  
  if (old_data)
    {
    delete [] old_data;
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetAttribute(const char *name, const char *value)
{
  if (!name || !name[0] || !value || !value[0])
    {
    return;
    }

  int i;

  // Set an existing attribute

  for(i = 0; i < this->NumberOfAttributes; ++i)
    {
    if(!strcmp(this->AttributeNames[i], name))
      {
      delete [] this->AttributeValues[i];
      this->AttributeValues[i] = new char [strlen(value) + 1];
      strcpy(this->AttributeValues[i], value);
      return;
      }
    }

  // Or add an attribute

  if (this->NumberOfAttributes == this->AttributesSize)
    {
    int newSize = this->AttributesSize * 2;
    char **newAttributeNames = new char* [newSize];
    char **newAttributeValues = new char* [newSize];
    for(i = 0; i < this->NumberOfAttributes; ++i)
      {
      newAttributeNames[i] = new char [strlen(this->AttributeNames[i]) + 1];
      strcpy(newAttributeNames[i], this->AttributeNames[i]);
      delete [] this->AttributeNames[i];
      newAttributeValues[i] = new char [strlen(this->AttributeValues[i]) + 1];
      strcpy(newAttributeValues[i], this->AttributeValues[i]);
      delete [] this->AttributeValues[i];
      }
    delete [] this->AttributeNames;
    delete [] this->AttributeValues;
    this->AttributeNames = newAttributeNames;
    this->AttributeValues = newAttributeValues;
    this->AttributesSize = newSize;
    }
  
  i = this->NumberOfAttributes++;
  this->AttributeNames[i] = new char [strlen(name) + 1];
  strcpy(this->AttributeNames[i], name);
  this->AttributeValues[i] = new char [strlen(value) + 1];
  strcpy(this->AttributeValues[i], value);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::AddNestedElement(vtkXMLDataElement* element)
{
  if (!element)
    {
    return;
    }
  
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
void vtkXMLDataElement::RemoveNestedElement(vtkXMLDataElement* element)
{
  if (!element)
    {
    return;
    }
  
  int i, j;
  for(i = 0; i < this->NumberOfNestedElements; ++i)
    {
    if (this->NestedElements[i] == element)
      {
      for (j = i; j < this->NumberOfNestedElements - 1; ++j)
        {
        this->NestedElements[j] = this->NestedElements[j + 1];
        }
      element->UnRegister(this);
      this->NumberOfNestedElements--;
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkXMLDataElement::GetAttribute(const char* name)
{
  if (!name)
    {
    return 0;
    }

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
const char* vtkXMLDataElement::GetAttributeName(int idx)
{
  if (idx < 0 || idx >= this->NumberOfAttributes)
    {
    return 0;
    }

  return this->AttributeNames[idx];
}

//----------------------------------------------------------------------------
const char* vtkXMLDataElement::GetAttributeValue(int idx)
{
  if (idx < 0 || idx >= this->NumberOfAttributes)
    {
    return 0;
    }

  return this->AttributeValues[idx];
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
vtkXMLDataElement* vtkXMLDataElement::GetRoot()
{
  if (!this->Parent)
    {
    return this;
    }
  return this->Parent->GetRoot();
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetNumberOfNestedElements()
{
  return this->NumberOfNestedElements;
}
  
//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::GetNestedElement(int index)
{
  if (index >=0 && index < this->NumberOfNestedElements)
    {
    return this->NestedElements[index];
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::FindNestedElementWithName(const char* name)
{
  if (!name)
    {
    return 0;
    }

  int i;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    const char* nname = this->NestedElements[i]->GetName();
    if(nname && (strcmp(nname, name) == 0))
      {
      return this->NestedElements[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::FindNestedElementWithNameAndId(
  const char* name, const char* id)
{
  if (!name || !id)
    {
    return 0;
    }

  int i;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    const char* nname = this->NestedElements[i]->GetName();
    const char* nid = this->NestedElements[i]->GetId();
    if(nname && nid && (strcmp(nname, name) == 0) && (strcmp(nid, id) == 0))
      {
      return this->NestedElements[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLDataElement::FindNestedElementWithNameAndAttribute(
  const char* name, const char* att_name, const char* att_value)
{
  if (!name || !att_name || !att_value)
    {
    return 0;
    }

  int i;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    const char* nname = this->NestedElements[i]->GetName();
    if(nname && (strcmp(nname, name) == 0))
      {
      const char *val = this->NestedElements[i]->GetAttribute(att_name);
      if (val && !strcmp(val, att_value))
        {
        return this->NestedElements[i];
        }
      }
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
  if (!id)
    {
    return 0;
    }

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
  if (!id)
    {
    return 0;
    }

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
  if (!id)
    {
    return 0;
    }

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
int vtkXMLDataElement::GetScalarAttribute(const char* name, double& value)
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
#ifdef VTK_USE_64BIT_IDS
int vtkXMLDataElement::GetScalarAttribute(const char* name, vtkIdType& value)
{
  return this->GetVectorAttribute(name, 1, &value);
}
#endif

//----------------------------------------------------------------------------
template <class T>
int vtkXMLDataElementVectorAttributeParse(const char* str, int length, T* data)
{
  if(!str || !length || !data) { return 0; }
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
  return vtkXMLDataElementVectorAttributeParse(this->GetAttribute(name),
                                               length, data);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          float* data)
{
  return vtkXMLDataElementVectorAttributeParse(this->GetAttribute(name),
                                               length, data);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          double* data)
{
  return vtkXMLDataElementVectorAttributeParse(this->GetAttribute(name),
                                               length, data);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          unsigned long* data)
{
  return vtkXMLDataElementVectorAttributeParse(this->GetAttribute(name),
                                               length, data);
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
int vtkXMLDataElement::GetVectorAttribute(const char* name, int length,
                                          vtkIdType* data)
{
  return vtkXMLDataElementVectorAttributeParse(this->GetAttribute(name),
                                               length, data);
}
#endif

//----------------------------------------------------------------------------
int vtkXMLDataElement::GetWordTypeAttribute(const char* name, int& value)
{
  // These string values must match vtkXMLWriter::GetWordTypeName().
  const char* v = this->GetAttribute(name);
  if(!v)
    {
    vtkErrorMacro("Missing word type attribute \"" << name << "\".");
    return 0;
    }
  else if(strcmp(v, "Float32") == 0)
    {
#if VTK_SIZEOF_FLOAT == 4
    value = VTK_FLOAT;
    return 1;
#elif VTK_SIZEOF_DOUBLE == 4
    value = VTK_DOUBLE;
    return 1;
#else
    vtkErrorMacro("Float32 support not compiled in VTK.");
    return 0;
#endif
    }
  else if(strcmp(v, "Float64") == 0)
    {
#if VTK_SIZEOF_FLOAT == 8
    value = VTK_FLOAT;
    return 1;
#elif VTK_SIZEOF_DOUBLE == 8
    value = VTK_DOUBLE;
    return 1;
#else
    vtkErrorMacro("Float64 support not compiled in VTK.");
    return 0;
#endif
    }
  else if(strcmp(v, "Int8") == 0)
    {
    // For compatibility, use char when it is signed.
#if VTK_TYPE_CHAR_IS_SIGNED
    value = VTK_CHAR;
#else
    value = VTK_SIGNED_CHAR;
#endif
    return 1;
    }
  else if(strcmp(v, "UInt8") == 0)
    {
    value = VTK_UNSIGNED_CHAR;
    return 1;
    }
  else if(strcmp(v, "Int16") == 0)
    {
#if VTK_SIZEOF_SHORT == 2
    value = VTK_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 2
    value = VTK_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 2
    value = VTK_LONG;
    return 1;
#else
    vtkErrorMacro("Int16 support not compiled in VTK.");
    return 0;
#endif    
    }
  else if(strcmp(v, "UInt16") == 0)
    {
#if VTK_SIZEOF_SHORT == 2
    value = VTK_UNSIGNED_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 2
    value = VTK_UNSIGNED_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 2
    value = VTK_UNSIGNED_LONG;
    return 1;
#else
    vtkErrorMacro("UInt16 support not compiled in VTK.");
    return 0;
#endif    
    }
  else if(strcmp(v, "Int32") == 0)
    {
#if VTK_SIZEOF_SHORT == 4
    value = VTK_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 4
    value = VTK_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 4
    value = VTK_LONG;
    return 1;
#else
    vtkErrorMacro("Int32 support not compiled in VTK.");
    return 0;
#endif    
    }
  else if(strcmp(v, "UInt32") == 0)
    {
#if VTK_SIZEOF_SHORT == 4
    value = VTK_UNSIGNED_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 4
    value = VTK_UNSIGNED_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 4
    value = VTK_UNSIGNED_LONG;
    return 1;
#else
    vtkErrorMacro("UInt32 support not compiled in VTK.");
    return 0;
#endif    
    }
  else if(strcmp(v, "Int64") == 0)
    {
#if VTK_SIZEOF_SHORT == 8
    value = VTK_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 8
    value = VTK_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 8
    value = VTK_LONG;
    return 1;
#elif defined(VTK_TYPE_USE_LONG_LONG) && VTK_SIZEOF_LONG_LONG == 8
    value = VTK_LONG_LONG;
    return 1;
#elif defined(VTK_TYPE_USE___INT64) && VTK_SIZEOF___INT64 == 8
    value = VTK___INT64;
    return 1;
#else
    vtkErrorMacro("Int64 support not compiled in VTK.");
    return 0;
#endif    
    }
  else if(strcmp(v, "UInt64") == 0)
    {
#if VTK_SIZEOF_SHORT == 8
    value = VTK_UNSIGNED_SHORT;
    return 1;
#elif VTK_SIZEOF_INT == 8
    value = VTK_UNSIGNED_INT;
    return 1;
#elif VTK_SIZEOF_LONG == 8
    value = VTK_UNSIGNED_LONG;
    return 1;
#elif defined(VTK_TYPE_USE_LONG_LONG) && VTK_SIZEOF_LONG_LONG == 8
    value = VTK_UNSIGNED_LONG_LONG;
    return 1;
#elif defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE) && VTK_SIZEOF___INT64 == 8
    value = VTK_UNSIGNED___INT64;
    return 1;
#else
    vtkErrorMacro("UInt64 support not compiled in VTK.");
    return 0;
#endif
    }
  else if (strcmp(v, "String") == 0)
    {
    value = VTK_STRING;
    return 1;
    }
  else
    {
    vtkErrorMacro("Unknown data type \"" << v << "\".  Supported types are:\n"
                  "Int8,  Int16,  Int32,  Int64,\n"
                  "UInt8, UInt16, UInt32, UInt64,\n"
                  "Float32, Float64, String\n");
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetIntAttribute(const char* name, int value)
{
  this->SetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetFloatAttribute(const char* name, float value)
{
  this->SetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetDoubleAttribute(const char* name, double value)
{
  this->SetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetUnsignedLongAttribute(const char* name,
                                                 unsigned long value)
{
  this->SetVectorAttribute(name, 1, &value);
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
void vtkXMLDataElement::SetIdTypeAttribute(const char* name, 
                                           vtkIdType value)
{
  this->SetVectorAttribute(name, 1, &value);
}
#endif

//----------------------------------------------------------------------------
template <class T>
void vtkXMLDataElementVectorAttributeSet(vtkXMLDataElement *elem, const char* name, int length, const T* data)
{
  if (!elem || !name || !length) 
    { 
    return; 
    }
  strstream vstr;
  vstr << data[0];
  for(int i = 1; i < length; ++i)
    {
    vstr << ' ' << data[i];
    }
  vstr << ends;
  elem->SetAttribute(name, vstr.str());
  vstr.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetVectorAttribute(const char* name, int length,
                                           const int* data)
{
  vtkXMLDataElementVectorAttributeSet(this, name, length, data);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetVectorAttribute(const char* name, int length,
                                           const float* data)
{
  vtkXMLDataElementVectorAttributeSet(this, name, length, data);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetVectorAttribute(const char* name, int length,
                                           const double* data)
{
  vtkXMLDataElementVectorAttributeSet(this, name, length, data);
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::SetVectorAttribute(const char* name, int length,
                                           const unsigned long* data)
{
  vtkXMLDataElementVectorAttributeSet(this, name, length, data);
}

//----------------------------------------------------------------------------
#ifdef VTK_USE_64BIT_IDS
void vtkXMLDataElement::SetVectorAttribute(const char* name, int length,
                                           const vtkIdType* data)
{
  vtkXMLDataElementVectorAttributeSet(this, name, length, data);
}
#endif

//----------------------------------------------------------------------------
void vtkXMLDataElement::SeekInlineDataPosition(vtkXMLDataParser* parser)
{
  if (!parser)
    {
    return;
    }

  istream* stream = parser->GetStream();
  if(!this->InlineDataPosition)
    {
    // Scan for the start of the actual inline data.
    char c=0;
    stream->clear(stream->rdstate() & ~ios::eofbit);
    stream->clear(stream->rdstate() & ~ios::failbit);
    parser->SeekG(this->GetXMLByteIndex());
    while(stream->get(c) && (c != '>'));
    while(stream->get(c) && this->IsSpace(c));
    unsigned long pos = parser->TellG();
    this->InlineDataPosition = pos-1;
    }

  // Seek to the data position.
  parser->SeekG(this->InlineDataPosition);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::IsSpace(char c)
{
  return isspace(c);
}

//----------------------------------------------------------------------------
int vtkXMLDataElement::IsEqualTo(vtkXMLDataElement *elem)
{
  if (this == elem)
    {
    return 1;
    }

  if (!elem)
    {
    return 0;
    }

  if (this->GetNumberOfAttributes() != elem->GetNumberOfAttributes() ||
      this->GetNumberOfNestedElements() != elem->GetNumberOfNestedElements() ||
      (this->GetName() != elem->GetName() && 
       (!this->GetName() || !elem->GetName() || strcmp(this->GetName(), 
                                                       elem->GetName()))))
    {
    return 0;
    }

  // Compare attributes

  int i;
  for (i = 0; i < this->GetNumberOfAttributes(); ++i)
    {
    const char *value = elem->GetAttribute(this->AttributeNames[i]);
    if (!value || strcmp(value, this->AttributeValues[i]))
      {
      return 0;
      }
    }

  // Compare nested elements

  for (i = 0; i < this->GetNumberOfNestedElements(); ++i)
    {
    if (!this->GetNestedElement(i)->IsEqualTo(elem->GetNestedElement(i)))
      {
      return 0;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::DeepCopy(vtkXMLDataElement *elem)
{
  if (!elem)
    {
    return;
    }

  this->SetName(elem->GetName());
  this->SetId(elem->GetId());
  this->SetXMLByteIndex(elem->GetXMLByteIndex());
  this->SetAttributeEncoding(elem->GetAttributeEncoding());

  // Copy attributes

  this->RemoveAllAttributes();

  int i;
  for (i = 0; i < elem->GetNumberOfAttributes(); ++i)
    {
    const char *att_name = elem->GetAttributeName(i);
    this->SetAttribute(att_name, elem->GetAttribute(att_name));
    }
  
  // Copy nested elements

  this->RemoveAllNestedElements();

  for (i = 0; i < elem->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement *nested_elem = vtkXMLDataElement::New();
    nested_elem->DeepCopy(elem->GetNestedElement(i));
    this->AddNestedElement(nested_elem);
    nested_elem->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLByteIndex: " << this->XMLByteIndex << "\n";
  os << indent << "Name: " << (this->Name? this->Name : "(none)") << "\n";
  os << indent << "Id: " << (this->Id? this->Id : "(none)") << "\n";
  os << indent << "NumberOfAttributes: " << this->NumberOfAttributes << "\n";
  os << indent << "AttributeEncoding: " << this->AttributeEncoding << "\n";
  os << indent << "CharacterData: " << 
    (this->CharacterData? this->CharacterData : "(null)") << endl;
}

