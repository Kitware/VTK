// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridPointProbe
 * @brief   Probe a vtkCellGrid with points.
 *
 * Given the following
 * + an input polydata, P;
 * + an input cell-grid, C; and
 * + optionally, a cell-attribute, A, of input C
 * produce an output polydata containing the
 * points from P that lie inside C and have values of A interpolated to them.
 *
 * Note that points of P may be repeated in the output if they are contained
 * in multiple cells of C (i.e., because multiple cells overlap some points).
 * This is most likely to occur at/near boundaries of cells in C.
 */

#ifndef vtkCellGridPointProbe_h
#define vtkCellGridPointProbe_h

#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkCellGridEvaluator;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridPointProbe : public vtkPolyDataAlgorithm
{
public:
  static vtkCellGridPointProbe* New();
  vtkTypeMacro(vtkCellGridPointProbe, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set the input cell-grid connection.
  ///
  /// This is used by ParaView. You may also simply
  /// call SetInputConnection(1, \a source) to obtain
  /// the same result.
  void SetSourceConnection(vtkAlgorithmOutput* source);

  /// Set/get the name of the generated vtkCellAttribute to interpolate.
  ///
  /// If no value is provided, then no interpolation will be performed.
  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);

protected:
  vtkCellGridPointProbe();
  ~vtkCellGridPointProbe() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<vtkCellGridEvaluator> Request;
  char* AttributeName{ nullptr };

private:
  vtkCellGridPointProbe(const vtkCellGridPointProbe&) = delete;
  void operator=(const vtkCellGridPointProbe&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridPointProbe_h
