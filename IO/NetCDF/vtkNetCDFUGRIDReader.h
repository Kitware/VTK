// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkNetCDFUGRIDReader_h
#define vtkNetCDFUGRIDReader_h

#include "vtkDataArraySelection.h" // For vtkSmartPointer downcast
#include "vtkIONetCDFModule.h"     // For export macro
#include "vtkSmartPointer.h"       // For vtkSmartPointer
#include "vtkUnstructuredGridAlgorithm.h"

#include <cstdlib> // For std::size_t
#include <vector>  // For std::vector

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkNetCDFUGRIDeader
 * @brief   Read unstructured NetCDF UGRID files.
 *
 * This reader read a single 2D mesh for a NetCDF UGRID. It will extract points and cells
 * but not edges. Temporal datasets are supported as long as the "time" variable exists in the file.
 * Supported point types are float and double.
 * Supported cell types are triangle and quad.
 * Supported data array types are [u]int[8/16/32/64], float and double.
 */
class VTKIONETCDF_EXPORT vtkNetCDFUGRIDReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNetCDFUGRIDReader* New();
  vtkTypeMacro(vtkNetCDFUGRIDReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the file name of the file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  ///@}

  ///@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  ///@}

  ///@{
  /**
   * Get the name of the point or cell with the given index in the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  ///@}

  ///@{
  /**
   * Get/Set whether the point or cell with the given name is to be read.
   */
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  ///@}

  ///@{
  /**
   * If on, any float or double variable read that has a _FillValue attribute
   * will have that fill value replaced with a not-a-number (NaN) value.  The
   * advantage of setting these to NaN values is that, if implemented properly
   * by the system and careful math operations are used, they can implicitly be
   * ignored by calculations like finding the range of the values.  That said,
   * this option should be used with caution as VTK does not fully support NaN
   * values and therefore odd calculations may occur.  By default this is off.
   */
  vtkGetMacro(ReplaceFillValueWithNan, bool);
  vtkSetMacro(ReplaceFillValueWithNan, bool);
  vtkBooleanMacro(ReplaceFillValueWithNan, bool);
  ///@}

protected:
  vtkNetCDFUGRIDReader();
  ~vtkNetCDFUGRIDReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  bool Open();
  bool ParseHeader();
  bool FillArraySelection(const std::vector<int>& ids, vtkDataArraySelection* selection);
  bool FillPoints(vtkUnstructuredGrid* output);
  bool FillCells(vtkUnstructuredGrid* output);
  bool FillArrays(vtkUnstructuredGrid* output, std::size_t timeStep);
  void Close();

  bool CheckError(int error);
  std::string GetAttributeString(int var, std::string name);
  std::string GetVariableName(int var);
  std::string GetAttributeName(int var, int att);
  std::string GetDimensionName(int dim);
  vtkSmartPointer<vtkDataArray> GetArrayData(int var, std::size_t time, std::size_t size);

private:
  char* FileName = nullptr;

  int NcId = -1;
  int MeshVarId = -1;
  int FaceVarId = -1;
  int FaceFillValue = -1;
  int FaceStartIndex = 0;
  int NodeXVarId = -1;
  int NodeYVarId = -1;
  int NodeType = -1;
  std::size_t NodeCount = 0;
  std::size_t FaceCount = 0;
  std::size_t NodesPerFace = 0;
  std::size_t FaceStride = 0;
  std::size_t NodesPerFaceStride = 0;
  bool ReplaceFillValueWithNan = false;
  std::vector<int> NodeArrayVarIds; // data variables linked to nodes (points)
  std::vector<int> FaceArrayVarIds; // data variables linked to face (cells)
  std::vector<double> TimeSteps;

  vtkSmartPointer<vtkDataArraySelection> PointDataArraySelection;
  vtkSmartPointer<vtkDataArraySelection> CellDataArraySelection;

  vtkNetCDFUGRIDReader(const vtkNetCDFUGRIDReader&) = delete;
  void operator=(const vtkNetCDFUGRIDReader&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
