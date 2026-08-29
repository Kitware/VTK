// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUSDExporter
 * @brief   export a scene into USD format.
 *
 * vtkUSDExporter is a concrete subclass of vtkExporter that writes USD
 * files.
 *
 * USD files contain a scene description that includes view, light, and camera parameters.
 *
 * Limitations:
 *
 * * Exporting textures from vtkMappers with scalar visibility on is supported, but only
 * when the ColorMode is set to VTK_COLOR_MODE_MAP_SCALARS.
 *
 * @sa
 * vtkExporter
 */

#ifndef vtkUSDExporter_h
#define vtkUSDExporter_h

#include "vtkExporter.h"
#include "vtkIOUSDModule.h" // For export macro

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkUSDExporterInternals;

class VTKIOUSD_EXPORT vtkUSDExporter : public vtkExporter
{
public:
  static vtkUSDExporter* New();
  vtkTypeMacro(vtkUSDExporter, vtkExporter);
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
   * accumulate into a single UsdStage as USD time samples (keyed off
   * TimeValue) instead of each call producing an independent file. Call
   * Finish() once the last frame has been written to save and close the
   * stage.
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
   * Save and close the stage accumulated since Start(). Has no effect if
   * Start() was not called.
   */
  void Finish();

protected:
  vtkUSDExporter();
  ~vtkUSDExporter() override;

private:
  vtkUSDExporter(const vtkUSDExporter&) = delete;
  void operator=(const vtkUSDExporter&) = delete;

  void WriteData() override;

  /**
   * Name of the USD file to write.
   */
  char* FileName;

  double TimeValue = 0.0;
  bool Started = false;
  vtkUSDExporterInternals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
