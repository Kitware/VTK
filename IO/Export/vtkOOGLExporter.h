// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;
class vtkActor;

class VTKIOEXPORT_EXPORT vtkOOGLExporter : public vtkExporter
{
public:
  static vtkOOGLExporter* New();
  vtkTypeMacro(vtkOOGLExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the Geomview file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkOOGLExporter();
  ~vtkOOGLExporter() override;

  void WriteData() override;
  void WriteALight(vtkLight* aLight, FILE* fp);
  void WriteAnActor(vtkActor* anActor, FILE* fp, int count);
  char* FileName;

private:
  vtkOOGLExporter(const vtkOOGLExporter&) = delete;
  void operator=(const vtkOOGLExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
