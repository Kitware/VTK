/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMCubesWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkCellArray;
class vtkDataArray;
class vtkPoints;
class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkMCubesWriter : public vtkWriter
{
public:
  static vtkMCubesWriter *New();
  vtkTypeMacro(vtkMCubesWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set/get file name of marching cubes limits file.
   */
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);
  //@}

  //@{
  /**
   * Get the input to this writer.
   */
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);
  //@}

  //@{
  /**
   * Specify file name of vtk polygon data file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkMCubesWriter();
  ~vtkMCubesWriter();

  void WriteData();

  void WriteMCubes(FILE *fp, vtkPoints *pts, vtkDataArray *normals,
                   vtkCellArray *polys);
  void WriteLimits(FILE *fp, double *bounds);

  char *LimitsFileName;

  char *FileName;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkMCubesWriter(const vtkMCubesWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMCubesWriter&) VTK_DELETE_FUNCTION;
};

#endif


