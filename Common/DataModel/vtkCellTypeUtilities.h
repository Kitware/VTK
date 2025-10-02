// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * vtkCellTypeUtilities is a collection of methods for cell type lookup.
 *
 * It provides conversion between type id (as defined in vtkCellType.h),
 * class name and display name, as long as other informations like the cell dimension.
 *
 * @note for backward compatibility some method use an `int` as the cell type,
 * but it should be an unsigned char. See vtkCellType.h
 */
#ifndef vtkCellTypeUtilities_h
#define vtkCellTypeUtilities_h

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

VTK_ABI_NAMESPACE_END
#endif
