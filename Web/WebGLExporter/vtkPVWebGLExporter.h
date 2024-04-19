// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkPVWebGLExporter_h
#define vtkPVWebGLExporter_h

#include "vtkExporter.h"
#include "vtkWebGLExporterModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKWEBGLEXPORTER_EXPORT vtkPVWebGLExporter : public vtkExporter
{
public:
  static vtkPVWebGLExporter* New();
  vtkTypeMacro(vtkPVWebGLExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Specify the name of the VRML file to write.
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

protected:
  vtkPVWebGLExporter();
  ~vtkPVWebGLExporter() override;

  void WriteData() override;

  char* FileName;

private:
  vtkPVWebGLExporter(const vtkPVWebGLExporter&) = delete;
  void operator=(const vtkPVWebGLExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
