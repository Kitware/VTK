// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRandomAttributeGenerator
 * @brief   generate and create random data attributes
 *
 * vtkRandomAttributeGenerator is a filter that creates random attributes
 * including scalars, vectors, normals, tensors, texture coordinates and/or
 * general data arrays. These attributes can be generated as point data, cell
 * data or general field data. The generation of each component is normalized
 * between a user-specified minimum and maximum value.
 *
 * This filter provides that capability to specify the data type of the
 * attributes, the range for each of the components, and the number of
 * components. Note, however, that this flexibility only goes so far because
 * some attributes (e.g., normals, vectors and tensors) are fixed in the
 * number of components, and in the case of normals and tensors, are
 * constrained in the values that some of the components can take (i.e.,
 * normals have magnitude one, and tensors are symmetric).
 *
 * @warning
 * In general this class is used for debugging or testing purposes.
 *
 * @warning
 * It is possible to generate multiple attributes simultaneously.
 *
 * @warning
 * By default, no data is generated. Make sure to enable the generation of
 * some attributes if you want this filter to affect the output. Also note
 * that this filter passes through input geometry, topology and attributes.
 * Newly created attributes may replace attribute data that would have
 * otherwise been passed through.
 *
 * @sa
 * vtkBrownianPoints
 */

#ifndef vtkRandomAttributeGenerator_h
#define vtkRandomAttributeGenerator_h

#include "vtkDeprecation.h"          // For VTK_DEPRECATED_IN_9_4_0
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkCompositeDataSet;
class vtkDataSet;
class vtkFieldData;
class vtkHyperTreeGrid;
class vtkPointData;

