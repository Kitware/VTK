/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkArrayCalculator
 * @brief   perform mathematical operations on data in field data arrays
 *
 * vtkArrayCalculator performs operations on vectors or scalars in field
 * data arrays.  It uses vtkFunctionParser to do the parsing and to
 * evaluate the function for each entry in the input arrays.  The arrays
 * used in a given function must be all in point data or all in cell data.
 * The resulting array will be stored as a field data array.  The result
 * array can either be stored in a new array or it can overwrite an existing
 * array.
 *
 * The functions that this array calculator understands is:
 * <pre>
 * standard operations: + - * / ^ .
 * build unit vectors: iHat, jHat, kHat (ie (1,0,0), (0,1,0), (0,0,1))
 * abs
 * acos
 * asin
 * atan
 * ceil
 * cos
 * cosh
 * exp
 * floor
 * log
 * mag
 * min
 * max
 * norm
 * sign
 * sin
 * sinh
 * sqrt
 * tan
 * tanh
 * </pre>
 * Note that some of these operations work on scalars, some on vectors, and some on
 * both (e.g., you can multiply a scalar times a vector). The operations are performed
 * tuple-wise (i.e., tuple-by-tuple). The user must specify which arrays to use as
 * vectors and/or scalars, and the name of the output data array.
 *
 * @sa
 * vtkFunctionParser
*/

#ifndef vtkArrayCalculator_h
#define vtkArrayCalculator_h

#include "vtkDataObject.h" // For attribute types
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkDataSet;
class vtkFunctionParser;

#ifndef VTK_LEGACY_REMOVE
#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2
#define VTK_ATTRIBUTE_MODE_USE_VERTEX_DATA 3
#define VTK_ATTRIBUTE_MODE_USE_EDGE_DATA 4
#endif

