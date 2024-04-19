// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetToDataObjectFilter
 * @brief   map dataset into data object (i.e., a field)
 *
 * vtkDataSetToDataObjectFilter is an class that transforms a dataset into
 * data object (i.e., a field). The field will have labeled data arrays
 * corresponding to the topology, geometry, field data, and point and cell
 * attribute data.
 *
 * You can control what portions of the dataset are converted into the
 * output data object's field data. The instance variables Geometry,
 * Topology, FieldData, PointData, and CellData are flags that control
 * whether the dataset's geometry (e.g., points, spacing, origin);
 * topology (e.g., cell connectivity, dimensions); the field data
 * associated with the dataset's superclass data object; the dataset's
 * point data attributes; and the dataset's cell data attributes. (Note:
 * the data attributes include scalars, vectors, tensors, normals, texture
 * coordinates, and field data.)
 *
 * The names used to create the field data are as follows. For vtkPolyData,
 * "Points", "Verts", "Lines", "Polys", and "Strips". For vtkUnstructuredGrid,
 * "Cells" and "CellTypes". For vtkStructuredPoints, "Dimensions", "Spacing",
 * and "Origin". For vtkStructuredGrid, "Points" and "Dimensions". For
 * vtkRectilinearGrid, "XCoordinates", "YCoordinates", and "ZCoordinates".
 * for point attribute data, "PointScalars", "PointVectors", etc. For cell
 * attribute data, "CellScalars", "CellVectors", etc. Field data arrays retain
 * their original name.
 *
 * @sa
 * vtkDataObject vtkFieldData vtkDataObjectToDataSetFilter
 */

#ifndef vtkDataSetToDataObjectFilter_h
#define vtkDataSetToDataObjectFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkDataSetToDataObjectFilter : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkDataSetToDataObjectFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate the object to transform all data into a data object.
   */
  static vtkDataSetToDataObjectFilter* New();

  ///@{
  /**
   * Turn on/off the conversion of dataset geometry to a data object.
   */
  vtkSetMacro(Geometry, vtkTypeBool);
  vtkGetMacro(Geometry, vtkTypeBool);
  vtkBooleanMacro(Geometry, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the conversion of dataset topology to a data object.
   */
  vtkSetMacro(Topology, vtkTypeBool);
  vtkGetMacro(Topology, vtkTypeBool);
  vtkBooleanMacro(Topology, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If LegacyTopology and Topology are both true, print out the legacy topology
   * arrays. Default is true.
   */
  vtkSetMacro(LegacyTopology, vtkTypeBool);
  vtkGetMacro(LegacyTopology, vtkTypeBool);
  vtkBooleanMacro(LegacyTopology, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If ModernTopology and Topology are both true, print out the modern topology
   * arrays. Default is true.
   */
  vtkSetMacro(ModernTopology, vtkTypeBool);
  vtkGetMacro(ModernTopology, vtkTypeBool);
  vtkBooleanMacro(ModernTopology, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the conversion of dataset field data to a data object.
   */
  vtkSetMacro(FieldData, vtkTypeBool);
  vtkGetMacro(FieldData, vtkTypeBool);
  vtkBooleanMacro(FieldData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the conversion of dataset point data to a data object.
   */
  vtkSetMacro(PointData, vtkTypeBool);
  vtkGetMacro(PointData, vtkTypeBool);
  vtkBooleanMacro(PointData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the conversion of dataset cell data to a data object.
   */
  vtkSetMacro(CellData, vtkTypeBool);
  vtkGetMacro(CellData, vtkTypeBool);
  vtkBooleanMacro(CellData, vtkTypeBool);
  ///@}

protected:
  vtkDataSetToDataObjectFilter();
  ~vtkDataSetToDataObjectFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**,
    vtkInformationVector*) override; // generate output data
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int, vtkInformation*) override;

  vtkTypeBool Geometry;
  vtkTypeBool Topology;
  vtkTypeBool LegacyTopology;
  vtkTypeBool ModernTopology;
  vtkTypeBool PointData;
  vtkTypeBool CellData;
  vtkTypeBool FieldData;

private:
  vtkDataSetToDataObjectFilter(const vtkDataSetToDataObjectFilter&) = delete;
  void operator=(const vtkDataSetToDataObjectFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
