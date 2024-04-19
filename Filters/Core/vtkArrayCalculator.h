// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArrayCalculator
 * @brief   perform mathematical operations on data in field data arrays
 *
 * vtkArrayCalculator performs operations on vectors or scalars in field
 * data arrays.  It uses vtkFunctionParser/vtkExprTkFunctionParser to do
 * the parsing and to evaluate the function for each entry in the input
 * arrays.  The arrays used in a given function must be all in point data
 * or all in cell data. The resulting array will be stored as a field
 * data array.  The result array can either be stored in a new array or
 * it can overwrite an existing array. vtkArrayCalculator can run in
 * parallel using vtkSMPTools.
 *
 * The functions that this array calculator understands is:
 *
 * standard operations:
 * +
 * -
 * *
 * /
 * ^
 * . (only by vtkFunctionParser)
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
 * ln
 * mag
 * min
 * max
 * norm
 * dot (only by vtkExprTkFunctionParser)
 * cross
 * sign
 * sin
 * sinh
 * sqrt
 * tan
 * tanh
 *
 * Note that some of these operations work on scalars, some on vectors, and some on
 * both (e.g., you can multiply a scalar times a vector). The operations are performed
 * tuple-wise (i.e., tuple-by-tuple). The user must specify which arrays to use as
 * vectors and/or scalars, and the name of the output data array.
 *
 * @sa
 * For more detailed documentation of the supported functionality see:
 * 1) vtkFunctionParser
 * 2) vtkExprTkFunctionParser (default)
 */

#ifndef vtkArrayCalculator_h
#define vtkArrayCalculator_h

#include "vtkDataObject.h"        // For attribute types
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkTuple.h" // needed for vtkTuple
#include <vector>     // needed for vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkArrayCalculator : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkArrayCalculator, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkArrayCalculator* New();

  ///@{
  /**
   * Set/Get the function to be evaluated.
   */
  vtkSetStringMacro(Function);
  vtkGetStringMacro(Function);
  ///@}

  ///@{
  /**
   * Add an array name to the list of arrays used in the function and specify
   * which components of the array to use in evaluating the function. The
   * array name must match the name in the function. If the array name is not
   * sanitized, the variable name will be the array name enclosed in quotes.
   * Use AddScalarVariable or AddVectorVariable to use a user defined
   * variable name.
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  void AddScalarArrayName(const char* arrayName, int component = 0);
  void AddVectorArrayName(
    const char* arrayName, int component0 = 0, int component1 = 1, int component2 = 2);
  ///@}

  ///@{
  /**
   * Add a variable name, a corresponding array name, and which components of
   * the array to use. The variable name should be sanitized or in quotes.
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  void AddScalarVariable(const char* variableName, const char* arrayName, int component = 0);
  void AddVectorVariable(const char* variableName, const char* arrayName, int component0 = 0,
    int component1 = 1, int component2 = 2);
  ///@}

  ///@{
  /**
   * Add a variable name, a corresponding array name, and which components of
   * the array to use. The variable name should be sanitized or in quotes.
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  void AddCoordinateScalarVariable(const char* variableName, int component = 0);
  void AddCoordinateVectorVariable(
    const char* variableName, int component0 = 0, int component1 = 1, int component2 = 2);
  ///@}

  ///@{
  /**
   * Set the name of the array in which to store the result of
   * evaluating this function.  If this is the name of an existing array,
   * that array will be overwritten.  Otherwise a new array will be
   * created with the specified name.
   */
  vtkSetStringMacro(ResultArrayName);
  vtkGetStringMacro(ResultArrayName);
  ///@}

  ///@{
  /**
   * Type of the result array. It is ignored if CoordinateResults is true.
   * Initial value is VTK_DOUBLE.
   */
  vtkGetMacro(ResultArrayType, int);
  vtkSetMacro(ResultArrayType, int);
  ///@}

  ///@{
  /**
   * Set whether to output results as coordinates.  ResultArrayName will be
   * ignored.  Outputting as coordinates is only valid with vector results and
   * if the AttributeMode is AttributeModeToUsePointData.
   * If a valid output can't be made, an error will occur.
   */
  vtkGetMacro(CoordinateResults, vtkTypeBool);
  vtkSetMacro(CoordinateResults, vtkTypeBool);
  vtkBooleanMacro(CoordinateResults, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set whether to output results as point/cell normals. Outputting as
   * normals is only valid with vector results. Point or cell normals are
   * selected using AttributeMode.
   */
  vtkGetMacro(ResultNormals, bool);
  vtkSetMacro(ResultNormals, bool);
  vtkBooleanMacro(ResultNormals, bool);
  ///@}

  ///@{
  /**
   * Set whether to output results as point/cell texture coordinates.
   * Point or cell texture coordinates are selected using AttributeMode.
   * 2-component texture coordinates cannot be generated at this time.
   */
  vtkGetMacro(ResultTCoords, bool);
  vtkSetMacro(ResultTCoords, bool);
  vtkBooleanMacro(ResultTCoords, bool);
  ///@}

  /**
   * Returns a string representation of the calculator's AttributeType
   */
  const char* GetAttributeTypeAsString();

  static const int DEFAULT_ATTRIBUTE_TYPE = -1;
  ///@{
  /**
   * Control which AttributeType the filter operates on (point data or cell data
   * for vtkDataSets).  By default the filter uses Point/Vertex/Row data depending
   * on the input data type.  The input value for this function should be one of the
   * constants in vtkDataObject::AttributeTypes or DEFAULT_ATTRIBUTE_TYPE for 'default behavior'.
   */
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);
  void SetAttributeTypeToDefault() { this->SetAttributeType(DEFAULT_ATTRIBUTE_TYPE); }
  void SetAttributeTypeToPointData() { this->SetAttributeType(vtkDataObject::POINT); }
  void SetAttributeTypeToCellData() { this->SetAttributeType(vtkDataObject::CELL); }
  void SetAttributeTypeToEdgeData() { this->SetAttributeType(vtkDataObject::EDGE); }
  void SetAttributeTypeToVertexData() { this->SetAttributeType(vtkDataObject::VERTEX); }
  void SetAttributeTypeToRowData() { this->SetAttributeType(vtkDataObject::ROW); }
  ///@}

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

  ///@{
  /**
   * Methods to get information about the current variables.
   */
  const std::vector<std::string>& GetScalarArrayNames() { return this->ScalarArrayNames; }
  std::string GetScalarArrayName(int i);
  const std::vector<std::string>& GetVectorArrayNames() { return this->VectorArrayNames; }
  std::string GetVectorArrayName(int i);
  const std::vector<std::string>& GetScalarVariableNames() { return this->ScalarVariableNames; }
  std::string GetScalarVariableName(int i);
  const std::vector<std::string>& GetVectorVariableNames() { return this->VectorVariableNames; }
  std::string GetVectorVariableName(int i);
  const std::vector<int>& GetSelectedScalarComponents() { return this->SelectedScalarComponents; }
  int GetSelectedScalarComponent(int i);
  const std::vector<vtkTuple<int, 3>>& GetSelectedVectorComponents()
  {
    return this->SelectedVectorComponents;
  }
  vtkTuple<int, 3> GetSelectedVectorComponents(int i);
  int GetNumberOfScalarArrays() { return static_cast<int>(this->ScalarArrayNames.size()); }
  int GetNumberOfVectorArrays() { return static_cast<int>(this->VectorArrayNames.size()); }
  ///@}

  ///@{
  /**
   * When ReplaceInvalidValues is on, all invalid values (such as
   * sqrt(-2), note that function parser does not handle complex
   * numbers) will be replaced by ReplacementValue. Otherwise an
   * error will be reported.
   */
  vtkSetMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkGetMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkBooleanMacro(ReplaceInvalidValues, vtkTypeBool);
  vtkSetMacro(ReplacementValue, double);
  vtkGetMacro(ReplacementValue, double);
  ///@}

  ///@{
  /**
   * When this option is set, silently ignore datasets where the requested field
   * data array is not present. When an input array is not present, the result array
   * will not be generated nor added to the output.
   */
  vtkSetMacro(IgnoreMissingArrays, bool);
  vtkGetMacro(IgnoreMissingArrays, bool);
  vtkBooleanMacro(IgnoreMissingArrays, bool);
  ///@}

  /**
   * Enum that includes the types of parsers that can be used.
   */
  enum FunctionParserTypes
  {
    FunctionParser,       // vtkFunctionParser
    ExprTkFunctionParser, // vtkExprTkFunctionParser
    NumberOfFunctionParserTypes
  };

  ///@{
  /**
   * Set/Get the FunctionParser type that will be used.
   * vtkFunctionParser = 0, vtkExprTkFunctionParser = 1. Default is 1.
   */
  vtkSetEnumMacro(FunctionParserType, FunctionParserTypes);
  void SetFunctionParserTypeToFunctionParser()
  {
    this->FunctionParserType = FunctionParserTypes::FunctionParser;
    this->Modified();
  }
  void SetFunctionParserTypeToExprTkFunctionParser()
  {
    this->FunctionParserType = FunctionParserTypes::ExprTkFunctionParser;
    this->Modified();
  }
  vtkGetEnumMacro(FunctionParserType, FunctionParserTypes);
  ///@}

  /**
   * Returns the output of the filter downcast to a vtkDataSet or nullptr if the
   * cast fails.
   */
  vtkDataSet* GetDataSetOutput();

