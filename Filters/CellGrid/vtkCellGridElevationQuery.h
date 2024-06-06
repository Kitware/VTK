// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridElevationQuery
 * @brief   Request a new vtkCellAttribute corresponding to "elevation".
 */

#ifndef vtkCellGridElevationQuery_h
#define vtkCellGridElevationQuery_h

#include "vtkCellGridQuery.h"
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkNew.h"                   // For ivar.

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkCellAttribute;

/**\brief A cell-grid query for creating an "elevation" field.
 *
 * Initialize() prepares the \a Elevation ivar.
 * As responders process cell metadata, they should call
 * Elevation->SetCellTypeInfo() and update Range to
 * enclose all the elevation values they add.
 *
 * Finalize() may optionally set a colormap with the proper range.
 */
class VTKFILTERSCELLGRID_EXPORT vtkCellGridElevationQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridElevationQuery* New();
  vtkTypeMacro(vtkCellGridElevationQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Initialize() override;
  bool Finalize() override;

  std::string Name;
  double Shock{ 0. };
  int NumberOfAxes{ 1 };
  std::array<double, 3> Origin{ { 0., 0., 0. } };
  std::array<double, 3> Axis{ { 0., 0., 1. } };
  std::array<double, 2> Range{ { 1., 0. } };
  vtkNew<vtkCellAttribute> Elevation;

protected:
  vtkCellGridElevationQuery() = default;
  ~vtkCellGridElevationQuery() override = default;

private:
  vtkCellGridElevationQuery(const vtkCellGridElevationQuery&) = delete;
  void operator=(const vtkCellGridElevationQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridElevationQuery_h
