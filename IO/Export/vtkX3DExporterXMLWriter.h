/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterXMLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkX3DExporterXMLWriter
 * @brief   X3D Exporter XML Writer
 *
 * vtkX3DExporterXMLWriter
*/

#ifndef vtkX3DExporterXMLWriter_h
#define vtkX3DExporterXMLWriter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkX3DExporterWriter.h"
#include <string> // for std::string

class vtkX3DExporterXMLNodeInfoStack;

class VTKIOEXPORT_EXPORT vtkX3DExporterXMLWriter : public vtkX3DExporterWriter
{

public:
  static vtkX3DExporterXMLWriter *New();
  vtkTypeMacro(vtkX3DExporterXMLWriter, vtkX3DExporterWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CloseFile() override;
  int OpenFile(const char* file) override;
  void Flush() override;

  int OpenStream() override;

  void StartDocument() override;
  void EndDocument() override;

  // Elements
  void StartNode(int elementID) override;
  void EndNode() override;

  // Attributes
  // SFString / MFString
  void SetField(int attributeID, const char*, bool mfstring = true) override;
  // SFInt32
  void SetField(int attributeID, int) override;
  // SFFloat
  void SetField(int attributeID, float) override;
  // SFDouble
  void SetField(int attributeID, double) override;
  // SFBool
  void SetField(int attributeID, bool) override;

  // For MFxxx attributes
  void SetField(int attributeID, int type, const double* a) override;
  void SetField(int attributeID, int type, vtkDataArray* a) override;
  void SetField(int attributeID, const double* values, size_t size) override;
  // MFInt32, SFIMAGE
  void SetField(int attributeID, const int* values, size_t size, bool image = false) override;

protected:
  vtkX3DExporterXMLWriter();
  ~vtkX3DExporterXMLWriter() override;

private:

  const char* GetNewline() { return "\n"; };
  void AddDepth();
  void SubDepth();

  std::string ActTab;
  int Depth;
  ostream *OutputStream;
  vtkX3DExporterXMLNodeInfoStack* InfoStack;

  vtkX3DExporterXMLWriter(const vtkX3DExporterXMLWriter&) = delete;
  void operator=(const vtkX3DExporterXMLWriter&) = delete;

};

#endif

