// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2007, Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
/**
 * @class   vtkPOVExporter
 * @brief   Export scene into povray format.
 *
 *
 * This Exporter can be attached to a render window in order to generate
 * scene description files for the Persistence of Vision Raytracer
 * www.povray.org.
 *
 * @par Thanks:
 * Li-Ta Lo (ollie@lanl.gov) and Jim Ahrens (ahrens@lanl.gov)
 * Los Alamos National Laboratory
 */

#ifndef vtkPOVExporter_h
#define vtkPOVExporter_h

#include "vtkExporter.h"
#include "vtkIOExportModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkActor;
class vtkCamera;
class vtkLight;
class vtkPolyData;
class vtkProperty;
class vtkTexture;
class vtkPOVInternals;

class VTKIOEXPORT_EXPORT vtkPOVExporter : public vtkExporter
{
public:
  static vtkPOVExporter* New();
  vtkTypeMacro(vtkPOVExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The filename to save into.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkPOVExporter();
  ~vtkPOVExporter() override;

  void WriteData() override;
  virtual void WriteHeader(vtkRenderer* renderer);
  void WriteCamera(vtkCamera* camera);
  void WriteLight(vtkLight* light);
  void WriteProperty(vtkProperty* property);
  void WritePolygons(vtkPolyData* polydata, bool scalar_visible);
  void WriteTriangleStrips(vtkPolyData* strip, bool scalar_visible);

  virtual void WriteActor(vtkActor* actor);

  char* FileName;
  FILE* FilePtr;

private:
  vtkPOVExporter(const vtkPOVExporter&) = delete;
  void operator=(const vtkPOVExporter&) = delete;

  vtkPOVInternals* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
