/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomAttributeGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkDataSet;
class vtkCompositeDataSet;

class VTKFILTERSGENERAL_EXPORT vtkRandomAttributeGenerator : public vtkPassInputTypeAlgorithm
{
public:
  /**
   * Create instance with minimum speed 0.0, maximum speed 1.0.
   */
  static vtkRandomAttributeGenerator *New();

  vtkTypeMacro(vtkRandomAttributeGenerator,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the type of array to create (all components of this array are of this
   * type). This holds true for all arrays that are created.
   */
  vtkSetMacro(DataType,int);
  void SetDataTypeToBit() {this->SetDataType(VTK_BIT);}
  void SetDataTypeToChar() {this->SetDataType(VTK_CHAR);}
  void SetDataTypeToUnsignedChar() {this->SetDataType(VTK_UNSIGNED_CHAR);}
  void SetDataTypeToShort() {this->SetDataType(VTK_SHORT);}
  void SetDataTypeToUnsignedShort() {this->SetDataType(VTK_UNSIGNED_SHORT);}
  void SetDataTypeToInt() {this->SetDataType(VTK_INT);}
  void SetDataTypeToUnsignedInt() {this->SetDataType(VTK_UNSIGNED_INT);}
  void SetDataTypeToLong() {this->SetDataType(VTK_LONG);}
  void SetDataTypeToLongLong() {this->SetDataType(VTK_LONG_LONG);}
  void SetDataTypeToUnsignedLong() {this->SetDataType(VTK_UNSIGNED_LONG);}
  void SetDataTypeToUnsignedLongLong() {this->SetDataType(VTK_UNSIGNED_LONG_LONG);}
  void SetDataTypeToIdType() {this->SetDataType(VTK_ID_TYPE);}
  void SetDataTypeToFloat() {this->SetDataType(VTK_FLOAT);}
  void SetDataTypeToDouble() {this->SetDataType(VTK_DOUBLE);}
  vtkGetMacro(DataType,int);
  //@}

  //@{
  /**
   * Specify the number of components to generate. This value only applies to those
   * attribute types that take a variable number of components. For example, a vector
   * is only three components so the number of components is not applicable; whereas
   * a scalar may support multiple, varying number of components.
   */
  vtkSetClampMacro(NumberOfComponents,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfComponents,int);
  //@}

  //@{
  /**
   * Set the minimum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  vtkSetMacro(MinimumComponentValue,double);
  vtkGetMacro(MinimumComponentValue,double);
  void SetComponentRange (double minimumValue, double maximumValue)
  {
    this->SetMinimumComponentValue (minimumValue);
    this->SetMaximumComponentValue (maximumValue);
  }
  //@}

  //@{
  /**
   * Set the maximum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  vtkSetMacro(MaximumComponentValue,double);
  vtkGetMacro(MaximumComponentValue,double);
  //@}

  //@{
  /**
   * Specify the number of tuples to generate. This value only applies when creating
   * general field data. In all other cases (i.e., point data or cell data), the number
   * of tuples is controlled by the number of points and cells, respectively.
   */
  vtkSetClampMacro(NumberOfTuples,vtkIdType,0,VTK_INT_MAX);
  vtkGetMacro(NumberOfTuples,vtkIdType);
  //@}

  //@{
  /**
   * Indicate that point scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  vtkSetMacro(GeneratePointScalars,vtkTypeBool);
  vtkGetMacro(GeneratePointScalars,vtkTypeBool);
  vtkBooleanMacro(GeneratePointScalars,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GeneratePointVectors,vtkTypeBool);
  vtkGetMacro(GeneratePointVectors,vtkTypeBool);
  vtkBooleanMacro(GeneratePointVectors,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GeneratePointNormals,vtkTypeBool);
  vtkGetMacro(GeneratePointNormals,vtkTypeBool);
  vtkBooleanMacro(GeneratePointNormals,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  vtkSetMacro(GeneratePointTensors,vtkTypeBool);
  vtkGetMacro(GeneratePointTensors,vtkTypeBool);
  vtkBooleanMacro(GeneratePointTensors,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  vtkSetMacro(GeneratePointTCoords,vtkTypeBool);
  vtkGetMacro(GeneratePointTCoords,vtkTypeBool);
  vtkBooleanMacro(GeneratePointTCoords,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary point array is to be generated. Note that the
   * specified number of components is used to create the array.
   */
  vtkSetMacro(GeneratePointArray,vtkTypeBool);
  vtkGetMacro(GeneratePointArray,vtkTypeBool);
  vtkBooleanMacro(GeneratePointArray,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  vtkSetMacro(GenerateCellScalars,vtkTypeBool);
  vtkGetMacro(GenerateCellScalars,vtkTypeBool);
  vtkBooleanMacro(GenerateCellScalars,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GenerateCellVectors,vtkTypeBool);
  vtkGetMacro(GenerateCellVectors,vtkTypeBool);
  vtkBooleanMacro(GenerateCellVectors,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  vtkSetMacro(GenerateCellNormals,vtkTypeBool);
  vtkGetMacro(GenerateCellNormals,vtkTypeBool);
  vtkBooleanMacro(GenerateCellNormals,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  vtkSetMacro(GenerateCellTensors,vtkTypeBool);
  vtkGetMacro(GenerateCellTensors,vtkTypeBool);
  vtkBooleanMacro(GenerateCellTensors,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  vtkSetMacro(GenerateCellTCoords,vtkTypeBool);
  vtkGetMacro(GenerateCellTCoords,vtkTypeBool);
  vtkBooleanMacro(GenerateCellTCoords,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary cell array is to be generated. Note that the
   * specified number of components is used to create the array.
   */
  vtkSetMacro(GenerateCellArray,vtkTypeBool);
  vtkGetMacro(GenerateCellArray,vtkTypeBool);
  vtkBooleanMacro(GenerateCellArray,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary field data array is to be generated. Note
   * that the specified number of components is used to create the scalar.
   */
  vtkSetMacro(GenerateFieldArray,vtkTypeBool);
  vtkGetMacro(GenerateFieldArray,vtkTypeBool);
  vtkBooleanMacro(GenerateFieldArray,vtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that the generated attributes are
   * constant within a block. This can be used to highlight
   * blocks in a composite dataset.
   */
  vtkSetMacro(AttributesConstantPerBlock,bool);
  vtkGetMacro(AttributesConstantPerBlock,bool);
  vtkBooleanMacro(AttributesConstantPerBlock,bool);
  //@}


  //@{
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
  //@}

protected:
  vtkRandomAttributeGenerator();
  ~vtkRandomAttributeGenerator() override {}

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int       DataType;
  int       NumberOfComponents;
  vtkIdType NumberOfTuples;
  double    MinimumComponentValue;
  double    MaximumComponentValue;

  vtkTypeBool GeneratePointScalars;
  vtkTypeBool GeneratePointVectors;
  vtkTypeBool GeneratePointNormals;
  vtkTypeBool GeneratePointTCoords;
  vtkTypeBool GeneratePointTensors;
  vtkTypeBool GeneratePointArray;

  vtkTypeBool GenerateCellScalars;
  vtkTypeBool GenerateCellVectors;
  vtkTypeBool GenerateCellNormals;
  vtkTypeBool GenerateCellTCoords;
  vtkTypeBool GenerateCellTensors;
  vtkTypeBool GenerateCellArray;

  vtkTypeBool GenerateFieldArray;
  bool AttributesConstantPerBlock;

  // Helper functions
  vtkDataArray *GenerateData(int dataType, vtkIdType numTuples, int numComp,
                             int minComp, int maxComp, double min, double max);
  int RequestData(vtkDataSet *input, vtkDataSet *output);
  int RequestData(vtkCompositeDataSet *input, vtkCompositeDataSet *output);
  template <class T>
    void GenerateRandomTuples(T *data,
                              vtkIdType numTuples,
                              int numComp,
                              int minComp,
                              int maxComp,
                              double min,
                              double max);


private:
  vtkRandomAttributeGenerator(const vtkRandomAttributeGenerator&) = delete;
  void operator=(const vtkRandomAttributeGenerator&) = delete;
};

#endif