protected:
  vtkArrayCalculator();
  ~vtkArrayCalculator() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Get the attribute type for the input.
   */
  int GetAttributeTypeFromInput(vtkDataObject* input);

  /**
   * A variable name is valid if it's sanitized or enclosed in quotes.
   * 1) if it's valid, return itself.
   * 2) if it's not valid, return itself enclosed in quotes,
   *
   * @note A sanitized variable name is accepted by the following regex: ^[a-zA-Z][a-zA-Z_0-9]*.
   */
  static std::string CheckValidVariableName(const char* variableName);

  FunctionParserTypes FunctionParserType;

  char* Function;
  char* ResultArrayName;
  std::vector<std::string> ScalarArrayNames;
  std::vector<std::string> VectorArrayNames;
  std::vector<std::string> ScalarVariableNames;
  std::vector<std::string> VectorVariableNames;
  int AttributeType;
  std::vector<int> SelectedScalarComponents;
  std::vector<vtkTuple<int, 3>> SelectedVectorComponents;

  vtkTypeBool ReplaceInvalidValues;
  double ReplacementValue;
  bool IgnoreMissingArrays;

  vtkTypeBool CoordinateResults;
  bool ResultNormals;
  bool ResultTCoords;
  std::vector<std::string> CoordinateScalarVariableNames;
  std::vector<std::string> CoordinateVectorVariableNames;
  std::vector<int> SelectedCoordinateScalarComponents;
  std::vector<vtkTuple<int, 3>> SelectedCoordinateVectorComponents;

  int ResultArrayType;

private:
  vtkArrayCalculator(const vtkArrayCalculator&) = delete;
  void operator=(const vtkArrayCalculator&) = delete;

  // Do the bulk of the work
  template <typename TFunctionParser>
  int ProcessDataObject(vtkDataObject* input, vtkDataObject* output);
};

VTK_ABI_NAMESPACE_END
#endif
