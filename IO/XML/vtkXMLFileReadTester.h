/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLFileReadTester.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLFileReadTester
 * @brief   Utility class for vtkXMLReader and subclasses.
 *
 * vtkXMLFileReadTester reads the smallest part of a file necessary to
 * determine whether it is a VTK XML file.  If so, it extracts the
 * file type and version number.
*/

#ifndef vtkXMLFileReadTester_h
#define vtkXMLFileReadTester_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLParser.h"

class VTKIOXML_EXPORT vtkXMLFileReadTester: public vtkXMLParser
{
public:
  vtkTypeMacro(vtkXMLFileReadTester,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLFileReadTester* New();

  /**
   * Try to read the file given by FileName.  Returns 1 if the file is
   * a VTK XML file, and 0 otherwise.
   */
  int TestReadFile();

  //@{
  /**
   * Get the data type of the XML file tested.  If the file could not
   * be read, returns NULL.
   */
  vtkGetStringMacro(FileDataType);
  //@}

  //@{
  /**
   * Get the file version of the XML file tested.  If the file could not
   * be read, returns NULL.
   */
  vtkGetStringMacro(FileVersion);
  //@}

protected:
  vtkXMLFileReadTester();
  ~vtkXMLFileReadTester() VTK_OVERRIDE;

  void StartElement(const char* name, const char** atts) VTK_OVERRIDE;
  int ParsingComplete() VTK_OVERRIDE;
  void ReportStrayAttribute(const char*, const char*, const char*) VTK_OVERRIDE {}
  void ReportMissingAttribute(const char*, const char*) VTK_OVERRIDE {}
  void ReportBadAttribute(const char*, const char*, const char*) VTK_OVERRIDE {}
  void ReportUnknownElement(const char*) VTK_OVERRIDE {}
  void ReportXmlParseError() VTK_OVERRIDE {}

  char* FileDataType;
  char* FileVersion;
  int Done;

  vtkSetStringMacro(FileDataType);
  vtkSetStringMacro(FileVersion);

private:
  vtkXMLFileReadTester(const vtkXMLFileReadTester&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLFileReadTester&) VTK_DELETE_FUNCTION;
};

#endif
