// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2006 Atamai, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMNIObjectReader
 * @brief   A reader for MNI surface mesh files.
 *
 * The MNI .obj file format is used to store geometrical data.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Only polygon and line files are supported by this reader, but for
 * those formats, all data elements are read including normals, colors,
 * and surface properties.  ASCII and binary file types are supported.
 * @sa
 * vtkMINCImageReader vtkMNIObjectWriter vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
 */

#ifndef vtkMNIObjectReader_h
#define vtkMNIObjectReader_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkProperty;
class vtkPolyData;
class vtkFloatArray;
class vtkIntArray;
class vtkPoints;
class vtkCellArray;

class VTKIOMINC_EXPORT vtkMNIObjectReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMNIObjectReader, vtkPolyDataAlgorithm);

  static vtkMNIObjectReader* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the file name.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Get the extension for this file format.
   */
  virtual const char* GetFileExtensions() { return ".obj"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() { return "MNI object"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* name);

  /**
   * Get the property associated with the object.
   */
  virtual vtkProperty* GetProperty() { return this->Property; }

protected:
  vtkMNIObjectReader();
  ~vtkMNIObjectReader() override;

  char* FileName;
  vtkProperty* Property;
  int FileType;

  istream* InputStream;
  int LineNumber;
  char* LineText;
  char* CharPointer;

  int ReadLine(char* text, unsigned int length);
  int SkipWhitespace();
  int ParseValues(vtkDataArray* array, vtkIdType n);
  int ParseIdValue(vtkIdType* value);

  int ReadNumberOfPoints(vtkIdType* numCells);
  int ReadNumberOfCells(vtkIdType* numCells);
  int ReadProperty(vtkProperty* property);
  int ReadLineThickness(vtkProperty* property);
  int ReadPoints(vtkPolyData* polyData, vtkIdType numPoints);
  int ReadNormals(vtkPolyData* polyData, vtkIdType numPoints);
  int ReadColors(vtkProperty* property, vtkPolyData* data, vtkIdType numPoints, vtkIdType numCells);
  int ReadCells(vtkPolyData* data, vtkIdType numCells, int cellType);

  int ReadPolygonObject(vtkPolyData* output);
  int ReadLineObject(vtkPolyData* output);

  virtual int ReadFile(vtkPolyData* output);

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

private:
  vtkMNIObjectReader(const vtkMNIObjectReader&) = delete;
  void operator=(const vtkMNIObjectReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
