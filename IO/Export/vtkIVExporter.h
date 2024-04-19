// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIVExporter
 * @brief   export a scene into OpenInventor 2.0 format.
 *
 * vtkIVExporter is a concrete subclass of vtkExporter that writes
 * OpenInventor 2.0 files.
 *
 * @sa
 * vtkExporter
 */

#ifndef vtkIVExporter_h
#define vtkIVExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;
class vtkActor;
class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;

class VTKIOEXPORT_EXPORT vtkIVExporter : public vtkExporter
{
public:
  static vtkIVExporter* New();
  vtkTypeMacro(vtkIVExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the OpenInventor file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkIVExporter();
  ~vtkIVExporter() override;

  void WriteData() override;
  void WriteALight(vtkLight* aLight, FILE* fp);
  void WriteAnActor(vtkActor* anActor, FILE* fp);
  void WritePointData(vtkPoints* points, vtkDataArray* normals, vtkDataArray* tcoords,
    vtkUnsignedCharArray* colors, FILE* fp);
  char* FileName;

private:
  vtkIVExporter(const vtkIVExporter&) = delete;
  void operator=(const vtkIVExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
