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

#ifndef vtkTransmitRectilinearGridPiece_h
#define vtkTransmitRectilinearGridPiece_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTransmitStructuredDataPiece.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkTransmitRectilinearGridPiece
  : public vtkTransmitStructuredDataPiece
{
public:
  static vtkTransmitRectilinearGridPiece* New();
  vtkTypeMacro(vtkTransmitRectilinearGridPiece, vtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTransmitRectilinearGridPiece();
  ~vtkTransmitRectilinearGridPiece() override;

private:
  vtkTransmitRectilinearGridPiece(const vtkTransmitRectilinearGridPiece&) = delete;
  void operator=(const vtkTransmitRectilinearGridPiece&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
