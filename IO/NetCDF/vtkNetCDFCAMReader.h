/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtk_netcdfcpp_fwd.h" // Forward declarations for vtknetcdfcpp

class vtkCallbackCommand;
class vtkDataArraySelection;

class VTKIONETCDF_EXPORT vtkNetCDFCAMReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNetCDFCAMReader *New();
  vtkTypeMacro(vtkNetCDFCAMReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns 1 if this file can be read and 0 if the file cannot be read.
   * Because NetCDF CAM files come in pairs and we only check one of the
   * files, the result is not definitive.  Invalid files may still return 1
   * although a valid file will never return 0.
   */
  static int CanReadFile(const char* fileName);

  void SetFileName(const char* fileName);
  vtkGetStringMacro(FileName);

  void SetConnectivityFileName(const char* fileName);
  vtkGetStringMacro(ConnectivityFileName);

  //@{
  /**
   * Set whether or not to read a single level.  A
   * value of one indicates that only a single level will be read in.
   * The NetCDF variables loaded will then be ones with dimensions
   * of (time, ncol).  This will result in a surface grid. Otherwise
   * a volumetric grid will be created (if lev > 1) and the variables
   * with dimensions of (time, lev, ncol) will be read in.
   * By default, SingleLevel = 0.
   * @deprecated in VTK 7.1 use SetVerticalDimension or
   *             GetVerticalDimension instead.
   */
  VTK_LEGACY(virtual void SingleLevelOn ());
  VTK_LEGACY(virtual void SingleLevelOff ());
  VTK_LEGACY(virtual void SetSingleLevel (int level));
  VTK_LEGACY(virtual int GetSingleLevel ());
  //@}

  //@{
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
  //@}

  //@{
  /**
   * If SingleXXXLayer is 1, we'll load only the layer specified by
   * XXXLayerIndex.  Otherwise, we load all layers. We do that for
   * midpoint layer variables ( which have dimension 'lev') or for
   * interface layer variables (which have dimension 'ilev').
   */
  vtkBooleanMacro(SingleMidpointLayer, int);
  vtkSetMacro(SingleMidpointLayer, int);
  vtkGetMacro(SingleMidpointLayer, int);
  vtkSetMacro(MidpointLayerIndex, int);
  vtkGetMacro(MidpointLayerIndex, int);
  vtkGetVector2Macro(MidpointLayersRange, int);

  vtkBooleanMacro(SingleInterfaceLayer, int);
  vtkSetMacro(SingleInterfaceLayer, int);
  vtkGetMacro(SingleInterfaceLayer, int);
  vtkSetMacro(InterfaceLayerIndex, int);
  vtkGetMacro(InterfaceLayerIndex, int);
  vtkGetVector2Macro(InterfaceLayersRange, int);
  //@}

  //@{
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
  //@}


  //@{
  /**
   * Specify which "side" of the domain to add the connecting
   * cells at.  0 indicates left side and 1 indicates right side.
   * The default is the right side.
   * @deprecated This method is no longer supported. The reader automatically
   * decides which side to pad cells on. Using this method has no effect.
   */
  VTK_LEGACY(void SetCellLayerRight(int));
  VTK_LEGACY(int GetCellLayerRight());
  //@}

protected:
  vtkNetCDFCAMReader();
  ~vtkNetCDFCAMReader() VTK_OVERRIDE;

  int RequestInformation(vtkInformation*, vtkInformationVector**,
                         vtkInformationVector*) VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;

  /**
   * Returns true for success.  Based on the piece, number of pieces,
   * number of levels of cells, and the number of cells per level, gives
   * a partitioned space of levels and cells.
   */
  bool GetPartitioning(
    int piece, int numPieces,int numCellLevels, int numCellsPerLevel,
    int & beginCellLevel, int & endCellLevel, int & beginCell, int & endCell);

  void BuildVarArray();
  static void SelectionCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);


private:
  vtkNetCDFCAMReader(const vtkNetCDFCAMReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNetCDFCAMReader&) VTK_DELETE_FUNCTION;

  //@{
  /**
   * The file name of the file that contains all of the point
   * data (coordinates and fields).
   */
  char* FileName;
  char* CurrentFileName;
  vtkSetStringMacro(CurrentFileName);
  //@}

  //@{
  /**
   * The file name that contains the cell connectivity information.
   */
  char* ConnectivityFileName;
  char* CurrentConnectivityFileName;
  vtkSetStringMacro(CurrentConnectivityFileName);
  //@}

  int VerticalDimension;
  double * TimeSteps;
  long NumberOfTimeSteps;
  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

  int SingleMidpointLayer;
  int MidpointLayerIndex;
  int MidpointLayersRange[2];

  int SingleInterfaceLayer;
  int InterfaceLayerIndex;
  int InterfaceLayersRange[2];


  //@{
  /**
   * The NetCDF file descriptors.  NULL indicates they haven't
   * been opened.
   */
  NcFile* PointsFile;
  NcFile* ConnectivityFile;
};
  //@}

#endif
