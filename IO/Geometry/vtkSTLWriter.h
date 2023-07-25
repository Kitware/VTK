// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSTLWriter
 * @brief   write stereo lithography files
 *
 * vtkSTLWriter writes stereo lithography (.stl) files in either ASCII or
 * binary form. Stereo lithography files contain only triangles. Since VTK 8.1,
 * this writer converts non-triangle polygons into triangles, so there is no
 * longer a need to use vtkTriangleFilter prior to using this writer if the
 * input contains polygons with more than three vertices.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.
 */

#ifndef vtkSTLWriter_h
#define vtkSTLWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkPoints;
class vtkPolyData;
class vtkUnsignedCharArray;

class VTKIOGEOMETRY_EXPORT vtkSTLWriter : public vtkWriter
{
public:
  static vtkSTLWriter* New();
  vtkTypeMacro(vtkSTLWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Set the header for the file as text. The header cannot contain 0x00 characters.
   * \sa SetBinaryHeader()
   */
  vtkSetStringMacro(Header);
  vtkGetStringMacro(Header);
  ///@}

  ///@{
  /**
   * Set binary header for the file.
   * Binary header is only used when writing binary type files.
   * If both Header and BinaryHeader are specified then BinaryHeader is used.
   * Maximum length of binary header is 80 bytes, any content over this limit is ignored.
   */
  virtual void SetBinaryHeader(vtkUnsignedCharArray* binaryHeader);
  vtkGetObjectMacro(BinaryHeader, vtkUnsignedCharArray);
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
  vtkSTLWriter();
  ~vtkSTLWriter() override;

  void WriteData() override;

  void WriteBinarySTL(vtkPoints* pts, vtkCellArray* polys, vtkCellArray* strips);
  void WriteAsciiSTL(vtkPoints* pts, vtkCellArray* polys, vtkCellArray* strips);

  char* FileName;
  char* Header;
  vtkUnsignedCharArray* BinaryHeader;
  int FileType;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkSTLWriter(const vtkSTLWriter&) = delete;
  void operator=(const vtkSTLWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
