/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtXMLProcessor.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkQtXMLProcessor - Processes field data as if it were XML.
//
// .SECTION Description - Maps vtkFieldData to XML data, which is then
// processed using either XQuery or XSLT.
//
// Conceptually, vtkQtXMLProcessor treats an input vtkFieldData object
// as a "table" made-up of columns, rows, and cells/fields.  This table
// is mapped (using shallow-copy semantics) into an XML document that
// can then be processed using XQuery or XSLT.
//
// Following is an example of how a field data containing two arrays
// named "foo" and "bar" might be mapped into XML:
//
// <table>
//   <rows>
//     <row>
//       <foo>value of foo at index 0</foo>
//       <bar>value of bar at index 0</bar>
//     </row>
//     <row>
//       <foo>value of foo at index 1</foo>
//       <bar>value of bar at index 1</bar>
//     </row>
//     ...
//   </rows>
// </table>
//
// Note how the array names are mapped to elements in the resulting
// XML.  Because there are strict rules on the naming of XML elements,
// array names will be automatically "mangled" to produce conforming
// element names.  Users may optionally supply their own mappings from
// array names to element names.
//
// Parameters:
//   FieldType: Controls which field data should be extracted from
//     the input for processing.
//
//   InputDomain: Controls whether XML processing will be applied to
//     individual rows (the default) or the entire input field data.
//
//   QueryType: Controls whether to use XQuery or XSLT for processing.
//
//   Query: The XQuery or XSLT template to be used for processing.
//
//   OutputArray: The name of the output array that will store the
//     processed results.
//
// Inputs:
//   Input port 0: A vtkDataObject containing arbitrary field data.
//
// Outputs:
//   Output port 0: A shallow-copy of the input vtkDataObject.
//     If InputDomain is set to "ROW_DOMAIN" (the default), the data
//     object's field data will contain an additional string array
//     containing the results of running XQuery / XSLT on each
//     individual "row" in the field data.
//
//     If InputDomain is set to "DATA_OBJECT_DOMAIN", the data object
//     will be identical to the input.
//
//   Output port 1: A vtkTable.  If InputDomain is set to "ROW_DOMAIN"
//     (the default), the table will be completely empty.
//     If InputDomain is set to "DATA_OBJECT_DOMAIN", the table 
//     will contain a single string array with a single value 
//     containing the results of running XQuery / XSLT on the
//     entire contents of the input field data.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkQtXMLProcessor_h
#define __vtkQtXMLProcessor_h

#include "vtkPassInputTypeAlgorithm.h"

#include "vtkStdString.h" //avoids an error when building plugins

class VTK_TEXT_ANALYSIS_EXPORT vtkQtXMLProcessor:
  public vtkPassInputTypeAlgorithm
{
public:
  static vtkQtXMLProcessor* New();
  vtkTypeRevisionMacro(vtkQtXMLProcessor,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
//BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5
    };

  enum
    {
    ROW_DOMAIN = 0,
    DATA_OBJECT_DOMAIN = 1,
    VALUE_DOMAIN = 2
    };

  enum
    {
    XQUERY = 0,
    XSLT = 1
    };

//ETX
  
  vtkGetMacro(FieldType, int);
  vtkSetMacro(FieldType, int);

  vtkGetMacro(InputDomain, int);
  vtkSetMacro(InputDomain, int);

  vtkGetMacro(QueryType, int);
  vtkSetMacro(QueryType, int);

  vtkSetStringMacro(Query);
  vtkGetStringMacro(Query);

  vtkSetStringMacro(OutputArray);
  vtkGetStringMacro(OutputArray);

  void MapArrayName(const vtkStdString& from, const vtkStdString& to);
  void ClearArrayNameMap();

//BTX
protected:
  vtkQtXMLProcessor();
  ~vtkQtXMLProcessor();
  
  int FillOutputPortInformation(int port, vtkInformation* info);
  
  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
  
private:
  vtkQtXMLProcessor(const vtkQtXMLProcessor&); // Not implemented
  void operator=(const vtkQtXMLProcessor&);   // Not implemented

  class XMLAdapter;
  class Internals;
  Internals* const Implementation;

  int FieldType;
  int InputDomain;
  int QueryType;
  char* Query;
  char* OutputArray;
//ETX
};

#endif

