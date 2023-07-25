// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkNetCDFCAMReader
 * @brief   Read unstructured NetCDF CAM files.
 *
 * Reads in a NetCDF CAM (Community Atmospheric Model) file and
 * produces and unstructured grid.  The grid is actually unstructured
 * in the X and Y directions and rectilinear in the Z direction. If we
 * read one layer we produce quad cells otherwise we produce hex
 * cells.  The reader requires 2 NetCDF files: the main file has all
 * attributes, the connectivity file has point positions and cell
 * connectivity information.
 */

#ifndef vtkNetCDFCAMReader_h
#define vtkNetCDFCAMReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCallbackCommand;
class vtkDataArraySelection;

class VTKIONETCDF_EXPORT vtkNetCDFCAMReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNetCDFCAMReader* New();
  vtkTypeMacro(vtkNetCDFCAMReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns 1 if this file can be read and 0 if the file cannot be read.
   * Because NetCDF CAM files come in pairs and we only check one of the
   * files, the result is not definitive.  Invalid files may still return 1
   * although a valid file will never return 0.
   */
  static int CanReadFile(VTK_FILEPATH const char* fileName);

  void SetFileName(VTK_FILEPATH const char* fileName);
  vtkGetFilePathMacro(FileName);

  void SetConnectivityFileName(VTK_FILEPATH const char* fileName);
  vtkGetFilePathMacro(ConnectivityFileName);

  ///@{
  /**
   * Set whether to read a single layer, midpoint layers or interface layers.
   * VERTICAL_DIMENSION_SINGLE_LAYER (0) indicates that only a single
   * layer will be read in. The NetCDF variables loaded will be the
   * ones with dimensions (time, ncol).
   * VERTICAL_DIMENSION_MIDPOINT_LAYERS (1) indicates that variables defined
   * on midpoint layers will be read in. These are variables with dimensions
   * (time, lev, ncol).
   * VERTICAL_DIMENSION_INTERFACE_LAYERS (2) indicates that variables
   * defined on interface layers will be read in. These are variables with
   * dimensions (time, ilev, ncol).
   */
  enum VerticalDimension
  {
    VERTICAL_DIMENSION_SINGLE_LAYER,
    VERTICAL_DIMENSION_MIDPOINT_LAYERS,
    VERTICAL_DIMENSION_INTERFACE_LAYERS,
    VERTICAL_DIMENSION_COUNT
  };
  vtkSetClampMacro(VerticalDimension, int, 0, 2);
  vtkGetMacro(VerticalDimension, int);
  ///@}

  ///@{
  /**
   * If SingleXXXLayer is 1, we'll load only the layer specified by
   * XXXLayerIndex.  Otherwise, we load all layers. We do that for
   * midpoint layer variables ( which have dimension 'lev') or for
   * interface layer variables (which have dimension 'ilev').
   */
  vtkBooleanMacro(SingleMidpointLayer, vtkTypeBool);
  vtkSetMacro(SingleMidpointLayer, vtkTypeBool);
  vtkGetMacro(SingleMidpointLayer, vtkTypeBool);
  vtkSetMacro(MidpointLayerIndex, int);
  vtkGetMacro(MidpointLayerIndex, int);
  vtkGetVector2Macro(MidpointLayersRange, int);

  vtkBooleanMacro(SingleInterfaceLayer, vtkTypeBool);
  vtkSetMacro(SingleInterfaceLayer, vtkTypeBool);
  vtkGetMacro(SingleInterfaceLayer, vtkTypeBool);
  vtkSetMacro(InterfaceLayerIndex, int);
  vtkGetMacro(InterfaceLayerIndex, int);
  vtkGetVector2Macro(InterfaceLayersRange, int);
  ///@}

  ///@{
  /**
   * The following methods allow selective reading of variables.
   * By default, ALL data variables on the nodes are read.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  ///@}

protected:
  vtkNetCDFCAMReader();
  ~vtkNetCDFCAMReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Returns true for success.  Based on the piece, number of pieces,
   * number of levels of cells, and the number of cells per level, gives
   * a partitioned space of levels and cells.
   */
  bool GetPartitioning(size_t piece, size_t numPieces, size_t numCellLevels,
    size_t numCellsPerLevel, size_t& beginCellLevel, size_t& endCellLevel, size_t& beginCell,
    size_t& endCell);

  void BuildVarArray();
  static void SelectionCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  vtkNetCDFCAMReader(const vtkNetCDFCAMReader&) = delete;
  void operator=(const vtkNetCDFCAMReader&) = delete;

  ///@{
  /**
   * The file name of the file that contains all of the point
   * data (coordinates and fields).
   */
  char* FileName;
  char* CurrentFileName;
  vtkSetStringMacro(CurrentFileName);
  ///@}

  ///@{
  /**
   * The file name that contains the cell connectivity information.
   */
  char* ConnectivityFileName;
  char* CurrentConnectivityFileName;
  vtkSetStringMacro(CurrentConnectivityFileName);
  ///@}

  int VerticalDimension;
  double* TimeSteps;
  size_t NumberOfTimeSteps;
  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

  vtkTypeBool SingleMidpointLayer;
  int MidpointLayerIndex;
  int MidpointLayersRange[2];

  vtkTypeBool SingleInterfaceLayer;
  int InterfaceLayerIndex;
  int InterfaceLayersRange[2];

  class Internal;
  Internal* Internals;
};

VTK_ABI_NAMESPACE_END
#endif
