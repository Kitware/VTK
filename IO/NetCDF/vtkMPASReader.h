/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPASReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2002-2005 Los Alamos National Laboratory

This software and ancillary information known as vtk_ext (and herein
called "SOFTWARE") is made available under the terms described below.
The SOFTWARE has been approved for release with associated LA_CC
Number 99-44, granted by Los Alamos National Laboratory in July 1999.

Unless otherwise indicated, this SOFTWARE has been authored by an
employee or employees of the University of California, operator of the
Los Alamos National Laboratory under Contract No. W-7405-ENG-36 with
the United States Department of Energy.

The United States Government has rights to use, reproduce, and
distribute this SOFTWARE.  The public may copy, distribute, prepare
derivative works and publicly display this SOFTWARE without charge,
provided that this Notice and any statement of authorship are
reproduced on all copies.

Neither the U. S. Government, the University of California, nor the
Advanced Computing Laboratory makes any warranty, either express or
implied, nor assumes any liability or responsibility for the use of
this SOFTWARE.

If SOFTWARE is modified to produce derivative works, such modified
SOFTWARE should be clearly marked, so as not to confuse it with the
version available from Los Alamos National Laboratory.

=========================================================================*/
/**
 * @class   vtkMPASReader
 * @brief   Read an MPAS netCDF file
 *
 * This program reads an MPAS netCDF data file to allow paraview to
 * display a dual-grid sphere or latlon projection.  Also allows
 * display of primal-grid sphere.
 * The variables that have time dim are available to ParaView.
 *
 * Assume all variables are of interest if they have dims
 * (Time, nCells|nVertices, nVertLevels, [nTracers]).
 * Does not deal with edge data.
 *
 * When using this reader, it is important that you remember to do the
 *following:
 *   1.  When changing a selected variable, remember to select it also
 *       in the drop down box to "color by".  It doesn't color by that variable
 *       automatically.
 *   2.  When selecting multilayer sphere view, make layer thickness around
 *       100,000.
 *   3.  When selecting multilayer lat/lon view, make layer thickness around 10.
 *   4.  Always click the -Z orientation after making a switch from lat/lon to
 *       sphere, from single to multilayer or changing thickness.
 *   5.  Be conservative on the number of changes you make before hitting Apply,
 *       since there may be bugs in this reader.  Just make one change and then
 *       hit Apply.
 *
 *
 * Christine Ahrens (cahrens@lanl.gov)
 * Version 1.3
*/

#ifndef vtkMPASReader_h
#define vtkMPASReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#include <string> // for std::string

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkDoubleArray;
class vtkStdString;
class vtkStringArray;

class VTKIONETCDF_EXPORT vtkMPASReader : public vtkUnstructuredGridAlgorithm
{
 public:
  static vtkMPASReader *New();
  vtkTypeMacro(vtkMPASReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify file name of MPAS data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get the number of data cells
   */
  vtkGetMacro(MaximumCells, int);
  //@}

  //@{
  /**
   * Get the number of points
   */
  vtkGetMacro(MaximumPoints, int);
  //@}

  //@{
  /**
   * Get the number of data variables at the cell centers and points
   */
  virtual int GetNumberOfCellVars();
  virtual int GetNumberOfPointVars();
  //@}

  //@{
  /**
   * Get the reader's output
   */
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int index);
  //@}

  //@{
  /**
   * If true, dimension info is included in the array name. For instance,
   * "tracers" will become "tracers(Time, nCells, nVertLevels, nTracers)".
   * This is useful for user-visible array selection, but is disabled by default
   * for backwards compatibility.
   */
  vtkSetMacro(UseDimensionedArrayNames, bool)
  vtkGetMacro(UseDimensionedArrayNames, bool)
  vtkBooleanMacro(UseDimensionedArrayNames, bool)
  //@}

