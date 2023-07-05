// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMemoryLimitImageDataStreamer
 * @brief   Initiates streaming on image data.
 *
 * To satisfy a request, this filter calls update on its input
 * many times with smaller update extents.  All processing up stream
 * streams smaller pieces.
 */

#ifndef vtkMemoryLimitImageDataStreamer_h
#define vtkMemoryLimitImageDataStreamer_h

#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkImageDataStreamer.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLELIMAGING_EXPORT vtkMemoryLimitImageDataStreamer : public vtkImageDataStreamer
{
public:
  static vtkMemoryLimitImageDataStreamer* New();
  vtkTypeMacro(vtkMemoryLimitImageDataStreamer, vtkImageDataStreamer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / Get the memory limit in kibibytes (1024 bytes).
   */
  vtkSetMacro(MemoryLimit, unsigned long);
  vtkGetMacro(MemoryLimit, unsigned long);
  ///@}

  // See the vtkAlgorithm for a description of what these do
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkMemoryLimitImageDataStreamer();
  ~vtkMemoryLimitImageDataStreamer() override = default;

  unsigned long MemoryLimit;

private:
  vtkMemoryLimitImageDataStreamer(const vtkMemoryLimitImageDataStreamer&) = delete;
  void operator=(const vtkMemoryLimitImageDataStreamer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
