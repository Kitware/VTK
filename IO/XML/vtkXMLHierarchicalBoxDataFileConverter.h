// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLHierarchicalBoxDataFileConverter
 * @brief   converts older *.vth, *.vthb
 * files to newer format.
 *
 * vtkXMLHierarchicalBoxDataFileConverter is a utility class to convert v0.1 and
 * v1.0 of the VTK XML hierarchical file format to the v1.1. Users can then use
 * vtkXMLUniformGridAMRReader to read the dataset into VTK.
 */

#ifndef vtkXMLHierarchicalBoxDataFileConverter_h
#define vtkXMLHierarchicalBoxDataFileConverter_h

#include "vtkIOXMLModule.h" // needed for export macro.
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkXMLDataElement;

class VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataFileConverter : public vtkObject
{
public:
  static vtkXMLHierarchicalBoxDataFileConverter* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataFileConverter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the input filename.
   */
  vtkSetFilePathMacro(InputFileName);
  vtkGetFilePathMacro(InputFileName);
  ///@}

  ///@{
  /**
   * Set the output filename.
   */
  vtkSetFilePathMacro(OutputFileName);
  vtkGetFilePathMacro(OutputFileName);
  ///@}

  /**
   * Converts the input file to new format and writes out the output file.
   */
  bool Convert();

protected:
  vtkXMLHierarchicalBoxDataFileConverter();
  ~vtkXMLHierarchicalBoxDataFileConverter() override;

  vtkXMLDataElement* ParseXML(const char* filename);

  // Returns GridDescription. VTK_UNCHANGED for invalid/failure.
  int GetOriginAndSpacing(vtkXMLDataElement* ePrimary, double origin[3], double*& spacing);

  char* InputFileName;
  char* OutputFileName;
  char* FilePath;
  vtkSetFilePathMacro(FilePath);

private:
  vtkXMLHierarchicalBoxDataFileConverter(const vtkXMLHierarchicalBoxDataFileConverter&) = delete;
  void operator=(const vtkXMLHierarchicalBoxDataFileConverter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
