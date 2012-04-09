/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkXMLTreeReader - reads an XML file into a vtkTree
//
// .SECTION Description
// vtkXMLTreeReader parses an XML file and uses the nesting structure of the
// XML tags to generate a tree.  Node attributes are assigned to node arrays,
// and the special arrays .tagname and .chardata contain the tag type and the
// text internal to the tag, respectively.  The arrays are of type
// vtkStringArray.  There is an array for each attribute type in the XML file,
// even if it appears in only one tag.  If an attribute is missing from a tag,
// its value is the empty string.
//
// If MaskArrays is on (the default is off), the filter will additionally make bit
// arrays whose names are prepended with ".valid." which are 1 if the element 
// contains that attribute, and 0 otherwise. 
//
// For example, the XML file containing the text:
// <pre>
// &lt;node name="jeff" age="26"&gt;
//   this is text in jeff's node
//   &lt;node name="joe"&gt;
//     &lt;node name="al" initials="amb" other="something"/&gt;
//     &lt;node name="dave" age="30"/&gt;
//   &lt;/node&gt;
//   &lt;node name="lisa"&gt;this is text in lisa's node&lt;/node&gt;
//   &lt;node name="darlene" age="29"/&gt;
// &lt;/node&gt;
// </pre>
//
// would be parsed into a tree with the following node IDs and structure:
//
// <pre>
// 0 (jeff) - children: 1 (joe), 4 (lisa), 5 (darlene)
// 1 (joe)  - children: 2 (al), 3 (dave)
// 2 (al)
// 3 (dave)
// 4 (lisa)
// 5 (darlene)
// </pre>
//
// and the node data arrays would be as follows:
//
// <pre>
// name      initials  other     age       .tagname  .chardata
// ------------------------------------------------------------------------------------------------
// jeff      (empty)   (empty)   26         node     "  this is text in jeff's node\n  \n  \n  \n"
// joe       (empty)   (empty)   (empty)    node     "\n    \n    \n  "
// al        amb       something (empty)    node     (empty)
// dave      (empty)   (empty)   30         node     (empty)
// lisa      (empty)   (empty)   (empty)    node     "this is text in lisa's node"
// darlene   (empty)   (empty)   29         node     (empty)
// </pre>
//
// There would also be the following bit arrays if MaskArrays is on:
//
// <pre>
// .valid.name   .valid.initials   .valid.other   .valid.age
// ---------------------------------------------------------
// 1             0                 0              1
// 1             0                 0              0
// 1             1                 1              0
// 1             0                 0              1
// 1             0                 0              0
// 1             0                 0              1
// </pre>


#ifndef __vtkXMLTreeReader_h
#define __vtkXMLTreeReader_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkXMLTreeReader : public vtkTreeAlgorithm
{
public:
  static vtkXMLTreeReader* New();
  vtkTypeMacro(vtkXMLTreeReader,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If set, reads in the XML file specified.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  // Description:
  // If set, and FileName is not set, reads in the XML string.
  vtkGetStringMacro(XMLString);
  vtkSetStringMacro(XMLString);

  // Description:
  // The name of the edge pedigree ids. Default is "edge id".
  vtkGetStringMacro(EdgePedigreeIdArrayName);
  vtkSetStringMacro(EdgePedigreeIdArrayName);

  // Description:
  // The name of the vertex pedigree ids. Default is "vertex id".
  vtkGetStringMacro(VertexPedigreeIdArrayName);
  vtkSetStringMacro(VertexPedigreeIdArrayName);

  // Description:
  // Set whether to use an property from the XML file as pedigree ids (off),
  // or generate a new array with integer values starting at zero (on).
  // Default is on.
  vtkSetMacro(GenerateEdgePedigreeIds, bool);
  vtkGetMacro(GenerateEdgePedigreeIds, bool);
  vtkBooleanMacro(GenerateEdgePedigreeIds, bool);
  vtkSetMacro(GenerateVertexPedigreeIds, bool);
  vtkGetMacro(GenerateVertexPedigreeIds, bool);
  vtkBooleanMacro(GenerateVertexPedigreeIds, bool);

  // Description:
  // If on, makes bit arrays for each attribute with name .valid.attribute_name
  // for each attribute.  Default is off.
  vtkGetMacro(MaskArrays, bool);
  vtkSetMacro(MaskArrays, bool);
  vtkBooleanMacro(MaskArrays, bool);

  // Description:
  // If on, stores the XML character data (i.e. textual data between tags)
  // into an array named CharDataField, otherwise this field is skipped.
  // Default is off.
  vtkGetMacro(ReadCharData, bool);
  vtkSetMacro(ReadCharData, bool);
  vtkBooleanMacro(ReadCharData, bool);
  
  // Description:
  // If on, stores the XML tag name data in a field called .tagname
  // otherwise this field is skipped.
  // Default is on.
  vtkGetMacro(ReadTagName, bool);
  vtkSetMacro(ReadTagName, bool);
  vtkBooleanMacro(ReadTagName, bool);


  static const char * TagNameField;
  static const char * CharDataField;

protected:
  vtkXMLTreeReader();
  ~vtkXMLTreeReader();
  char* FileName;
  char* XMLString;
  bool ReadCharData;
  bool ReadTagName;
  bool MaskArrays;
  char* EdgePedigreeIdArrayName;
  char* VertexPedigreeIdArrayName;
  bool GenerateEdgePedigreeIds;
  bool GenerateVertexPedigreeIds;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkXMLTreeReader(const vtkXMLTreeReader&); // Not implemented
  void operator=(const vtkXMLTreeReader&);   // Not implemented
};

#endif

