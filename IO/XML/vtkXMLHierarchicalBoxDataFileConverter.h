/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHierarchicalBoxDataFileConverter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalBoxDataFileConverter - converts older *.vth, *.vthb
// files to newer format.
// .SECTION Description
// vtkXMLHierarchicalBoxDataFileConverter is a utility class to convert v0.1 and
// v1.0 of the VTK XML hierarchical file format to the v1.1. Users can then use
// vtkXMLUniformGridAMRReader to read the dataset into VTK.

#ifndef __vtkXMLHierarchicalBoxDataFileConverter_h
#define __vtkXMLHierarchicalBoxDataFileConverter_h

#include "vtkObject.h"
#include "vtkIOXMLModule.h" // needed for export macro.

class vtkXMLDataElement;

class VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataFileConverter : public vtkObject
{
public:
  static vtkXMLHierarchicalBoxDataFileConverter* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataFileConverter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input filename.
  vtkSetStringMacro(InputFileName);
  vtkGetStringMacro(InputFileName);

  // Description:
  // Set the output filename.
  vtkSetStringMacro(OutputFileName);
  vtkGetStringMacro(OutputFileName);

  // Description:
  // Converts the input file to new format and writes out the output file.
  bool Convert();

//BTX
protected:
  vtkXMLHierarchicalBoxDataFileConverter();
  ~vtkXMLHierarchicalBoxDataFileConverter();

  vtkXMLDataElement* ParseXML(const char* filename);

  // Returns GridDescription. VTK_UNCHANGED for invalid/failure.
  int GetOriginAndSpacing(
    vtkXMLDataElement* ePrimary, double origin[3], double* &spacing);

  char *InputFileName;
  char *OutputFileName;
  char *FilePath;
  vtkSetStringMacro(FilePath);

private:
  vtkXMLHierarchicalBoxDataFileConverter(const vtkXMLHierarchicalBoxDataFileConverter&); // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataFileConverter&); // Not implemented.
//ETX
};

#endif
