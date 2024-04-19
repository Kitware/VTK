// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTransmitRectilinearGridPiece
 * @brief   Redistributes data produced
 * by serial readers
 *
 *
 * This filter can be used to redistribute data from producers that can't
 * produce data in parallel. All data is produced on first process and
 * the distributed to others using the multiprocess controller.
 *
 * Note that this class is legacy. The superclass does all the work and
 * can be used directly instead.
 */

#ifndef vtkTransmitImageDataPiece_h
#define vtkTransmitImageDataPiece_h

#include "vtkFiltersParallelImagingModule.h" // For export macro
#include "vtkTransmitStructuredDataPiece.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLELIMAGING_EXPORT vtkTransmitImageDataPiece
  : public vtkTransmitStructuredDataPiece
{
public:
  static vtkTransmitImageDataPiece* New();
  vtkTypeMacro(vtkTransmitImageDataPiece, vtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTransmitImageDataPiece();
  ~vtkTransmitImageDataPiece() override;

private:
  vtkTransmitImageDataPiece(const vtkTransmitImageDataPiece&) = delete;
  void operator=(const vtkTransmitImageDataPiece&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
