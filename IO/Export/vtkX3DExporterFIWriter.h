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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void CloseFile() VTK_OVERRIDE;
  int OpenFile(const char* file) VTK_OVERRIDE;
  int OpenStream() VTK_OVERRIDE;

  //void Write(const char* str);

  void Flush() VTK_OVERRIDE;

  void StartDocument() VTK_OVERRIDE;
  void EndDocument() VTK_OVERRIDE;

  // Elements
  void StartNode(int elementID) VTK_OVERRIDE;
  void EndNode() VTK_OVERRIDE;

  // Attributes
  // SFString / MFString
  //void SetField(int attributeID, const std::string &value);
  void SetField(int attributeID, const char*, bool mfstring = false) VTK_OVERRIDE;
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

  // MFInt32
  void SetField(int attributeID, int type, vtkCellArray* a);
  void SetField(int attributeID, const int* values, size_t size, bool image = false) VTK_OVERRIDE;

  //@{
  /**
   * Use fastest instead of best compression
   */
  vtkSetClampMacro(Fastest, int, 0, 1);
  vtkBooleanMacro(Fastest, int);
  vtkGetMacro(Fastest, int);
  //@}

protected:
  vtkX3DExporterFIWriter();
  ~vtkX3DExporterFIWriter() VTK_OVERRIDE;

private:
  void StartAttribute(int attributeID, bool literal, bool addToTable = false);
  void EndAttribute();

  void CheckNode(bool callerIsAttribute = true);
  bool IsLineFeedEncodingOn;

  //int Depth;
  vtkX3DExporterFIByteWriter* Writer;
  vtkX3DExporterFINodeInfoStack* InfoStack;
  vtkZLibDataCompressor* Compressor;

  int Fastest;

  vtkX3DExporterFIWriter(const vtkX3DExporterFIWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkX3DExporterFIWriter&) VTK_DELETE_FUNCTION;

};

#endif

