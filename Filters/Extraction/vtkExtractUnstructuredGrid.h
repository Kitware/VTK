/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractUnstructuredGrid - extract subset of unstructured grid geometry
// .SECTION Description
// vtkExtractUnstructuredGrid is a general-purpose filter to
// extract geometry (and associated data) from an unstructured grid
// dataset. The extraction process is controlled by specifying a range
// of point ids, cell ids, or a bounding box (referred to as "Extent").
// Those cells laying within these regions are sent to the output.
// The user has the choice of merging coincident points (Merging is on)
// or using the original point set (Merging is off).

// .SECTION Caveats
// If merging is off, the input points are copied through to the
// output. This means unused points may be present in the output data.
// If merging is on, then coincident points with different point attribute
// values are merged.

// .SECTION See Also
// vtkImageDataGeometryFilter vtkStructuredGridGeometryFilter
// vtkRectilinearGridGeometryFilter
// vtkExtractGeometry vtkExtractVOI

#ifndef __vtkExtractUnstructuredGrid_h
#define __vtkExtractUnstructuredGrid_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractUnstructuredGrid,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with all types of clipping turned off.
  static vtkExtractUnstructuredGrid *New();

  // Description:
  // Turn on/off selection of geometry by point id.
  vtkSetMacro(PointClipping,int);
  vtkGetMacro(PointClipping,int);
  vtkBooleanMacro(PointClipping,int);

  // Description:
  // Turn on/off selection of geometry by cell id.
  vtkSetMacro(CellClipping,int);
  vtkGetMacro(CellClipping,int);
  vtkBooleanMacro(CellClipping,int);

  // Description:
  // Turn on/off selection of geometry via bounding box.
  vtkSetMacro(ExtentClipping,int);
  vtkGetMacro(ExtentClipping,int);
  vtkBooleanMacro(ExtentClipping,int);

  // Description:
  // Specify the minimum point id for point id selection.
  vtkSetClampMacro(PointMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMinimum,vtkIdType);

  // Description:
  // Specify the maximum point id for point id selection.
  vtkSetClampMacro(PointMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMaximum,vtkIdType);

  // Description:
  // Specify the minimum cell id for point id selection.
  vtkSetClampMacro(CellMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMinimum,vtkIdType);

  // Description:
  // Specify the maximum cell id for point id selection.
  vtkSetClampMacro(CellMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMaximum,vtkIdType);

  // Description:
  // Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
  void SetExtent(double xMin, double xMax, double yMin, double yMax,
                 double zMin, double zMax);

  // Description:
  // Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
  void SetExtent(double extent[6]);
  double *GetExtent() { return this->Extent;};

  // Description:
  // Turn on/off merging of coincident points. Note that is merging is
  // on, points with different point attributes (e.g., normals) are merged,
  // which may cause rendering artifacts.
  vtkSetMacro(Merging,int);
  vtkGetMacro(Merging,int);
  vtkBooleanMacro(Merging,int);

  // Description:
  // Set / get a spatial locator for merging points. By
  // default an instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

  // Description:
  // Return the MTime also considering the locator.
  unsigned long GetMTime();

protected:
  vtkExtractUnstructuredGrid();
  ~vtkExtractUnstructuredGrid() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkIdType PointMinimum;
  vtkIdType PointMaximum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

  int Merging;
  vtkIncrementalPointLocator *Locator;
private:
  vtkExtractUnstructuredGrid(const vtkExtractUnstructuredGrid&);  // Not implemented.
  void operator=(const vtkExtractUnstructuredGrid&);  // Not implemented.
};

#endif


