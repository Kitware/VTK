// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkIOSSReaderInternal.h"

#include "vtkCellAttribute.h" // For CellTypeInfo.

VTK_ABI_NAMESPACE_BEGIN

class vtkDGCell;
class vtkIOSSCellGridReader;

/**
 * @class vtkIOSSCellGridReaderInternal
 * @brief Internal methods for the cell-grid version of the IOSS reader.
 *
 * Note that this class is not part of the public API of VTK and thus
 * has no export macros.
 */
class vtkIOSSCellGridReaderInternal : public vtkIOSSReaderInternal
{
public:
  vtkIOSSCellGridReaderInternal(vtkIOSSCellGridReader* self);

  std::vector<vtkSmartPointer<vtkCellGrid>> GetCellGrids(const std::string& blockName,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSCellGridReader* self);

  std::vector<vtkSmartPointer<vtkCellGrid>> GetElementBlock(const std::string& blockName,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSCellGridReader* self);

  std::vector<vtkSmartPointer<vtkCellGrid>> GetSideSet(const std::string& blockName,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSCellGridReader* self);

  std::vector<vtkSmartPointer<vtkCellGrid>> GetNodeSet(const std::string& blockName,
    vtkIOSSReader::EntityType vtk_entity_type, const DatabaseHandle& handle, int timestep,
    vtkIOSSCellGridReader* self);

  vtkCellAttribute::CellTypeInfo GetCellGridInfoForBlock(
    int shape_conn_size, int shape_order, vtkDGCell* dg);

  void GetNodalAttributes(vtkDataArraySelection* fieldSelection, vtkDataSetAttributes* arrayGroup,
    vtkCellGrid* grid, vtkDGCell* meta, Ioss::GroupingEntity* group_entity, Ioss::Region* region,
    const DatabaseHandle& handle, int timestep, bool read_ioss_ids,
    const std::string& cache_key_suffix);

  void GetElementAttributes(vtkDataArraySelection* fieldSelection, vtkDataSetAttributes* arrayGroup,
    vtkCellGrid* grid, vtkDGCell* meta, Ioss::GroupingEntity* group_entity, Ioss::Region* region,
    const DatabaseHandle& handle, int timestep, bool read_ioss_ids,
    const std::string& cache_key_suffix);
};

VTK_ABI_NAMESPACE_END
