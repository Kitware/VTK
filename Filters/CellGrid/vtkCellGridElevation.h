// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridElevation
 * @brief   Adds a cell attribute representing elevation.
 *
 * This filter adds a new cell attribute – named "Elevation" by default –
 * to an input vtkCellGrid. The cell attribute is scalar-valued and
 * generally represents distance from some point along one or more axes.
 *
 * In order to make the attribute more interesting for demonstration
 * purposes, an additional "shock" parameter can be used by responders
 * to introduce discontinuities in the attribute at cell boundaries
 * (for cells which allow discontinuities such as vtkDGCell).
 */

#ifndef vtkCellGridElevation_h
#define vtkCellGridElevation_h

#include "vtkCellGridAlgorithm.h"
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridElevationQuery;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridElevation : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridElevation* New();
  vtkTypeMacro(vtkCellGridElevation, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get the name of the generated vtkCellAttribute.
  ///
  /// The default is `elevation` if no value is provided.
  vtkSetStringMacro(AttributeName);
  vtkGetStringMacro(AttributeName);

  /// Set/get the location where the output scalar is zero.
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);

  /// Set/get the number of axes along which elevation is measured.
  /// This is a number between 1 and 3, inclusive.
  /// These correspond to:
  /// + 1 – **linear**: elevation is measured by projecting any test point
  ///       to \a Axis, then computing the distance to the \a Origin.
  /// + 2 – **cylindrical**: elevation is measured from the nearest point
  ///       along the line passing through the \a Origin along the \a Axis.
  ///       All points along line have an elevation of 0.
  /// + 3 – **spherical**: elevation is measured using the L² norm of the
  ///       vector from the \a Origin to each test point. The \a Axis is
  ///       ignored.
  ///
  /// The default is 1 (linear).
  vtkSetClampMacro(NumberOfAxes, int, 1, 3);
  vtkGetMacro(NumberOfAxes, int);

  /// Set/get the principal direction along which elevation is measured.
  /// The exact way the axis is used varies with the \a NumberOfAxes setting.
  vtkSetVector3Macro(Axis, double);
  vtkGetVector3Macro(Axis, double);

  /// Set/get the "shock" value, which is a distance added to each elevation
  /// value proportional to the distance from the cell center to the test
  /// point within that cell. The intent is to provide a way to introduce
  /// discontinuities into the field to demonstrate the capabilities of DG
  /// cells.
  vtkSetMacro(Shock, double);
  vtkGetMacro(Shock, double);

protected:
  vtkCellGridElevation() = default;
  ~vtkCellGridElevation() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<vtkCellGridElevationQuery> Request;
  double Origin[3] = { 0., 0., 0 };
  double Axis[3] = { 0., 0., 1. };
  double Shock{ 0. };
  int NumberOfAxes{ 1 };
  char* AttributeName{ nullptr };

private:
  vtkCellGridElevation(const vtkCellGridElevation&) = delete;
  void operator=(const vtkCellGridElevation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridElevation_h
