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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLFileReadTester* New();

  /**
   * Try to read the file given by FileName.  Returns 1 if the file is
   * a VTK XML file, and 0 otherwise.
   */
  int TestReadFile();

  //@{
  /**
   * Get the data type of the XML file tested.  If the file could not
   * be read, returns nullptr.
   */
  vtkGetStringMacro(FileDataType);
  //@}

  //@{
  /**
   * Get the file version of the XML file tested.  If the file could not
   * be read, returns nullptr.
   */
  vtkGetStringMacro(FileVersion);
  //@}

protected:
  vtkXMLFileReadTester();
  ~vtkXMLFileReadTester() override;

  void StartElement(const char* name, const char** atts) override;
  int ParsingComplete() override;
  void ReportStrayAttribute(const char*, const char*, const char*) override {}
  void ReportMissingAttribute(const char*, const char*) override {}
  void ReportBadAttribute(const char*, const char*, const char*) override {}
  void ReportUnknownElement(const char*) override {}
  void ReportXmlParseError() override {}

  char* FileDataType;
  char* FileVersion;
  int Done;

  vtkSetStringMacro(FileDataType);
  vtkSetStringMacro(FileVersion);

private:
  vtkXMLFileReadTester(const vtkXMLFileReadTester&) = delete;
  void operator=(const vtkXMLFileReadTester&) = delete;
};

#endif
