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
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CloseFile();
  virtual int OpenFile(const char* file);
  virtual int OpenStream();

  //void Write(const char* str);

  virtual void Flush();

  void StartDocument();
  void EndDocument();

  // Elements
  void StartNode(int elementID);
  void EndNode();

  // Attributes
  // SFString / MFString
  //void SetField(int attributeID, const std::string &value);
  void SetField(int attributeID, const char*, bool mfstring = false);
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

  // MFInt32
  void SetField(int attributeID, int type, vtkCellArray* a);
  void SetField(int attributeID, const int* values, size_t size, bool image = false);

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
  ~vtkX3DExporterFIWriter();

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

