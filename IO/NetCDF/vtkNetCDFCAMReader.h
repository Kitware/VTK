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
 * Reads in a NetCDF CAM (Community Atmospheric Model) file and produces
 * and unstructured grid.  The grid is actually unstructured in the
 * X and Y directions and rectilinear in the Z direction with all
 * hex cells.  The reader requires 2 NetCDF files.  The first is the
 * cell connectivity file which has the quad connectivity in the plane.
 * The other connectivity file has all of the point and field information.
 * Currently this reader ignores time that may exist in the points
 * file.
*/

#ifndef vtkNetCDFCAMReader_h
#define vtkNetCDFCAMReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class NcFile;
class vtkCallbackCommand;
class vtkDataArraySelection;

class VTKIONETCDF_EXPORT vtkNetCDFCAMReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNetCDFCAMReader *New();
  vtkTypeMacro(vtkNetCDFCAMReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
   * of (time, ncols).  This will result in a surface grid. Otherwise
   * a volumetric grid will be created (if lev > 1) and the variables
   * with dimensions of (time, lev, ncols) will be read in.
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
   * Set whether to read a single level, layer midpoints or layer
   * interface levels.
   * VERTICAL_DIMENSION_SINGLE_LAYER (0) indicates that only a single
   * layer will be read in. The NetCDF variables loaded will be the
   * ones with dimensions (time, ncols).
   * VERTICAL_DIMENSION_LAYER_MIDPOINTS (1) indicates that variables defined
   * on layer midpoints will be read in. These are variables with dimensions
   * (time, lev, ncols).
   * VERTICAL_DIMENSION_LAYER_INTERFACE (2) indicates that variables
   * defined on layer interfaces will be read in. These are variables with
   * dimensions (time, ilev, ncols).
   */
  enum VerticalDimension
  {
    VERTICAL_DIMENSION_SINGLE_LEVEL,
    VERTICAL_DIMENSION_LAYER_MIDPOINTS,
    VERTICAL_DIMENSION_LAYER_INTERFACE,
    VERTICAL_DIMENSION_COUNT
  };
  vtkSetClampMacro(VerticalDimension, int, 0, 2);
  vtkGetMacro(VerticalDimension, int);
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
  ~vtkNetCDFCAMReader();

  int RequestInformation(vtkInformation*, vtkInformationVector**,
                         vtkInformationVector*);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

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
