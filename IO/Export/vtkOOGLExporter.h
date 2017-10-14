/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOOGLExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOOGLExporter
 * @brief   export a scene into Geomview OOGL format.
 *
 * vtkOOGLExporter is a concrete subclass of vtkExporter that writes
 * Geomview OOGL files.
 *
 * @sa
 * vtkExporter
*/

#ifndef vtkOOGLExporter_h
#define vtkOOGLExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkLight;
class vtkActor;

class VTKIOEXPORT_EXPORT vtkOOGLExporter : public vtkExporter
{
public:
  static vtkOOGLExporter *New();
  vtkTypeMacro(vtkOOGLExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the name of the Geomview file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

protected:
  vtkOOGLExporter();
  ~vtkOOGLExporter() override;

  void WriteData() override;
  void WriteALight(vtkLight *aLight, FILE *fp);
  void WriteAnActor(vtkActor *anActor, FILE *fp, int count);
  char *FileName;

private:
  vtkOOGLExporter(const vtkOOGLExporter&) = delete;
  void operator=(const vtkOOGLExporter&) = delete;
};

#endif

