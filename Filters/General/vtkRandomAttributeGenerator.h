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
// .NAME vtkRandomAttributeGenerator - generate and create random data attributes
// .SECTION Description
// vtkRandomAttributeGenerator is a filter that creates random attributes
// including scalars, vectors, normals, tensors, texture coordinates and/or
// general data arrays. These attributes can be generated as point data, cell
// data or general field data. The generation of each component is normalized
// between a user-specified minimum and maximum value.
//
// This filter provides that capability to specify the data type of the
// attributes, the range for each of the components, and the number of
// components. Note, however, that this flexibility only goes so far because
// some attributes (e.g., normals, vectors and tensors) are fixed in the
// number of components, and in the case of normals and tensors, are
// constrained in the values that some of the components can take (i.e.,
// normals have magnitude one, and tensors are symmetric).

// .SECTION Caveats
// In general this class is used for debugging or testing purposes.
//
// It is possible to generate multiple attributes simultaneously.
//
// By default, no data is generated. Make sure to enable the generation of
// some attributes if you want this filter to affect the output. Also note
// that this filter passes through input geometry, topology and attributes.
// Newly created attributes may replace attribute data that would have
// otherwise been passed through.

// .SECTION See also
// vtkBrownianPoints


#ifndef vtkRandomAttributeGenerator_h
#define vtkRandomAttributeGenerator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkDataSet;
class vtkCompositeDataSet;

class VTKFILTERSGENERAL_EXPORT vtkRandomAttributeGenerator : public vtkPassInputTypeAlgorithm
{
public:
  // Description:
  // Create instance with minimum speed 0.0, maximum speed 1.0.
  static vtkRandomAttributeGenerator *New();

