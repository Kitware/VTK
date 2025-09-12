// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellTypeUtilities_h
#define vtkCellTypeUtilities_h

#include "vtkCellType.h"              // For CellType enum
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

class VTKCOMMONDATAMODEL_EXPORT vtkCellTypeUtilities : public vtkObject
{
public:
  static vtkCellTypeUtilities* New();

  vtkTypeMacro(vtkCellTypeUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a display name for the given cell type,
   * or an empty string if the id is not recognized.
   */
  static std::string GetTypeAsString(int typeId);

  /**
   * Return a cell type (as defined in vtkCellType.h) from the given display name.
   * Return VTK_EMPTY_CELL if display name is not recognized.
   */
  static int GetTypeIdFromName(const std::string& name);

  /**
   * Given an int (as defined in vtkCellType.h) identifier for a class
   * return it's classname.
   */
  static const char* GetClassNameFromTypeId(int typeId);

  /**
   * Given a data object classname, return it's int identified (as
   * defined in vtkCellType.h)
   */
  static int GetTypeIdFromClassName(const char* classname);

  /**
   * This convenience method is a fast check to determine if a cell type
   * represents a linear or nonlinear cell.  This is generally much more
   * efficient than getting the appropriate vtkCell and checking its IsLinear
   * method.
   */
  static int IsLinear(unsigned char type);

  /**
   * Get the dimension of a cell.
   */
  static int GetDimension(unsigned char type);

protected:
  vtkCellTypeUtilities() = default;
  ~vtkCellTypeUtilities() override = default;

private:
  vtkCellTypeUtilities(const vtkCellTypeUtilities&) = delete;
  void operator=(const vtkCellTypeUtilities&) = delete;
};

inline int vtkCellTypeUtilities::IsLinear(unsigned char type)
{
  return (
    (type < VTK_HEXAGONAL_PRISM) || (type == VTK_CONVEX_POINT_SET) || (type == VTK_POLYHEDRON));
}

VTK_ABI_NAMESPACE_END
#endif
