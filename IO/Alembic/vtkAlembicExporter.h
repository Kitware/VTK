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

  /**
   * Begin a multi-frame export. Once called, consecutive calls to Write()
   * accumulate into a single Alembic archive as time samples (keyed off
   * TimeValue) instead of each call producing an independent file. Call
   * Finish() once the last frame has been written to save and close the
   * archive.
   */
  void Start();

  ///@{
  /**
   * Time value, in seconds, used for the time sample authored by the next
   * Write() call. Only meaningful between calls to Start() and Finish().
   */
  vtkSetMacro(TimeValue, double);
  vtkGetMacro(TimeValue, double);
  ///@}

  /**
   * Save and close the archive accumulated since Start(). Has no effect if
   * Start() was not called.
   */
  void Finish();

protected:
  vtkAlembicExporter();
  ~vtkAlembicExporter() override;

  void WriteData() override;

  char* FileName;

  double TimeValue = 0.0;
  bool Started = false;

private:
  vtkAlembicExporter(const vtkAlembicExporter&) = delete;
  void operator=(const vtkAlembicExporter&) = delete;

  class vtkAlembicExporterInternals;
  vtkAlembicExporterInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
