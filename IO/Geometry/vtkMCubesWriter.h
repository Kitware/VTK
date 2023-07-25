// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMCubesWriter
 * @brief   write binary marching cubes file
 *
 * vtkMCubesWriter is a polydata writer that writes binary marching cubes
 * files. (Marching cubes is an isosurfacing technique that generates many
 * triangles.) The binary format is supported by W. Lorensen's marching cubes
 * program (and the vtkSliceCubes object). Each triangle is represented by
 * three records, with each record consisting of six single precision
 * floating point numbers representing the a triangle vertex coordinate and
 * vertex normal.
 *
 * @warning
 * Binary files are written in sun/hp/sgi (i.e., Big Endian) form.
 *
 * @sa
 * vtkMarchingCubes vtkSliceCubes vtkMCubesReader
 */

#ifndef vtkMCubesWriter_h
#define vtkMCubesWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkPoints;
class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkMCubesWriter : public vtkWriter
{
public:
  static vtkMCubesWriter* New();
  vtkTypeMacro(vtkMCubesWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get file name of marching cubes limits file.
   */
  vtkSetFilePathMacro(LimitsFileName);
  vtkGetFilePathMacro(LimitsFileName);
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

protected:
  vtkMCubesWriter();
  ~vtkMCubesWriter() override;

  void WriteData() override;

  void WriteMCubes(FILE* fp, vtkPoints* pts, vtkDataArray* normals, vtkCellArray* polys);
  void WriteLimits(FILE* fp, double* bounds);

  char* LimitsFileName;

  char* FileName;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkMCubesWriter(const vtkMCubesWriter&) = delete;
  void operator=(const vtkMCubesWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
