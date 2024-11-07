// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkConduitArrayUtilities
 * @brief helper to convert Conduit arrays to VTK arrays.
 * @ingroup Insitu
 *
 * vtkConduitArrayUtilities is intended to convert Conduit nodes satisfying the
 * `mcarray` protocol to VTK arrays. It uses zero-copy, as much as possible.
 * Currently implementation fails if zero-copy is not possible. In future, that
 * may be changed to do a deep-copy (with appropriate warnings) if necessary.
 *
 * This is primarily designed for use by vtkConduitSource.
 */

#ifndef vtkConduitArrayUtilities_h
#define vtkConduitArrayUtilities_h

#include "vtkIOCatalystConduitModule.h" // for exports
#include "vtkObject.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include "conduit.h" // for conduit_node

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;

class VTKIOCATALYSTCONDUIT_EXPORT vtkConduitArrayUtilities : public vtkObject
{
public:
  static vtkConduitArrayUtilities* New();
  vtkTypeMacro(vtkConduitArrayUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns a vtkDataArray from a conduit node in the conduit mcarray protocol.
   */
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArray(const conduit_node* mcarray);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArray(
    const conduit_node* mcarray, const std::string& arrayname);
  ///@}

  ///@{
  /**
   * Returns a vtkDataArray from a conduit node in the conduit mcarray protocol
   * that is a conduit ghost array named ascent_ghosts.
   */
  static vtkSmartPointer<vtkDataArray> MCGhostArrayToVTKGhostArray(
    const conduit_node* mcarray, bool is_cell_data);
  ///@}

  /**
   * Converts an mcarray to vtkCellArray.
   *
   * This may reinterpret unsigned array as signed arrays to avoid deep-copying
   * of data to match data type expected by vtkCellArray API.
   */
  static vtkSmartPointer<vtkCellArray> MCArrayToVTKCellArray(
    vtkIdType cellSize, const conduit_node* mcarray);

  /**
   * If the number of components in the array does not match the target, a new
   * array is created.
   */
  static vtkSmartPointer<vtkDataArray> SetNumberOfComponents(
    vtkDataArray* array, int num_components);

  /**
   * Read a O2MRelation element
   */
  static vtkSmartPointer<vtkCellArray> O2MRelationToVTKCellArray(
    const conduit_node* o2mrelation, const std::string& leafname);

protected:
  vtkConduitArrayUtilities();
  ~vtkConduitArrayUtilities() override;

  static vtkSmartPointer<vtkDataArray> MCArrayToVTKArrayImpl(
    const conduit_node* mcarray, bool force_signed);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKAOSArray(
    const conduit_node* mcarray, bool force_signed);
  static vtkSmartPointer<vtkDataArray> MCArrayToVTKSOAArray(
    const conduit_node* mcarray, bool force_signed);

private:
  vtkConduitArrayUtilities(const vtkConduitArrayUtilities&) = delete;
  void operator=(const vtkConduitArrayUtilities&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