  //@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  //@}

  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);
  void DisableAllCellArrays();
  void EnableAllCellArrays();

  //@{
  /**
   * If the point/cell arrays contain dimensions other than Time, nCells, or
   * nVertices, they are configured here. Use GetNumberOfDimensions to get the
   * number of arbitrary dimensions in the loaded arrays and GetDimensionName to
   * retrieve the dimension names. GetDimensionSize returns the number of values
   * in the dimensions, and Set/GetDimensionCurrentIndex controls the value
   * to fix a given dimension at when extracting slices of data.
   */
  vtkIdType GetNumberOfDimensions();
  std::string GetDimensionName(int idx);
  vtkStringArray* GetAllDimensions();
  int GetDimensionCurrentIndex(const std::string &dim);
  void SetDimensionCurrentIndex(const std::string &dim, int idx);
  int GetDimensionSize(const std::string &dim);
  //@}

  //@{
  /**
   * Get/Set the name to the dimension that identifies the vertical dimension.
   * Defaults to "nVertLevels".
   */
  vtkSetMacro(VerticalDimension, std::string)
  vtkGetMacro(VerticalDimension, std::string)
  //@}

  //@{
  /**
   * Convenience function for setting/querying [GS]etDimensionCurrentIndex
   * for the dimension returned by GetVerticalDimension.
   */
  void SetVerticalLevel(int level);
  int GetVerticalLevel();
  //@}

  vtkGetVector2Macro(VerticalLevelRange, int)

  vtkSetMacro(LayerThickness, int)
  vtkGetMacro(LayerThickness, int)
  vtkGetVector2Macro(LayerThicknessRange, int)

  void SetCenterLon(int val);
  vtkGetVector2Macro(CenterLonRange, int)

  vtkSetMacro(ProjectLatLon, bool)
  vtkGetMacro(ProjectLatLon, bool)

  vtkSetMacro(IsAtmosphere, bool)
  vtkGetMacro(IsAtmosphere, bool)

  vtkSetMacro(IsZeroCentered, bool)
  vtkGetMacro(IsZeroCentered, bool)

  vtkSetMacro(ShowMultilayerView, bool)
  vtkGetMacro(ShowMultilayerView, bool)

  /**
   * Returns true if the given file can be read.
   */
  static int CanReadFile(const char *filename);

  vtkMTimeType GetMTime() override;

 protected:
  vtkMPASReader();
  ~vtkMPASReader() override;
  void ReleaseNcData();
  void DestroyData();

  char *FileName;         // First field part file giving path

  size_t NumberOfTimeSteps;   // Temporal domain
  double DTime;               // The current time

  // Observer to modify this object when array selections are modified
  vtkCallbackCommand* SelectionObserver;

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                         vtkInformationVector *) override;


  static void SelectionCallback(vtkObject* caller, unsigned long eid,
                                void* clientdata, void* calldata);

  // Selected field of interest
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  /**
   * Update the list of available dimensions. Only does work when
   * PointDataArraySelection or CellDataArraySelection is changed.
   */
  void UpdateDimensions(bool force = false);

  std::string VerticalDimension;
  int VerticalLevelRange[2];

  int LayerThickness;
  int LayerThicknessRange[2];

  int CenterLon;
  int CenterLonRange[2];

  enum GeometryType
  {
    Spherical,
    Projected,
    Planar
  };

  GeometryType Geometry;

  bool ProjectLatLon; // User option
  bool OnASphere; // Data file attribute
  bool IsAtmosphere;
  bool IsZeroCentered;
  bool ShowMultilayerView;

  bool IncludeTopography;
  bool DoBugFix;
  double CenterRad;

  bool UseDimensionedArrayNames;

  // geometry
  size_t MaximumNVertLevels;
  size_t NumberOfCells;
  size_t NumberOfPoints;
  int CellOffset;
  size_t PointOffset;
  size_t PointsPerCell;
  size_t CurrentExtraPoint;  // current extra point
  size_t CurrentExtraCell;   // current extra cell
  double* PointX;      // x coord of point
  double* PointY;      // y coord of point
  double* PointZ;      // z coord of point
  size_t ModNumPoints;
  size_t ModNumCells;
  int* OrigConnections;   // original connections
  int* ModConnections;    // modified connections
  size_t* CellMap;        // maps from added cell to original cell #
  size_t* PointMap;       // maps from added point to original point #
  int* MaximumLevelPoint;      //
  int MaximumCells;           // max cells
  int MaximumPoints;          // max points

  void SetDefaults();
  int GetNcDims();
  int GetNcAtts();
  int CheckParams();
  int GetNcVars(const char* cellDimName, const char* pointDimName);
  int ReadAndOutputGrid();
  int BuildVarArrays();
  int AllocSphericalGeometry();
  int AllocProjectedGeometry();
  int AllocPlanarGeometry();
  void ShiftLonData();
  int AddMirrorPoint(int index, double dividerX, double offset);
  void FixPoints();
  int EliminateXWrap();
  void OutputPoints();
  void OutputCells();
  unsigned char GetCellType();

  vtkDataArray* LoadPointVarData(int variable);
  vtkDataArray* LoadCellVarData(int variable);
  vtkDataArray* LookupPointDataArray(int varIdx);
  vtkDataArray* LookupCellDataArray(int varIdx);

  /**
   * Update the "Time" vtkStringArray in dataset's FieldData to contain the
   * xtime string for the current timestep.
   * If there is an error getting xtime, the current timestep number is inserted
   * instead.
   * If a non-string array named Time already exists in the FieldData, dataset
   * is not modified in any way.
   */
  void LoadTimeFieldData(vtkUnstructuredGrid *dataset);

 private:
  vtkMPASReader(const vtkMPASReader&) = delete;
  void operator=(const vtkMPASReader&) = delete;

  class Internal;
  Internal *Internals;
};

#endif
