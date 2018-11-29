/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGLTFExporter
 * @brief   export a scene into GLTF 2.0 format.
 *
 * vtkGLTFExporter is a concrete subclass of vtkExporter that writes GLTF 2.0
 * files. It currently only supports a very small subset of what VTK can do
 * including polygonal meshes with optional vertex colors. Over time the class
 * can be expanded to support more and more of what VTK renders.
 *
 * It should be noted that gltf is a format for rendering data. As such
 * it stores what the VTK scene renders as, not the underlying data. For
 * example it currently does not support quads or higher sided polygons
 * although VTK does. As such taking an exported gltf file and then selecting
 * wireframe in a viewer will give all triangles where VTK's rendering
 * would correctly draw the original polygons. etc.
 *
 * @sa
 * vtkExporter
*/

#ifndef vtkGLTFExporter_h
#define vtkGLTFExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class VTKIOEXPORT_EXPORT vtkGLTFExporter : public vtkExporter
{
public:
  static vtkGLTFExporter *New();
  vtkTypeMacro(vtkGLTFExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the name of the GLTF file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkGLTFExporter();
  ~vtkGLTFExporter() override;

  void WriteData() override;

  char *FileName;

private:
  vtkGLTFExporter(const vtkGLTFExporter&) = delete;
  void operator=(const vtkGLTFExporter&) = delete;
};

#endif
