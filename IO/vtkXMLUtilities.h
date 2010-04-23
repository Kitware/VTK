/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLUtilities - XML utilities.
// .SECTION Description
// vtkXMLUtilities provides XML-related convenience functions.
// .SECTION See Also
// vtkXMLDataElement

#ifndef __vtkXMLUtilities_h
#define __vtkXMLUtilities_h

#include "vtkObject.h"

class vtkXMLDataElement;

class VTK_IO_EXPORT vtkXMLUtilities : public vtkObject
{
public:
  static vtkXMLUtilities* New();
  vtkTypeMacro(vtkXMLUtilities, vtkObject);

  // Description:
  // Encode a string from one format to another 
  // (see VTK_ENCODING_... constants).
  // If special_entites is true, convert some characters to their corresponding
  // character entities.
  static void EncodeString(const char *input, int input_encoding, 
                           ostream &output, int output_encoding,
                           int special_entities = 0);

  // Description:
  // Collate a vtkXMLDataElement's attributes to a stream as a series of
  // name="value" pairs (the separator between each pair can be specified,
  // if not, it defaults to a space). 
  // Note that the resulting character-encoding will be UTF-8 (we assume
  // that this function is used to create XML files/streams).
  static void CollateAttributes(vtkXMLDataElement*, 
                                ostream&, 
                                const char *sep = 0);

  //BTX
  // Description:
  // Flatten a vtkXMLDataElement to a stream, i.e. output a textual stream
  // corresponding to that XML element, its attributes and its
  // nested elements.
  // If 'indent' is not NULL, it is used to indent the whole tree.
  // If 'indent' is not NULL and 'indent_attributes' is true, attributes will 
  // be indented as well.
  // Note that the resulting character-encoding will be UTF-8 (we assume
  // that this function is used to create XML files/streams).
  static void FlattenElement(vtkXMLDataElement*, 
                             ostream&, 
                             vtkIndent *indent = 0,
                             int indent_attributes = 1);

  // Description:
  // Write a vtkXMLDataElement to a file (in a flattened textual form)
  // Note that the resulting character-encoding will be UTF-8.
  // Return 1 on success, 0 otherwise.
  static int WriteElementToFile(vtkXMLDataElement*, 
                                const char *filename, 
                                vtkIndent *indent = 0);
  //ETX

  // Description:
  // Read a vtkXMLDataElement from a stream, string or file.
  // The 'encoding' parameter will be used to set the internal encoding of the
  // attributes of the data elements created by those functions (conversion
  // from the XML stream encoding to that new encoding will be performed
  // automatically). If set to VTK_ENCODING_NONE, the encoding won't be
  // changed and will default to the default vtkXMLDataElement encoding.
  // Return the root element on success, NULL otherwise.
  // Note that you have to call Delete() on the element returned by that
  // function to ensure it is freed properly.
  //BTX
  static vtkXMLDataElement* ReadElementFromStream(
    istream&, int encoding = VTK_ENCODING_NONE);
  static vtkXMLDataElement* ReadElementFromString(
    const char *str, int encoding = VTK_ENCODING_NONE);
  static vtkXMLDataElement* ReadElementFromFile(
    const char *filename, int encoding = VTK_ENCODING_NONE);
  //ETX

  // Description:
  // Sets attributes of an element from an array of encoded attributes.
  // The 'encoding' parameter will be used to set the internal encoding of the
  // attributes of the data elements created by those functions (conversion
  // from the XML stream encoding to that new encoding will be performed
  // automatically). If set to VTK_ENCODING_NONE, the encoding won't be
  // changed and will default to the default vtkXMLDataElement encoding.
  static void ReadElementFromAttributeArray(
        vtkXMLDataElement *element,
        const char** atts,
        int encoding);

  // Description:
  // Find all elements in 'tree' that are similar to 'elem' (using the
  // vtkXMLDataElement::IsEqualTo() predicate). 
  // Return the number of elements found and store those elements in
  // 'results' (automatically allocated).
  // Warning: the results do not include 'elem' if it was found in the tree ;
  // do not forget to deallocate 'results' if something was found.
  //BTX
  static int FindSimilarElements(vtkXMLDataElement *elem, 
                                 vtkXMLDataElement *tree, 
                                 vtkXMLDataElement ***results);
  //ETX

  // Description:
  // Factor and unfactor a tree. This operation looks for duplicate elements
  // in the tree, and replace them with references to a pool of elements.
  // Unfactoring a non-factored element is harmless.
  static void FactorElements(vtkXMLDataElement *tree);
  static void UnFactorElements(vtkXMLDataElement *tree);

protected:  
  vtkXMLUtilities() {};
  ~vtkXMLUtilities() {};
  
  static int FactorElementsInternal(vtkXMLDataElement *tree, 
                                    vtkXMLDataElement *root, 
                                    vtkXMLDataElement *pool);
  static int UnFactorElementsInternal(vtkXMLDataElement *tree, 
                                      vtkXMLDataElement *pool);

private:
  vtkXMLUtilities(const vtkXMLUtilities&); // Not implemented
  void operator=(const vtkXMLUtilities&); // Not implemented    
};

#endif


