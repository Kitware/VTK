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

class vtkX3DExporterXMLNodeInfoStack;

class VTKIOEXPORT_EXPORT vtkX3DExporterXMLWriter : public vtkX3DExporterWriter
{

public:
  static vtkX3DExporterXMLWriter *New();
  vtkTypeMacro(vtkX3DExporterXMLWriter, vtkX3DExporterWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void CloseFile() VTK_OVERRIDE;
  int OpenFile(const char* file) VTK_OVERRIDE;
  void Flush() VTK_OVERRIDE;

  int OpenStream() VTK_OVERRIDE;

  void StartDocument() VTK_OVERRIDE;
  void EndDocument() VTK_OVERRIDE;

  // Elements
  void StartNode(int elementID) VTK_OVERRIDE;
  void EndNode() VTK_OVERRIDE;

  // Attributes
  // SFString / MFString
  void SetField(int attributeID, const char*, bool mfstring = true) VTK_OVERRIDE;
  // SFInt32
  void SetField(int attributeID, int) VTK_OVERRIDE;
  // SFFloat
  void SetField(int attributeID, float) VTK_OVERRIDE;
  // SFDouble
  void SetField(int attributeID, double) VTK_OVERRIDE;
  // SFBool
  void SetField(int attributeID, bool) VTK_OVERRIDE;

  // For MFxxx attributes
  void SetField(int attributeID, int type, const double* a) VTK_OVERRIDE;
  void SetField(int attributeID, int type, vtkDataArray* a) VTK_OVERRIDE;
  void SetField(int attributeID, const double* values, size_t size) VTK_OVERRIDE;
  // MFInt32, SFIMAGE
  void SetField(int attributeID, const int* values, size_t size, bool image = false) VTK_OVERRIDE;

protected:
  vtkX3DExporterXMLWriter();
  ~vtkX3DExporterXMLWriter() VTK_OVERRIDE;

private:

  const char* GetNewline() { return "\n"; };
  void AddDepth();
  void SubDepth();

  std::string ActTab;
  int Depth;
  ostream *OutputStream;
  vtkX3DExporterXMLNodeInfoStack* InfoStack;

  vtkX3DExporterXMLWriter(const vtkX3DExporterXMLWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkX3DExporterXMLWriter&) VTK_DELETE_FUNCTION;

};

#endif

