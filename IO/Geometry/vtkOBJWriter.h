/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOBJWriter
 * @brief   write wavefront obj file
 *
 * vtkOBJWriter writes wavefront obj (.obj) files in ASCII form.
 * OBJ files contain the geometry including lines, triangles and polygons.
 * Normals and texture coordinates on points are also written if they exist.
 * One can specify a texture passing a vtkImageData on port 1.
 * If a texture is set, additionals .mtl and .png files are generated. Those files have the same
 * name without obj extension.
 */

#ifndef vtkOBJWriter_h
#define vtkOBJWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

class vtkDataSet;
class vtkImageData;
class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkOBJWriter : public vtkWriter
{
public:
  static vtkOBJWriter* New();
  vtkTypeMacro(vtkOBJWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the inputs to this writer.
   */
  vtkPolyData* GetInputGeometry();
  vtkImageData* GetInputTexture();
  vtkDataSet* GetInput(int port);
  //@}

  //@{
  /**
   * Get/Set the file name of the OBJ file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkOBJWriter();
  ~vtkOBJWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  char* FileName;

private:
  vtkOBJWriter(const vtkOBJWriter&) = delete;
  void operator=(const vtkOBJWriter&) = delete;
};

#endif
