// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLDataElement
 * @brief   Represents an XML element and those nested inside.
 *
 * vtkXMLDataElement is used by vtkXMLDataParser to represent an XML
 * element.  It provides methods to access the element's attributes
 * and nested elements in a convenient manner.  This allows easy
 * traversal of an input XML file by vtkXMLReader and its subclasses.
 *
 * @sa
 * vtkXMLDataParser
 */

#ifndef vtkXMLDataElement_h
#define vtkXMLDataElement_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkXMLDataParser;

class VTKCOMMONDATAMODEL_EXPORT vtkXMLDataElement : public vtkObject
{
public:
  vtkTypeMacro(vtkXMLDataElement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLDataElement* New();

  ///@{
  /**
   * Set/Get the name of the element.  This is its XML tag.
   */
  vtkGetStringMacro(Name);
  virtual void SetName(const char* _arg);
  ///@}

  ///@{
  /**
   * Set/Get the value of the id attribute of the element, if any.
   */
  vtkGetStringMacro(Id);
  vtkSetStringMacro(Id);
  ///@}

  /**
   * Get the attribute with the given name.  If it doesn't exist,
   * returns 0.
   */
  const char* GetAttribute(const char* name);

  /**
   * Set the attribute with the given name and value. If it doesn't exist,
   * adds it.
   */
  void SetAttribute(const char* name, const char* value);

  ///@{
  /**
   * Set/Get the character data between XML start/end tags.
   */
  void SetCharacterData(const char* data, int length);
  void AddCharacterData(const char* c, size_t length);
  vtkGetStringMacro(CharacterData);
  ///@}

  ///@{
  /**
   * Get the attribute with the given name and converted to a scalar
   * value.  Returns whether value was extracted.
   */
  int GetScalarAttribute(const char* name, int& value);
  int GetScalarAttribute(const char* name, float& value);
  int GetScalarAttribute(const char* name, double& value);
  int GetScalarAttribute(const char* name, long& value);
  int GetScalarAttribute(const char* name, unsigned long& value);
  ///@}

  ///@{
  /**
   * Set the attribute with the given name.
   * We can not use the same GetScalarAttribute() construct since
   * the compiler will not be able to resolve between
   * SetAttribute(..., int) and SetAttribute(..., unsigned long).
   */
  void SetIntAttribute(const char* name, int value);
  void SetFloatAttribute(const char* name, float value);
  void SetDoubleAttribute(const char* name, double value);
  void SetUnsignedLongAttribute(const char* name, unsigned long value);
  ///@}

  ///@{
  /**
   * Get the attribute with the given name and converted to a scalar
   * value.  Returns length of vector read.
   */
  int GetVectorAttribute(const char* name, int length, int* value);
  int GetVectorAttribute(const char* name, int length, float* value);
  int GetVectorAttribute(const char* name, int length, double* value);
  int GetVectorAttribute(const char* name, int length, long* value);
  int GetVectorAttribute(const char* name, int length, unsigned long* value);
  ///@}

  ///@{
  /**
   * Set the attribute with the given name.
   */
  void SetVectorAttribute(const char* name, int length, const int* value);
  void SetVectorAttribute(const char* name, int length, const float* value);
  void SetVectorAttribute(const char* name, int length, const double* value);
  void SetVectorAttribute(const char* name, int length, const unsigned long* value);
  ///@}

  int GetScalarAttribute(const char* name, long long& value);
  int GetVectorAttribute(const char* name, int length, long long* value);
  void SetVectorAttribute(const char* name, int length, long long const* value);
  int GetScalarAttribute(const char* name, unsigned long long& value);
  int GetVectorAttribute(const char* name, int length, unsigned long long* value);
  void SetVectorAttribute(const char* name, int length, unsigned long long const* value);

  /**
   * Get the attribute with the given name and converted to a word type.
   * Word types can be `VTK_TYPE_FLOAT32`, `VTK_TYPE_FLOAT64`, `VTK_INT8`,
   * `VTK_UINT8`, `VTK_INT16`, `VTK_UINT16`, `VTK_INT32`, `VTK_UINT32`,
   * `VTK_INT64`, `VTK_UINT64`, `VTK_STRING`, or `VTK_BIT`.
   */
  int GetWordTypeAttribute(const char* name, int& value);

  ///@{
  /**
   * Get the number of attributes.
   */
  vtkGetMacro(NumberOfAttributes, int);
  ///@}

  /**
   * Get the n-th attribute name.
   * Returns 0 if there is no such attribute.
   */
  const char* GetAttributeName(int idx);

  /**
   * Get the n-th attribute value.
   * Returns 0 if there is no such attribute.
   */
  const char* GetAttributeValue(int idx);

  ///@{
  /**
   * Remove one or all attributes.
   */
  virtual void RemoveAttribute(const char* name);
  virtual void RemoveAllAttributes();
  ///@}

  ///@{
  /**
   * Set/Get the parent of this element.
   */
  vtkXMLDataElement* GetParent();
  void SetParent(vtkXMLDataElement* parent);
  ///@}

  /**
   * Get root of the XML tree this element is part of.
   */
  virtual vtkXMLDataElement* GetRoot();

  /**
   * Get the number of elements nested in this one.
   */
  int GetNumberOfNestedElements();

  /**
   * Get the element nested in this one at the given index.
   */
  vtkXMLDataElement* GetNestedElement(int index);

  /**
   * Add nested element
   */
  void AddNestedElement(vtkXMLDataElement* element);

  /**
   * Remove nested element.
   */
  virtual void RemoveNestedElement(vtkXMLDataElement*);

  /**
   * Remove all nested elements.
   */
  virtual void RemoveAllNestedElements();

  ///@{
  /**
   * Find the first nested element with the given id, given name, or given
   * name and id.
   * WARNING: the search is only performed on the children, not
   * the grand-children.
   */
  vtkXMLDataElement* FindNestedElement(const char* id);
  vtkXMLDataElement* FindNestedElementWithName(const char* name);
  vtkXMLDataElement* FindNestedElementWithNameAndId(const char* name, const char* id);
  vtkXMLDataElement* FindNestedElementWithNameAndAttribute(
    const char* name, const char* att_name, const char* att_value);
  ///@}

  /**
   * Find the first nested element with given name.
   * WARNING: the search is performed on the whole XML tree.
   */
  vtkXMLDataElement* LookupElementWithName(const char* name);

  /**
   * Lookup the element with the given id, starting at this scope.
   */
  vtkXMLDataElement* LookupElement(const char* id);

  ///@{
  /**
   * Set/Get the offset from the beginning of the XML document to this element.
   */
  vtkGetMacro(XMLByteIndex, vtkTypeInt64);
  vtkSetMacro(XMLByteIndex, vtkTypeInt64);
  ///@}

  /**
   * Check if the instance has the same name, attributes, character data
   * and nested elements contents than the given element (this method is
   * applied recursively on the nested elements, and they must be stored in
   * the same order).
   * Warning: Id, Parent, XMLByteIndex are ignored.
   */
  virtual int IsEqualTo(vtkXMLDataElement* elem);

  /**
   * Copy this element from another of the same type (elem), recursively.
   * Old attributes and nested elements are removed, new ones are created
   * given the contents of 'elem'.
   * Warning: Parent is ignored.
   */
  virtual void DeepCopy(vtkXMLDataElement* elem);

  ///@{
  /**
   * Get/Set the internal character encoding of the attributes.
   * Default type is VTK_ENCODING_UTF_8.
   * Note that a vtkXMLDataParser has its own AttributesEncoding ivar. If
   * this ivar is set to something other than VTK_ENCODING_NONE, it will be
   * used to set the attribute encoding of each vtkXMLDataElement
   * created by this vtkXMLDataParser.
   */
  vtkSetClampMacro(AttributeEncoding, int, VTK_ENCODING_NONE, VTK_ENCODING_UNKNOWN);
  vtkGetMacro(AttributeEncoding, int);
  ///@}

  ///@{
  /**
   * Prints element tree as XML.
   */
  void PrintXML(ostream& os, vtkIndent indent);
  void PrintXML(VTK_FILEPATH const char* fname);
  ///@}

  ///@{
  /**
   * Get/Set the width (in number of fields) that character
   * data (that between open and closing tags ie. \<X\> ... \</X\>)
   * is printed. If the width is less than one the tag's character
   * data is printed all on one line. If it is greater than one
   * the character data is streamed inserting line feeds every
   * width number of fields. See PrintXML.
   */
  vtkGetMacro(CharacterDataWidth, int);
  vtkSetMacro(CharacterDataWidth, int);
  ///@}

protected:
  vtkXMLDataElement();
  ~vtkXMLDataElement() override;

  // The name of the element from the XML file.
  char* Name;
  // The value of the "id" attribute, if any was given.
  char* Id;

  int CharacterDataWidth;

  // Data inside of the tag's open and close. ie <X> character data </X>
  char* CharacterData;            // Null terminated buffer.
  size_t CharacterDataBlockSize;  // Allocation size if buffer needs expand
  size_t CharacterDataBufferSize; // Allocated size.
  size_t EndOfCharacterData;      // Number of bytes used.

  // Tags that have specialized character data handlers
  // can set this flag to improve performance. The default is unset.
  vtkTypeBool IgnoreCharacterData;

  // Get/Set the stream position of the elements inline data.
  vtkGetMacro(InlineDataPosition, vtkTypeInt64);
  vtkSetMacro(InlineDataPosition, vtkTypeInt64);
  // The offset into the XML stream where the inline data begins.
  vtkTypeInt64 InlineDataPosition;
  // The offset into the XML stream where the element begins.
  vtkTypeInt64 XMLByteIndex;

  // The raw property name/value pairs read from the XML attributes.
  char** AttributeNames;
  char** AttributeValues;
  int NumberOfAttributes;
  int AttributesSize;
  int AttributeEncoding;

  // The set of nested elements.
  int NumberOfNestedElements;
  int NestedElementsSize;
  vtkXMLDataElement** NestedElements;
  // The parent of this element.
  vtkXMLDataElement* Parent;

  // Internal utility methods.
  vtkXMLDataElement* LookupElementInScope(const char* id);
  vtkXMLDataElement* LookupElementUpScope(const char* id);
  static int IsSpace(char c);
  void PrintCharacterData(ostream& os, vtkIndent indent);
  static void PrintWithEscapedData(ostream& os, const char* data);

  friend class vtkXMLDataParser;
  friend class vtkXMLMaterialParser;

private:
  vtkXMLDataElement(const vtkXMLDataElement&) = delete;
  void operator=(const vtkXMLDataElement&) = delete;
};

//----------------------------------------------------------------------------
inline void vtkXMLDataElement::AddCharacterData(const char* data, size_t length)
{
  if (this->IgnoreCharacterData)
  {
    return;
  }
  // This is the index where we start to put the new data at.
  size_t eod = this->EndOfCharacterData - 1;
  // Check if the new data will write off the end. If it does
  // resize the character data buffer.
  this->EndOfCharacterData += length;
  if (this->EndOfCharacterData >= this->CharacterDataBufferSize)
  {
    while (this->EndOfCharacterData >= this->CharacterDataBufferSize)
    {
      this->CharacterDataBufferSize += this->CharacterDataBlockSize;
    }
    this->CharacterData =
      static_cast<char*>(realloc(this->CharacterData, this->CharacterDataBufferSize));
  }
  // put the new data at the end of the buffer, and null terminate.
  char* pCD = this->CharacterData + eod;
  memmove(pCD, data, length);
  pCD[length] = '\0';
}

VTK_ABI_NAMESPACE_END
#endif