class VTKFILTERSCORE_EXPORT vtkArrayCalculator : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkArrayCalculator,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkArrayCalculator *New();

  //@{
  /**
   * Set/Get the function to be evaluated.
   */
  virtual void SetFunction(const char* function);
  vtkGetStringMacro(Function);
  //@}

  //@{
  /**
   * Add an array name to the list of arrays used in the function and specify
   * which components of the array to use in evaluating the function.  The
   * array name must match the name in the function.  Use AddScalarVariable or
   * AddVectorVariable to use a variable name different from the array name.
   */
  void AddScalarArrayName(const char* arrayName, int component = 0);
  void AddVectorArrayName(const char* arrayName, int component0 = 0,
                          int component1 = 1, int component2 = 2);
  //@}

  //@{
  /**
   * Add a variable name, a corresponding array name, and which components of
   * the array to use.
   */
  void AddScalarVariable(const char* variableName, const char* arrayName,
                         int component = 0);
  void AddVectorVariable(const char* variableName, const char* arrayName,
                         int component0 = 0, int component1 = 1,
                         int component2 = 2);
  //@}

  //@{
  /**
   * Add a variable name, a corresponding array name, and which components of
   * the array to use.
   */
  void AddCoordinateScalarVariable(const char* variableName,
                                   int component = 0);
  void AddCoordinateVectorVariable(const char* variableName,
                                   int component0 = 0, int component1 = 1,
                                   int component2 = 2);
  //@}

  //@{
  /**
   * Set the name of the array in which to store the result of
   * evaluating this function.  If this is the name of an existing array,
   * that array will be overwritten.  Otherwise a new array will be
   * created with the specified name.
   */
  void SetResultArrayName(const char* name);
  vtkGetStringMacro(ResultArrayName);
  //@}

  //@{
  /**
   * Type of the result array. It is ignored if CoordinateResults is true.
   * Initial value is VTK_DOUBLE.
   */
  vtkGetMacro(ResultArrayType,int);
  vtkSetMacro(ResultArrayType,int);
  //@}

  //@{
  /**
   * Set whether to output results as coordinates.  ResultArrayName will be
   * ignored.  Outputting as coordinates is only valid with vector results and
   * if the AttributeMode is AttributeModeToUsePointData.
   * If a valid output can't be made, an error will occur.
   */
  vtkGetMacro(CoordinateResults, vtkTypeBool);
  vtkSetMacro(CoordinateResults, vtkTypeBool);
  vtkBooleanMacro(CoordinateResults, vtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to output results as point/cell normals. Outputting as
   * normals is only valid with vector results. Point or cell normals are
   * selected using AttributeMode.
   */
  vtkGetMacro(ResultNormals, bool);
  vtkSetMacro(ResultNormals, bool);
  vtkBooleanMacro(ResultNormals, bool);
  //@}

  //@{
  /**
   * Set whether to output results as point/cell texture coordinates.
   * Point or cell texture coordinates are selected using AttributeMode.
   * 2-component texture coordinates cannot be generated at this time.
   */
  vtkGetMacro(ResultTCoords, bool);
  vtkSetMacro(ResultTCoords, bool);
  vtkBooleanMacro(ResultTCoords, bool);
  //@}

  //@{
  /**
   * Control whether the filter operates on point data or cell data.
   * By default (AttributeModeToDefault), the filter uses point
   * data. Alternatively you can explicitly set the filter to use point data
   * (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
   * For graphs you can set the filter to use vertex data
   * (AttributeModeToUseVertexData) or edge data (AttributeModeToUseEdgeData).
   *
   * @deprecated Replaced By GetAttributeType and SetAttributeType as of VTK 8.1.
   */
#ifndef VTK_LEGACY_REMOVE
  VTK_LEGACY(void SetAttributeMode(int newMode);)
  VTK_LEGACY(int GetAttributeMode();)
  VTK_LEGACY(void SetAttributeModeToDefault())
    {this->SetAttributeType(DEFAULT_ATTRIBUTE_TYPE);};
  VTK_LEGACY(void SetAttributeModeToUsePointData())
    {this->SetAttributeType(vtkDataObject::POINT);};
  VTK_LEGACY(void SetAttributeModeToUseCellData())
    {this->SetAttributeType(vtkDataObject::CELL);};
  VTK_LEGACY(void SetAttributeModeToUseVertexData())
    {this->SetAttributeType(vtkDataObject::VERTEX);};
  VTK_LEGACY(void SetAttributeModeToUseEdgeData())
    {this->SetAttributeType(vtkDataObject::EDGE);};
  VTK_LEGACY(const char *GetAttributeModeAsString());
#endif
  //@}

  /**
   * Returns a string representation of the calculator's AttributeType
   */
  const char *GetAttributeTypeAsString();

  static const int DEFAULT_ATTRIBUTE_TYPE = -1;
  //@{
  /**
   * Control which AttributeType the filter operates on (point data or cell data
   * for vtkDataSets).  By default the filter uses Point/Vertex/Row data depending
   * on the input data type.  The input value for this function should be one of the
   * constants in vtkDataObject::AttributeTypes or DEFAULT_ATTRIBUTE_TYPE for 'default behavior'.
   */
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);
  void SetAttributeTypeToDefault()
  {this->SetAttributeType(DEFAULT_ATTRIBUTE_TYPE);}
  void SetAttributeTypeToPointData()
  {this->SetAttributeType(vtkDataObject::POINT);}
  void SetAttributeTypeToCellData()
  {this->SetAttributeType(vtkDataObject::CELL);}
  void SetAttributeTypeToEdgeData()
  {this->SetAttributeType(vtkDataObject::EDGE);}
  void SetAttributeTypeToVertexData()
  {this->SetAttributeType(vtkDataObject::VERTEX);}
  void SetAttributeTypeToRowData()
  {this->SetAttributeType(vtkDataObject::ROW);}
  //@}

  /**
   * Remove all the variable names and their associated array names.
   */
  void RemoveAllVariables();

  /**
   * Remove all the scalar variable names and their associated array names.
   */
  virtual void RemoveScalarVariables();

  /**
   * Remove all the scalar variable names and their associated array names.
   */
  virtual void RemoveVectorVariables();

  /**
   * Remove all the coordinate variables.
   */
  virtual void RemoveCoordinateScalarVariables();

  /**
   * Remove all the coordinate variables.
   */
  virtual void RemoveCoordinateVectorVariables();

  //@{
  /**
   * Methods to get information about the current variables.
   */
  char** GetScalarArrayNames() { return this->ScalarArrayNames; }
  char* GetScalarArrayName(int i);
  char** GetVectorArrayNames() { return this->VectorArrayNames; }
  char* GetVectorArrayName(int i);
  char** GetScalarVariableNames() { return this->ScalarVariableNames; }
  char* GetScalarVariableName(int i);
  char** GetVectorVariableNames() { return this->VectorVariableNames; }
  char* GetVectorVariableName(int i);
  int* GetSelectedScalarComponents() { return this->SelectedScalarComponents; }
  int GetSelectedScalarComponent(int i);
  int** GetSelectedVectorComponents() { return this->SelectedVectorComponents;}
  int* GetSelectedVectorComponents(int i);
  vtkGetMacro(NumberOfScalarArrays, int);
  vtkGetMacro(NumberOfVectorArrays, int);
  //@}

  //@{
  /**
   * When ReplaceInvalidValues is on, all invalid values (such as
   * sqrt(-2), note that function parser does not handle complex
   * numbers) will be replaced by ReplacementValue. Otherwise an
   * error will be reported
   */
  vtkSetMacro(ReplaceInvalidValues,vtkTypeBool);
  vtkGetMacro(ReplaceInvalidValues,vtkTypeBool);
  vtkBooleanMacro(ReplaceInvalidValues,vtkTypeBool);
  vtkSetMacro(ReplacementValue,double);
  vtkGetMacro(ReplacementValue,double);
  //@}

  /**
   * Returns the output of the filter downcast to a vtkDataSet or nullptr if the
   * cast fails.
   */
  vtkDataSet* GetDataSetOutput();

protected:
  vtkArrayCalculator();
  ~vtkArrayCalculator() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  char  * Function;
  char  * ResultArrayName;
  char ** ScalarArrayNames;
  char ** VectorArrayNames;
  char ** ScalarVariableNames;
  char ** VectorVariableNames;
  int     NumberOfScalarArrays;
  int     NumberOfVectorArrays;
  int     AttributeType;
  int   * SelectedScalarComponents;
  int  ** SelectedVectorComponents;
  vtkFunctionParser* FunctionParser;

  vtkTypeBool     ReplaceInvalidValues;
  double  ReplacementValue;

  vtkTypeBool     CoordinateResults;
  bool    ResultNormals;
  bool    ResultTCoords;
  char ** CoordinateScalarVariableNames;
  char ** CoordinateVectorVariableNames;
  int   * SelectedCoordinateScalarComponents;
  int  ** SelectedCoordinateVectorComponents;
  int     NumberOfCoordinateScalarArrays;
  int     NumberOfCoordinateVectorArrays;

  int     ResultArrayType;
private:
  vtkArrayCalculator(const vtkArrayCalculator&) = delete;
  void operator=(const vtkArrayCalculator&) = delete;
};

#endif
