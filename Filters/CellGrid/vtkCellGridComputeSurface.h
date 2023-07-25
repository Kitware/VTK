// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridComputeSurface
 * @brief   Adds arrays holding tuples of sides that are shared an odd number of times.
 *
 * This filter simply adds or replaces a 2-component array for each type of
 * side, for each cell type which identifies the set sides which are "un-shared."
 * Internally, it uses a vtkCellGridSidesQuery to obtain sides, so
 * the cells in your vtkCellGrid must provide a responder for this query type.
 *
 * @sa vtkCellGridSidesQuery
 */
#ifndef vtkCellGridComputeSurface_h
#define vtkCellGridComputeSurface_h

#include "vtkCellGridAlgorithm.h"
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridSidesQuery;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridComputeSurface : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridComputeSurface* New();
  vtkTypeMacro(vtkCellGridComputeSurface, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkStringToken GetSideAttribute();

protected:
  vtkCellGridComputeSurface() = default;
  ~vtkCellGridComputeSurface() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<vtkCellGridSidesQuery> Request;

private:
  vtkCellGridComputeSurface(const vtkCellGridComputeSurface&) = delete;
  void operator=(const vtkCellGridComputeSurface&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridComputeSurface_h