class VTKFILTERSGENERAL_EXPORT vtkRandomAttributeGenerator : public vtkPassInputTypeAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static vtkRandomAttributeGenerator* New();
  vtkTypeMacro(vtkRandomAttributeGenerator, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the type of array to create (all components of this array are of this
   * type). This holds true for all arrays that are created.
   */
  vtkSetMacro(DataType, int);
  void SetDataTypeToBit() { this->SetDataType(VTK_BIT); }
  void SetDataTypeToChar() { this->SetDataType(VTK_CHAR); }
  void SetDataTypeToUnsignedChar() { this->SetDataType(VTK_UNSIGNED_CHAR); }
  void SetDataTypeToShort() { this->SetDataType(VTK_SHORT); }
  void SetDataTypeToUnsignedShort() { this->SetDataType(VTK_UNSIGNED_SHORT); }
  void SetDataTypeToInt() { this->SetDataType(VTK_INT); }
  void SetDataTypeToUnsignedInt() { this->SetDataType(VTK_UNSIGNED_INT); }
  void SetDataTypeToLong() { this->SetDataType(VTK_LONG); }
  void SetDataTypeToLongLong() { this->SetDataType(VTK_LONG_LONG); }
  void SetDataTypeToUnsignedLong() { this->SetDataType(VTK_UNSIGNED_LONG); }
  void SetDataTypeToUnsignedLongLong() { this->SetDataType(VTK_UNSIGNED_LONG_LONG); }
  void SetDataTypeToIdType() { this->SetDataType(VTK_ID_TYPE); }
  void SetDataTypeToFloat() { this->SetDataType(VTK_FLOAT); }
  void SetDataTypeToDouble() { this->SetDataType(VTK_DOUBLE); }
  vtkGetMacro(DataType, int);
  ///@}

  ///@{
  /**
   * Specify the number of components to generate. This value only applies to those
   * attribute types that take a variable number of components. For example, a vector
   * is only three components so the number of components is not applicable; whereas
   * a scalar may support multiple, varying number of components.
   */
  vtkSetClampMacro(NumberOfComponents, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfComponents, int);
  ///@}

  ///@{
  /**
   * Set the minimum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  vtkSetMacro(MinimumComponentValue, double);
  vtkGetMacro(MinimumComponentValue, double);
  void SetComponentRange(double minimumValue, double maximumValue)
  {
    this->SetMinimumComponentValue(minimumValue);
    this->SetMaximumComponentValue(maximumValue);
  }
  ///@}

  ///@{
  /**
   * Set the maximum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  vtkSetMacro(MaximumComponentValue, double);
  vtkGetMacro(MaximumComponentValue, double);
  ///@}

  ///@{
  /**
   * Specify the number of tuples to generate. This value only applies when creating
   * general field data. In all other cases (i.e., point data or cell data), the number
   * of tuples is controlled by the number of points and cells, respectively.
   */
  vtkSetClampMacro(NumberOfTuples, vtkIdType, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfTuples, vtkIdType);
  ///@}

  ///@{
  /**
   * Indicate that point scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  vtkSetMacro(GeneratePointScalars, vtkTypeBool);
  vtkGetMacro(GeneratePointScalars, vtkTypeBool);
  vtkBooleanMacro(GeneratePointScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that point vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GeneratePointVectors, vtkTypeBool);
  vtkGetMacro(GeneratePointVectors, vtkTypeBool);
  vtkBooleanMacro(GeneratePointVectors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that point normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GeneratePointNormals, vtkTypeBool);
  vtkGetMacro(GeneratePointNormals, vtkTypeBool);
  vtkBooleanMacro(GeneratePointNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that point tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  vtkSetMacro(GeneratePointTensors, vtkTypeBool);
  vtkGetMacro(GeneratePointTensors, vtkTypeBool);
  vtkBooleanMacro(GeneratePointTensors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that point texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  vtkSetMacro(GeneratePointTCoords, vtkTypeBool);
  vtkGetMacro(GeneratePointTCoords, vtkTypeBool);
  vtkBooleanMacro(GeneratePointTCoords, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that an arbitrary point array is to be generated. The array is
   * added to the points data but is not labeled as one of scalars, vectors,
   * normals, tensors, or texture coordinates (i.e., AddArray() is
   * used). Note that the specified number of components is used to create
   * the array.
   */
  vtkSetMacro(GeneratePointArray, vtkTypeBool);
  vtkGetMacro(GeneratePointArray, vtkTypeBool);
  vtkBooleanMacro(GeneratePointArray, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that cell scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  vtkSetMacro(GenerateCellScalars, vtkTypeBool);
  vtkGetMacro(GenerateCellScalars, vtkTypeBool);
  vtkBooleanMacro(GenerateCellScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that cell vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GenerateCellVectors, vtkTypeBool);
  vtkGetMacro(GenerateCellVectors, vtkTypeBool);
  vtkBooleanMacro(GenerateCellVectors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that cell normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GenerateCellNormals, vtkTypeBool);
  vtkGetMacro(GenerateCellNormals, vtkTypeBool);
  vtkBooleanMacro(GenerateCellNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that cell tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  vtkSetMacro(GenerateCellTensors, vtkTypeBool);
  vtkGetMacro(GenerateCellTensors, vtkTypeBool);
  vtkBooleanMacro(GenerateCellTensors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that cell texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  vtkSetMacro(GenerateCellTCoords, vtkTypeBool);
  vtkGetMacro(GenerateCellTCoords, vtkTypeBool);
  vtkBooleanMacro(GenerateCellTCoords, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that an arbitrary cell array is to be generated. The array is
   * added to the cell data but is not labeled as one of scalars, vectors,
   * normals, tensors, or texture coordinates array (i.e., AddArray() is
   * used). Note that the specified number of components is used to create
   * the array.
   */
  vtkSetMacro(GenerateCellArray, vtkTypeBool);
  vtkGetMacro(GenerateCellArray, vtkTypeBool);
  vtkBooleanMacro(GenerateCellArray, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that an arbitrary field data array is to be generated. Note
   * that the specified number of components is used to create the scalar.
   */
  vtkSetMacro(GenerateFieldArray, vtkTypeBool);
  vtkGetMacro(GenerateFieldArray, vtkTypeBool);
  vtkBooleanMacro(GenerateFieldArray, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Indicate that the generated attributes are
   * constant within a block. This can be used to highlight
   * blocks in a composite dataset.
   */
  vtkSetMacro(AttributesConstantPerBlock, bool);
  vtkGetMacro(AttributesConstantPerBlock, bool);
  vtkBooleanMacro(AttributesConstantPerBlock, bool);
  ///@}

  ///@{
  /**
   * Convenience methods for generating data: all data, all point data, or all cell data.
   * For example, if all data is enabled, then all point, cell and field data is generated.
   * If all point data is enabled, then point scalars, vectors, normals, tensors, tcoords,
   * and a data array are produced.
   */
  void GenerateAllPointDataOn()
  {
    this->GeneratePointScalarsOn();
    this->GeneratePointVectorsOn();
    this->GeneratePointNormalsOn();
    this->GeneratePointTCoordsOn();
    this->GeneratePointTensorsOn();
    this->GeneratePointArrayOn();
  }
  void GenerateAllPointDataOff()
  {
    this->GeneratePointScalarsOff();
    this->GeneratePointVectorsOff();
    this->GeneratePointNormalsOff();
    this->GeneratePointTCoordsOff();
    this->GeneratePointTensorsOff();
    this->GeneratePointArrayOff();
  }
  void GenerateAllCellDataOn()
  {
    this->GenerateCellScalarsOn();
    this->GenerateCellVectorsOn();
    this->GenerateCellNormalsOn();
    this->GenerateCellTCoordsOn();
    this->GenerateCellTensorsOn();
    this->GenerateCellArrayOn();
  }
  void GenerateAllCellDataOff()
  {
    this->GenerateCellScalarsOff();
    this->GenerateCellVectorsOff();
    this->GenerateCellNormalsOff();
    this->GenerateCellTCoordsOff();
    this->GenerateCellTensorsOff();
    this->GenerateCellArrayOff();
  }
  void GenerateAllDataOn()
  {
    this->GenerateAllPointDataOn();
    this->GenerateAllCellDataOn();
    this->GenerateFieldArrayOn();
  }
  void GenerateAllDataOff()
  {
    this->GenerateAllPointDataOff();
    this->GenerateAllCellDataOff();
    this->GenerateFieldArrayOff();
  }
  ///@}

protected:
  vtkRandomAttributeGenerator() = default;
  ~vtkRandomAttributeGenerator() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int DataType = VTK_FLOAT;
  int NumberOfComponents = 1;
  vtkIdType NumberOfTuples = 0;
  double MinimumComponentValue = 0.0;
  double MaximumComponentValue = 1.0;

  vtkTypeBool GeneratePointScalars = 0;
  vtkTypeBool GeneratePointVectors = 0;
  vtkTypeBool GeneratePointNormals = 0;
  vtkTypeBool GeneratePointTCoords = 0;
  vtkTypeBool GeneratePointTensors = 0;
  vtkTypeBool GeneratePointArray = 0;

  vtkTypeBool GenerateCellScalars = 0;
  vtkTypeBool GenerateCellVectors = 0;
  vtkTypeBool GenerateCellNormals = 0;
  vtkTypeBool GenerateCellTCoords = 0;
  vtkTypeBool GenerateCellTensors = 0;
  vtkTypeBool GenerateCellArray = 0;

  vtkTypeBool GenerateFieldArray = 0;
  bool AttributesConstantPerBlock = false;

  /**
   * Returns new array with numTuples tuples and numComp components, with values
   * in the range [min, max]. Only fills components between minComp and maxComp.
   */
  vtkDataArray* GenerateData(int dataType, vtkIdType numTuples, int numComp, int minComp,
    int maxComp, double min, double max);

  /**
   * Fills data with numTuples tuples and numComp components, with values
   * in the range [min, max]. Only fills components between minComp and maxComp.
   */
  template <class T>
  void GenerateRandomTuples(
    T* data, vtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max);

  VTK_DEPRECATED_IN_9_4_0("This function has confusing naming and contains implementation details, "
                          "it as been made private.")
  int RequestData(vtkDataSet* input, vtkDataSet* output);
  VTK_DEPRECATED_IN_9_4_0("This function has confusing naming and contains implementation details, "
                          "it as been made private.")
  int RequestData(vtkCompositeDataSet* input, vtkCompositeDataSet* output);

private:
  vtkRandomAttributeGenerator(const vtkRandomAttributeGenerator&) = delete;
  void operator=(const vtkRandomAttributeGenerator&) = delete;

  /**
   * Helper functions used to generate random attributes for each input type
   */
  int ProcessDataSet(vtkDataSet* input, vtkDataSet* output);
  int ProcessComposite(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  int ProcessHTG(vtkHyperTreeGrid* input, vtkHyperTreeGrid* output);

  /**
   * Helper functions used to generate random attributes for each attribute type.
   * The random attributes are added using the AddArray() method. It is then set as active by name.
   * Note: using SetAttribute() will delete the current active attribute to replace it with the new
   * one.
   */
  void GeneratePointData(vtkPointData* outputPD, vtkIdType numPts);
  void GenerateCellData(vtkCellData* outputCD, vtkIdType numCells);
  void GenerateFieldData(vtkFieldData* outputFD);
};

VTK_ABI_NAMESPACE_END
#endif
