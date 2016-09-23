/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUniformGridAMRWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLUniformGridAMRWriter
 * @brief   writer for vtkUniformGridAMR.
 *
 * vtkXMLUniformGridAMRWriter is a vtkXMLCompositeDataWriter subclass to
 * handle vtkUniformGridAMR datasets (including vtkNonOverlappingAMR and
 * vtkOverlappingAMR).
*/

#ifndef vtkXMLUniformGridAMRWriter_h
#define vtkXMLUniformGridAMRWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

class VTKIOXML_EXPORT vtkXMLUniformGridAMRWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLUniformGridAMRWriter* New();
  vtkTypeMacro(vtkXMLUniformGridAMRWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the default file extension for files written by this writer.
   */
  virtual const char* GetDefaultFileExtension()
    { return "vth"; }

protected:
  vtkXMLUniformGridAMRWriter();
  ~vtkXMLUniformGridAMRWriter();

  /**
   * Methods to define the file's major and minor version numbers.
   * VTH/VTHB version number 1.1 is used for overlapping/non-overlapping AMR
   * datasets.
   */
  virtual int GetDataSetMajorVersion() { return 1; }
  virtual int GetDataSetMinorVersion() { return 1; }

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* parent, int &writerIdx);

private:
  vtkXMLUniformGridAMRWriter(const vtkXMLUniformGridAMRWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLUniformGridAMRWriter&) VTK_DELETE_FUNCTION;

};

#endif
