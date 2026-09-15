// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableProbe
 * @brief   Probe a data array from a vtkTable.
 *
 * vtkTableProbe interpolates a data array from a Source vtkTable to its Input vtkDataObject.
 *
 * The chosen columns of the Source can be seen as the axes of a N-dimension space.
 * Another column is the data of interest, that will be probed.
 * Each element in the Input can be seen as a random point in this space.
 * Thus the array names used as coordinates should be found in both the Source table and in the
 * Input data object.
 *
 * Then a probe is done in this space for each element of the Input, doing linear interpolation.
 * The output is a copy of Input, with the probed array added.
 *
 * This is similar to the `vtkProbeFilter` family as it uses input coordinates
 * to pick values in source object.
 * But unlike most of VTK filters, this works in a N-dimension space instead
 * of the usual 3D one.
 *
 * # Example in a 2D space
 *
 * Lets have this table:
 * ```
 * A, B, C
 * 0, 1, 4
 * 1, 1, 5
 * 1, 0, 6
 * 0, 0, 7
 * 1, 2, 8
 * 2, 2, 9
 * ```
 *
 * Using A and B as coordinates array, and C as a an associated Field, we can construct
 * a structured space in 2D, like:
 *
 *  x --- 8 --- 9
 *  |     |     |
 *  |     |     |
 *  4 --- 5 --- x
 *  |     |     |
 *  |     |     |
 *  7 --- 6 --- x
 *
 * Where numerical values are the C column and x represents missing input points.
 *
 * Then, lets have a vtkPolyData with a PointData containing 2 arrays:
 *
 * ```
 * A, B
 * 0, 0
 * 0, 0.5
 * 0.5, 0.5
 * ```
 *
 * The vtkTableProbe filter can be used to add a `C` point data array to this polydata with this
 * pseudo-code:
 *
 * ```c++
 * vtkNew<vtkTableProbe> tableProbe;
 * tableProbe->SetSourceConnection(tableProvider->GetOutputPort());
 * tableProbe->GetCoordinatesArray()->AddArray("X", true);
 * tableProbe->GetCoordinatesArray()->AddArray("Y", true);
 * tableProbe->SetInputArrayToProcess("C", vtkDataObject::ROW);
 * tableProbe->SetInputConnection(polydataProvider->GetOutputPort());
 * tableProbe->SetAttributeToProcess(vtkDataObject::POINT);
 * ```
 *
 * The output polydata will contains
 *
 * ```
 * A,   B,   C
 * 0,   0,   7
 * 0,   0.5, 6.5
 * 0.5, 0.5, 5.5
 * ```
 * Values are interpolated from the previous Grid, picking each (A,B) coordinate.
 *
 *
 * @warning This filter makes no strong assumption on the input vtkTable nor on the vtkDataObject:
 * - the table may not contain every point of the defined space,
 * - a probed point may be outside of the table bounds. In that case, the output
 * data array will contain a `NaN` value.
 *
 * @see vtkTableToRectilinearGrid, vtkProbeFilter, vtkImageProbeFilter
 */

#ifndef vtkTableProbe_h
#define vtkTableProbe_h

#include "vtkDataObject.h"           // for attribute types enum
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkDataSetAttributes;

class VTKFILTERSGENERAL_EXPORT vtkTableProbe : public vtkPassInputTypeAlgorithm
{
public:
  static vtkTableProbe* New();
  vtkTypeMacro(vtkTableProbe, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the connection for the Source vtkTable, to define the N-dimensional space.
   */
  void SetSourceConnection(vtkAlgorithmOutput* input);

  /**
   * Set the attribute type to use in input vtkDataObject.
   * The output data array will be added to the corresponding vtkDataSetAttribute.
   */
  vtkSetClampMacro(
    AttributeToProcess, int, vtkDataObject::POINT, vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES - 1);
  vtkGetMacro(AttributeToProcess, int);

  /**
   * The list of vtkDataArray to use as coordinates.
   * They should exist in both the vtkTable and the vtkDataObject dataset attribute.
   */
  vtkGetMacro(CoordinatesArray, vtkDataArraySelection*);

  /**
   * Get the MTime of the filter.
   * Take CoordinatesArray into account.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkTableProbe();
  ~vtkTableProbe() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Add a second input with vtkTable class requirement.
   */
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkTableProbe(const vtkTableProbe&) = delete;
  void operator=(const vtkTableProbe&) = delete;

  /**
   * Fill the filtered dataset attributes with the source coordinates arrays
   * that are enabled in CoordinatesArray.
   * Do not add the explicitly excludedField. This is here to ensure that the
   * probed field is not part of the coordinates space.
   */
  void GetCoordinatesArray(
    vtkDataSetAttributes* source, vtkDataSetAttributes* filtered, const std::string& excludedField);

  /**
   * Retrieve source array in the input attributes.
   */
  vtkDataArray* GetInputArrayFromSource(
    vtkDataSetAttributes* inputAttributes, vtkDataArray* sourceArray);

  int AttributeToProcess = vtkDataObject::POINT;

  vtkNew<vtkDataArraySelection> CoordinatesArray;
};

VTK_ABI_NAMESPACE_END
#endif
