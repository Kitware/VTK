/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkOpenGLGL2PSExporter - OpenGL2 implementation of GL2PS exporter.
//
// .SECTION Description
// Implementation of vtkGL2PSExporter for the OpenGL2 backend.

#ifndef vtkOpenGLGL2PSExporter_h
#define vtkOpenGLGL2PSExporter_h

#include "vtkIOExportOpenGL2Module.h" // For export macro
#include "vtkGL2PSExporter.h"

class VTKIOEXPORTOPENGL2_EXPORT vtkOpenGLGL2PSExporter: public vtkGL2PSExporter
{
public:
  static vtkOpenGLGL2PSExporter *New();
  vtkTypeMacro(vtkOpenGLGL2PSExporter, vtkGL2PSExporter)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkOpenGLGL2PSExporter();
  ~vtkOpenGLGL2PSExporter();

  void WriteData();

private:
  vtkOpenGLGL2PSExporter(const vtkOpenGLGL2PSExporter &); // Not implemented.
  void operator=(const vtkOpenGLGL2PSExporter &);   // Not implemented.
};

#endif // vtkOpenGLGL2PSExporter_h
