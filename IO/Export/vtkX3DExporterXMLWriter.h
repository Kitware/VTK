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
// .NAME vtkX3DExporterXMLWriter - X3D Exporter XML Writer
// .SECTION Description
// vtkX3DExporterXMLWriter

#ifndef __vtkX3DExporterXMLWriter_h
#define __vtkX3DExporterXMLWriter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkX3DExporterWriter.h"

class vtkX3DExporterXMLNodeInfoStack;

class VTKIOEXPORT_EXPORT vtkX3DExporterXMLWriter : public vtkX3DExporterWriter
{

public:
  static vtkX3DExporterXMLWriter *New();
  vtkTypeMacro(vtkX3DExporterXMLWriter, vtkX3DExporterWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CloseFile();
  virtual int OpenFile(const char* file);
  virtual void Flush();

  virtual int OpenStream();

  void StartDocument();
  void EndDocument();

  // Elements
  void StartNode(int elementID);
  void EndNode();

  // Attributes
  // SFString / MFString
  void SetField(int attributeID, const char*, bool mfstring = true);
  // SFInt32
  void SetField(int attributeID, int);
  // SFFloat
  void SetField(int attributeID, float);
  // SFDouble
  void SetField(int attributeID, double);
  // SFBool
  void SetField(int attributeID, bool);

  // For MFxxx attributes
  void SetField(int attributeID, int type, const double* a);
  void SetField(int attributeID, int type, vtkDataArray* a);
  void SetField(int attributeID, const double* values, size_t size);
  // MFInt32, SFIMAGE
  void SetField(int attributeID, const int* values, size_t size, bool image = false);

protected:
  vtkX3DExporterXMLWriter();
  ~vtkX3DExporterXMLWriter();

private:

  const char* GetNewline() { return "\n"; };
  void AddDepth();
  void SubDepth();

  std::string ActTab;
  int Depth;
  ostream *OutputStream;
  vtkX3DExporterXMLNodeInfoStack* InfoStack;

  vtkX3DExporterXMLWriter(const vtkX3DExporterXMLWriter&); // Not implemented.
  void operator=(const vtkX3DExporterXMLWriter&); // Not implemented.

};

#endif

