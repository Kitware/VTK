// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendPolyData
 * @brief   appends one or more polygonal datasets together
 *
 *
 * vtkAppendPolyData is a filter that appends one of more polygonal datasets
 * into a single polygonal dataset. All geometry is extracted and appended,
 * but point and cell attributes (i.e., scalars, vectors, normals) are
 * extracted and appended only if all datasets have the point and/or cell
 * attributes available.  (For example, if one dataset has point scalars but
 * another does not, point scalars will not be appended.)
 *
 * @warning
 * The related filter vtkRemovePolyData enables the subtraction, or removal
 * of the cells of a vtkPolyData. Hence vtkRemovePolyData functions like the
 * inverse operation to vtkAppendPolyData.
 *
 * @sa
 * vtkAppendFilter vtkRemovePolyData
 */

#ifndef vtkAppendPolyData_h
#define vtkAppendPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkPoints;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT VTK_MARSHALAUTO vtkAppendPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkAppendPolyData* New();

  vtkTypeMacro(vtkAppendPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * UserManagedInputs allows the user to set inputs by number instead of
   * using the AddInput/RemoveInput functions. Calls to
   * SetNumberOfInputs/SetInputConnectionByNumber should not be mixed with calls
   * to AddInput/RemoveInput. By default, UserManagedInputs is false.
   */
  vtkSetMacro(UserManagedInputs, vtkTypeBool);
  vtkGetMacro(UserManagedInputs, vtkTypeBool);
  vtkBooleanMacro(UserManagedInputs, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get if we want to use implicit array when appending the polydata. Using implicit array
   * improve the execution time of the filter and reduce the memory consumption of the output.
   * However, accessing arrays can be slower.
   * Default to false
   */
  vtkGetMacro(UseImplicitArray, bool);
  vtkSetMacro(UseImplicitArray, bool);
  vtkBooleanMacro(UseImplicitArray, bool);
  ///@}

  /**
   * Add a dataset to the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber instead.
   */
  void AddInputData(vtkPolyData*);

  /**
   * Remove a dataset from the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber (nullptr) instead.
   */
  void RemoveInputData(vtkPolyData*);

  ///@{
  /**
   * Get any input of this filter.
   */
  vtkPolyData* GetInput(int idx);
  vtkPolyData* GetInput() { return this->GetInput(0); }
  ///@}

  /**
   * Directly set(allocate) number of inputs, should only be used
   * when UserManagedInputs is true.
   */
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, vtkAlgorithmOutput* input);
  void SetInputDataByNumber(int num, vtkPolyData* ds);

  ///@{
  /**
   * ParallelStreaming is for a particular application.
   * It causes this filter to ask for a different piece
   * from each of its inputs.  If all the inputs are the same,
   * then the output of this append filter is the whole dataset
   * pieced back together.  Duplicate points are create
   * along the seams.  The purpose of this feature is to get
   * data parallelism at a course scale.  Each of the inputs
   * can be generated in a different process at the same time.
   */
  vtkSetMacro(ParallelStreaming, vtkTypeBool);
  vtkGetMacro(ParallelStreaming, vtkTypeBool);
  vtkBooleanMacro(ParallelStreaming, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  int ExecuteAppend(vtkPolyData* output, vtkPolyData* inputs[], int numInputs)
    VTK_SIZEHINT(inputs, numInputs);

  /**
   * PolyDataOffsets agregates cell array information from the input polydata list.
   *
   * vtkPolyData has 4 vtkCellArray (verts, lines, polys and strips), that should be filled in the
   * correct order. This is also visible in vtkCellData arrays ordering. When merging poly data, the
   * output should first aggregate verts (cells and data arrays) from each input, then lines, etc...
   *
   * This structure helps knowing the output offset for a given input polydata.
   *
   * Also handle number of points.
   */
  struct PolyDataOffsets
  {
    PolyDataOffsets(const std::vector<vtkPolyData*>& datasets);

    vtkIdType TotalNumberOfPoints = 0;
    vtkIdType TotalNumberOfCells = 0;
    vtkIdType TotalNumberOfVerts = 0;
    vtkIdType TotalNumberOfVertsConnectivity = 0;
    vtkIdType TotalNumberOfLines = 0;
    vtkIdType TotalNumberOfLinesConnectivity = 0;
    vtkIdType TotalNumberOfPolys = 0;
    vtkIdType TotalNumberOfPolysConnectivity = 0;
    vtkIdType TotalNumberOfStrips = 0;
    vtkIdType TotalNumberOfStripsConnectivity = 0;
    std::vector<vtkIdType> PointOffsets;
    std::vector<vtkIdType> VertOffsets;
    std::vector<vtkIdType> VertConnectivityOffsets;
    std::vector<vtkIdType> LineOffsets;
    std::vector<vtkIdType> LineConnectivityOffsets;
    std::vector<vtkIdType> PolyOffsets;
    std::vector<vtkIdType> PolyConnectivityOffsets;
    std::vector<vtkIdType> StripOffsets;
    std::vector<vtkIdType> StripConnectivityOffsets;
  };

  static void AppendCellData(const PolyDataOffsets& stats, const std::vector<vtkPolyData*>& inputs,
    vtkPolyData* output, bool useImplicit);

protected:
  vtkAppendPolyData();
  ~vtkAppendPolyData() override;

  // Flag for selecting parallel streaming behavior
  vtkTypeBool ParallelStreaming;
  int OutputPointsPrecision;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(vtkDataObject*)
  {
    vtkErrorMacro(<< "AddInput() must be called with a vtkPolyData not a vtkDataObject.");
  }

  /**
   * Fill the output vtkPoints with a concatenation of inputs ones.
   */
  void AppendPoints(
    const PolyDataOffsets& stats, const std::vector<vtkPolyData*>& inputs, vtkPolyData* output);

  /**
   * Fill the output 4 vtkCellArray with a concatenation of inputs ones.
   * Note that from vtkPolyData doc, it insert in order verts, lines, polys then strips.
   */
  void AppendCells(
    const PolyDataOffsets& stats, const std::vector<vtkPolyData*>& inputs, vtkPolyData* output);

  vtkTypeBool UserManagedInputs;

  bool UseImplicitArray = false;

  vtkAppendPolyData(const vtkAppendPolyData&) = delete;
  void operator=(const vtkAppendPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
