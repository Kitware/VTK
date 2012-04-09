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
// .NAME vtkNetCDFCAMReader - Read unstructured NetCDF CAM files.
// .SECTION Description
// Reads in a NetCDF CAM (Community Atmospheric Model) file and produces
// and unstructured grid.  The grid is actually unstructured in the
// X and Y directions and rectilinear in the Z direction with all
// hex cells.  The reader requires 2 NetCDF files.  The first is the
// cell connectivity file which has the quad connectivity in the plane.
// The other connectivity file has all of the point and field information.
// Currently this reader ignores time that may exist in the points
// file.

#ifndef __vtkNetCDFCAMReader_h
#define __vtkNetCDFCAMReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class NcFile;

class VTKIONETCDF_EXPORT vtkNetCDFCAMReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkNetCDFCAMReader *New();
  vtkTypeMacro(vtkNetCDFCAMReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns 1 if this file can be read and 0 if the file cannot be read.
  // Because NetCDF CAM files come in pairs and we only check one of the
  // files, the result is not definitive.  Invalid files may still return 1
  // although a valid file will never return 0.
  static int CanReadFile(const char* fileName);

  void SetFileName(const char* fileName);
  vtkGetStringMacro(FileName);

  void SetConnectivityFileName(const char* fileName);
  vtkGetStringMacro(ConnectivityFileName);

  // Description:
  // Set whether or not to read a single level.  A
  // value of one indicates that only a single level will be read in.
  // The NetCDF variables loaded will then be ones with dimensions
  // of (time, ncols).  This will result in a surface grid. Otherwise
  // a volumetric grid will be created (if lev > 1) and the variables
  // with dimensions of (time, lev, ncols) will be read in.
  // By default, SingleLevel = 0.
  vtkBooleanMacro(SingleLevel,int);
  vtkSetClampMacro(SingleLevel, int, 0, 1);
  vtkGetMacro(SingleLevel, int);

  // Description:
  // Specify which "side" of the domain to add the connecting
  // cells at.  0 indicates left side and 1 indicates right side.
  // The default is the right side.
  vtkSetMacro(CellLayerRight, int);
  vtkGetMacro(CellLayerRight, int);

protected:
  vtkNetCDFCAMReader();
  ~vtkNetCDFCAMReader();

  int RequestInformation(vtkInformation*, vtkInformationVector**,
                         vtkInformationVector*);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  // Description:
  // Returns true for success.  Based on the piece, number of pieces,
  // number of levels of cells, and the number of cells per level, gives
  // a partitioned space of levels and cells.
  bool GetPartitioning(
    int piece, int numPieces,int numCellLevels, int numCellsPerLevel,
    int & beginCellLevel, int & endCellLevel, int & beginCell, int & endCell);

private:
  vtkNetCDFCAMReader(const vtkNetCDFCAMReader&);  // Not implemented.
  void operator=(const vtkNetCDFCAMReader&);  // Not implemented.

  // Description:
  // The file name of the file that contains all of the point
  // data (coordinates and fields).
  char* FileName;
  char* CurrentFileName;
  vtkSetStringMacro(CurrentFileName);

  // Description:
  // The file name that contains the cell connectivity information.
  char* ConnectivityFileName;
  char* CurrentConnectivityFileName;
  vtkSetStringMacro(CurrentConnectivityFileName);

  int SingleLevel;

  int CellLayerRight;

  double * TimeSteps;

  long NumberOfTimeSteps;

  // Description:
  // The NetCDF file descriptors.  NULL indicates they haven't
  // been opened.
  NcFile* PointsFile;
  NcFile* ConnectivityFile;
};

#endif
