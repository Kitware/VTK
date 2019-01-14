/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporterFIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkX3DExporterFIWriter
 *
*/

#ifndef vtkX3DExporterFIWriter_h
#define vtkX3DExporterFIWriter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkX3DExporterWriter.h"

class vtkX3DExporterFIByteWriter;
class vtkX3DExporterFINodeInfoStack;
class vtkZLibDataCompressor;

class VTKIOEXPORT_EXPORT vtkX3DExporterFIWriter : public vtkX3DExporterWriter
{
public:
  static vtkX3DExporterFIWriter *New();
  vtkTypeMacro(vtkX3DExporterFIWriter, vtkX3DExporterWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CloseFile() override;
  int OpenFile(const char* file) override;
  int OpenStream() override;

  //void Write(const char* str);

  void Flush() override;

  void StartDocument() override;
  void EndDocument() override;

  // Elements
  void StartNode(int elementID) override;
  void EndNode() override;

  // Attributes
  // SFString / MFString
  //void SetField(int attributeID, const std::string &value);
  void SetField(int attributeID, const char*, bool mfstring = false) override;
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

  // MFInt32
  void SetField(int attributeID, int type, vtkCellArray* a);
  void SetField(int attributeID, const int* values, size_t size, bool image = false) override;

  //@{
  /**
   * Use fastest instead of best compression
   */
  vtkSetClampMacro(Fastest, vtkTypeBool, 0, 1);
  vtkBooleanMacro(Fastest, vtkTypeBool);
  vtkGetMacro(Fastest, vtkTypeBool);
  //@}

protected:
  vtkX3DExporterFIWriter();
  ~vtkX3DExporterFIWriter() override;

private:
  void StartAttribute(int attributeID, bool literal, bool addToTable = false);
  void EndAttribute();

  void CheckNode(bool callerIsAttribute = true);
  bool IsLineFeedEncodingOn;

  //int Depth;
  vtkX3DExporterFIByteWriter* Writer;
  vtkX3DExporterFINodeInfoStack* InfoStack;
  vtkZLibDataCompressor* Compressor;

  vtkTypeBool Fastest;

  vtkX3DExporterFIWriter(const vtkX3DExporterFIWriter&) = delete;
  void operator=(const vtkX3DExporterFIWriter&) = delete;

};

#endif

