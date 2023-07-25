// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNIObjectWriter
 * @brief   A writer for MNI surface mesh files.
 *
 * The MNI .obj file format is used to store geometrical data.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Only polygon and line files are supported by this writer.  For these
 * formats, all data elements are written including normals, colors,
 * and surface properties.  ASCII and binary file types are supported.
 * @sa
 * vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
 */

#ifndef vtkMNIObjectWriter_h
#define vtkMNIObjectWriter_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMapper;
class vtkProperty;
class vtkLookupTable;
class vtkPolyData;
class vtkFloatArray;
class vtkIntArray;
class vtkPoints;

class VTKIOMINC_EXPORT vtkMNIObjectWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkMNIObjectWriter, vtkWriter);

  static vtkMNIObjectWriter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".obj"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI object"; }

  ///@{
  /**
   * Set the property associated with the object.  Optional.
   * This is useful for exporting an actor.
   */
  virtual void SetProperty(vtkProperty* property);
  virtual vtkProperty* GetProperty() { return this->Property; }
  ///@}

  ///@{
  /**
   * Set the mapper associated with the object.  Optional.
   * This is useful for exporting an actor with the same colors
   * that are used to display the actor within VTK.
   */
  virtual void SetMapper(vtkMapper* mapper);
  virtual vtkMapper* GetMapper() { return this->Mapper; }
  ///@}

  ///@{
  /**
   * Set the lookup table associated with the object.  This will be
   * used to convert scalar values to colors, if a mapper is not set.
   */
  virtual void SetLookupTable(vtkLookupTable* table);
  virtual vtkLookupTable* GetLookupTable() { return this->LookupTable; }
  ///@}

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);
  ///@}

  ///@{
  /**
   * Specify file name of vtk polygon data file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify file type (ASCII or BINARY) for vtk data file.
   */
  vtkSetClampMacro(FileType, int, VTK_ASCII, VTK_BINARY);
  vtkGetMacro(FileType, int);
  void SetFileTypeToASCII() { this->SetFileType(VTK_ASCII); }
  void SetFileTypeToBinary() { this->SetFileType(VTK_BINARY); }
  ///@}

protected:
  vtkMNIObjectWriter();
  ~vtkMNIObjectWriter() override;

  vtkProperty* Property;
  vtkMapper* Mapper;
  vtkLookupTable* LookupTable;

  ostream* OutputStream;

  int WriteObjectType(int objType);
  int WriteValues(vtkDataArray* array);
  int WriteIdValue(vtkIdType value);
  int WriteNewline();

  int WriteProperty(vtkProperty* property);
  int WriteLineThickness(vtkProperty* property);
  int WritePoints(vtkPolyData* polyData);
  int WriteNormals(vtkPolyData* polyData);
  int WriteColors(vtkProperty* property, vtkMapper* mapper, vtkPolyData* data);
  int WriteCells(vtkPolyData* data, int cellType);

  int WritePolygonObject(vtkPolyData* output);
  int WriteLineObject(vtkPolyData* output);

  void WriteData() override;

  char* FileName;

  int FileType;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  ostream* OpenFile();
  void CloseFile(ostream* fp);

private:
  vtkMNIObjectWriter(const vtkMNIObjectWriter&) = delete;
  void operator=(const vtkMNIObjectWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
