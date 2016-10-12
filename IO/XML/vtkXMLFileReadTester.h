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
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLFileReadTester* New();

  /**
   * Try to read the file given by FileName.  Returns 1 if the file is
   * a VTK XML file, and 0 otherwise.
   */
  int TestReadFile();

  //@{
  /**
   * Get/Set the name of the file tested by TestReadFile().
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

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
  ~vtkXMLFileReadTester();

  void StartElement(const char* name, const char** atts);
  int ParsingComplete();
  void ReportStrayAttribute(const char*, const char*, const char*) {}
  void ReportMissingAttribute(const char*, const char*) {}
  void ReportBadAttribute(const char*, const char*, const char*) {}
  void ReportUnknownElement(const char*) {}
  void ReportXmlParseError() {}

  char* FileName;
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