  vtkTypeMacro(vtkRandomAttributeGenerator,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the type of array to create (all components of this array are of this
  // type). This holds true for all arrays that are created.
  vtkSetMacro(DataType,int);
  void SetDataTypeToBit() {this->SetDataType(VTK_BIT);}
  void SetDataTypeToChar() {this->SetDataType(VTK_CHAR);}
  void SetDataTypeToUnsignedChar() {this->SetDataType(VTK_UNSIGNED_CHAR);}
  void SetDataTypeToShort() {this->SetDataType(VTK_SHORT);}
  void SetDataTypeToUnsignedShort() {this->SetDataType(VTK_UNSIGNED_SHORT);}
  void SetDataTypeToInt() {this->SetDataType(VTK_INT);}
  void SetDataTypeToUnsignedInt() {this->SetDataType(VTK_UNSIGNED_INT);}
  void SetDataTypeToLong() {this->SetDataType(VTK_LONG);}
  void SetDataTypeToUnsignedLong() {this->SetDataType(VTK_UNSIGNED_LONG);}
  void SetDataTypeToFloat() {this->SetDataType(VTK_FLOAT);}
  void SetDataTypeToDouble() {this->SetDataType(VTK_DOUBLE);}
  vtkGetMacro(DataType,int);

  // Description:
  // Specify the number of components to generate. This value only applies to those
  // attribute types that take a variable number of components. For example, a vector
  // is only three components so the number of components is not applicable; whereas
  // a scalar may support multiple, varying number of components.
  vtkSetClampMacro(NumberOfComponents,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfComponents,int);

  // Description:
  // Set the minimum component value. This applies to all data that is generated,
  // although normals and tensors have internal constraints that must be
  // observed.
  vtkSetMacro(MinimumComponentValue,double);
  vtkGetMacro(MinimumComponentValue,double);
  void SetComponentRange (double minimumValue, double maximumValue)
    {
    this->SetMinimumComponentValue (minimumValue);
    this->SetMaximumComponentValue (maximumValue);
    }

  // Description:
  // Set the maximum component value. This applies to all data that is generated,
  // although normals and tensors have internal constraints that must be
  // observed.
  vtkSetMacro(MaximumComponentValue,double);
  vtkGetMacro(MaximumComponentValue,double);

  // Description:
  // Specify the number of tuples to generate. This value only applies when creating
  // general field data. In all other cases (i.e., point data or cell data), the number
  // of tuples is controlled by the number of points and cells, respectively.
  vtkSetClampMacro(NumberOfTuples,vtkIdType,0,VTK_INT_MAX);
  vtkGetMacro(NumberOfTuples,vtkIdType);

  // Description:
  // Indicate that point scalars are to be generated. Note that the specified
  // number of components is used to create the scalar.
  vtkSetMacro(GeneratePointScalars,int);
  vtkGetMacro(GeneratePointScalars,int);
  vtkBooleanMacro(GeneratePointScalars,int);

  // Description:
  // Indicate that point vectors are to be generated. Note that the
  // number of components is always equal to three.
  vtkSetMacro(GeneratePointVectors,int);
  vtkGetMacro(GeneratePointVectors,int);
  vtkBooleanMacro(GeneratePointVectors,int);

  // Description:
  // Indicate that point normals are to be generated. Note that the
  // number of components is always equal to three.
  vtkSetMacro(GeneratePointNormals,int);
  vtkGetMacro(GeneratePointNormals,int);
  vtkBooleanMacro(GeneratePointNormals,int);

  // Description:
  // Indicate that point tensors are to be generated. Note that the
  // number of components is always equal to nine.
  vtkSetMacro(GeneratePointTensors,int);
  vtkGetMacro(GeneratePointTensors,int);
  vtkBooleanMacro(GeneratePointTensors,int);

  // Description:
  // Indicate that point texture coordinates are to be generated. Note that
  // the specified number of components is used to create the texture
  // coordinates (but must range between one and three).
  vtkSetMacro(GeneratePointTCoords,int);
  vtkGetMacro(GeneratePointTCoords,int);
  vtkBooleanMacro(GeneratePointTCoords,int);

  // Description:
  // Indicate that an arbitrary point array is to be generated. Note that the
  // specified number of components is used to create the array.
  vtkSetMacro(GeneratePointArray,int);
  vtkGetMacro(GeneratePointArray,int);
  vtkBooleanMacro(GeneratePointArray,int);

  // Description:
  // Indicate that cell scalars are to be generated. Note that the specified
  // number of components is used to create the scalar.
  vtkSetMacro(GenerateCellScalars,int);
  vtkGetMacro(GenerateCellScalars,int);
  vtkBooleanMacro(GenerateCellScalars,int);

  // Description:
  // Indicate that cell vectors are to be generated. Note that the
  // number of components is always equal to three.
  vtkSetMacro(GenerateCellVectors,int);
  vtkGetMacro(GenerateCellVectors,int);
  vtkBooleanMacro(GenerateCellVectors,int);

  // Description:
  // Indicate that cell normals are to be generated. Note that the
  // number of components is always equal to three.
  vtkSetMacro(GenerateCellNormals,int);
  vtkGetMacro(GenerateCellNormals,int);
  vtkBooleanMacro(GenerateCellNormals,int);

  // Description:
  // Indicate that cell tensors are to be generated. Note that the
  // number of components is always equal to nine.
  vtkSetMacro(GenerateCellTensors,int);
  vtkGetMacro(GenerateCellTensors,int);
  vtkBooleanMacro(GenerateCellTensors,int);

  // Description:
  // Indicate that cell texture coordinates are to be generated. Note that
  // the specified number of components is used to create the texture
  // coordinates (but must range between one and three).
  vtkSetMacro(GenerateCellTCoords,int);
  vtkGetMacro(GenerateCellTCoords,int);
  vtkBooleanMacro(GenerateCellTCoords,int);

  // Description:
  // Indicate that an arbitrary cell array is to be generated. Note that the
  // specified number of components is used to create the array.
  vtkSetMacro(GenerateCellArray,int);
  vtkGetMacro(GenerateCellArray,int);
  vtkBooleanMacro(GenerateCellArray,int);

  // Description:
  // Indicate that an arbitrary field data array is to be generated. Note
  // that the specified number of components is used to create the scalar.
  vtkSetMacro(GenerateFieldArray,int);
  vtkGetMacro(GenerateFieldArray,int);
  vtkBooleanMacro(GenerateFieldArray,int);

  // Description:
  // Indicate that the generated attributes are
  // constant within a block. This can be used to highlight
  // blocks in a composite dataset.
  vtkSetMacro(AttributesConstantPerBlock,bool);
  vtkGetMacro(AttributesConstantPerBlock,bool);
  vtkBooleanMacro(AttributesConstantPerBlock,bool);


  // Description:
  // Convenience methods for generating data: all data, all point data, or all cell data.
  // For example, if all data is enabled, then all point, cell and field data is generated.
  // If all point data is enabled, then point scalars, vectors, normals, tensors, tcoords,
  // and a data array are produced.
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

protected:
  vtkRandomAttributeGenerator();
  ~vtkRandomAttributeGenerator() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int       DataType;
  int       NumberOfComponents;
  vtkIdType NumberOfTuples;
  double    MinimumComponentValue;
  double    MaximumComponentValue;

  int GeneratePointScalars;
  int GeneratePointVectors;
  int GeneratePointNormals;
  int GeneratePointTCoords;
  int GeneratePointTensors;
  int GeneratePointArray;

  int GenerateCellScalars;
  int GenerateCellVectors;
  int GenerateCellNormals;
  int GenerateCellTCoords;
  int GenerateCellTensors;
  int GenerateCellArray;

  int GenerateFieldArray;
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
  vtkRandomAttributeGenerator(const vtkRandomAttributeGenerator&);  // Not implemented.
  void operator=(const vtkRandomAttributeGenerator&);  // Not implemented.
};

#endif
