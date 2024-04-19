// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAlembicExporter
 * @brief   export a scene into Alembic format.
 *
 * vtkAlembicExporter is a concrete subclass of vtkExporter that writes Alembic
 * files.
 *
 * Alembic .abc files are a scene description, and include view and camera parameters.
 *
 * @sa
 * vtkExporter
 */

#ifndef vtkAlembicExporter_h
#define vtkAlembicExporter_h

#include "vtkExporter.h"
#include "vtkIOAlembicModule.h" // For export macro

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKIOALEMBIC_EXPORT vtkAlembicExporter : public vtkExporter
{
public:
  static vtkAlembicExporter* New();
  vtkTypeMacro(vtkAlembicExporter, vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkAlembicExporter();
  ~vtkAlembicExporter() override;

  void WriteData() override;

  char* FileName;

private:
  vtkAlembicExporter(const vtkAlembicExporter&) = delete;
  void operator=(const vtkAlembicExporter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
