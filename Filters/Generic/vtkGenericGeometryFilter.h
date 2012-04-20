/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericGeometryFilter - extract geometry from data (or convert data to polygonal type)
// .SECTION Description
// vtkGenericGeometryFilter is a general-purpose filter to extract geometry (and
// associated data) from any type of dataset. Geometry is obtained as
// follows: all 0D, 1D, and 2D cells are extracted. All 2D faces that are
// used by only one 3D cell (i.e., boundary faces) are extracted. It also is
// possible to specify conditions on point ids, cell ids, and on
// bounding box (referred to as "Extent") to control the extraction process.
//
// This filter also may be used to convert any type of data to polygonal
// type. The conversion process may be less than satisfactory for some 3D
// datasets. For example, this filter will extract the outer surface of a
// volume or structured grid dataset. (For structured data you may want to
// use vtkImageDataGeometryFilter, vtkStructuredGridGeometryFilter,
// vtkExtractUnstructuredGrid, vtkRectilinearGridGeometryFilter, or
// vtkExtractVOI.)

// .SECTION Caveats
// When vtkGenericGeometryFilter extracts cells (or boundaries of cells) it
// will (by default) merge duplicate vertices. This may cause problems
// in some cases. For example, if you've run vtkPolyDataNormals to
// generate normals, which may split meshes and create duplicate
// vertices, vtkGenericGeometryFilter will merge these points back
// together. Turn merging off to prevent this from occurring.

// .SECTION See Also
// vtkImageDataGeometryFilter vtkStructuredGridGeometryFilter
// vtkExtractGeometry vtkExtractVOI

#ifndef __vtkGenericGeometryFilter_h
#define __vtkGenericGeometryFilter_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkIncrementalPointLocator;
class vtkPointData;

class VTKFILTERSGENERIC_EXPORT vtkGenericGeometryFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkGenericGeometryFilter *New();
  vtkTypeMacro(vtkGenericGeometryFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkSetClampMacro(PointMinimum,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(PointMinimum,vtkIdType);

  // Description:
  // Specify the maximum point id for point id selection.
  vtkSetClampMacro(PointMaximum,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(PointMaximum,vtkIdType);

  // Description:
  // Specify the minimum cell id for point id selection.
  vtkSetClampMacro(CellMinimum,vtkIdType,0,VTK_LARGE_ID);
  vtkGetMacro(CellMinimum,vtkIdType);

  // Description:
  // Specify the maximum cell id for point id selection.
  vtkSetClampMacro(CellMaximum,vtkIdType,0,VTK_LARGE_ID);
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

  // Description:
  // If on, the output polygonal dataset will have a celldata array that
  // holds the cell index of the original 3D cell that produced each output
  // cell. This is useful for cell picking. The default is off to conserve
  // memory.
  vtkSetMacro(PassThroughCellIds,int);
  vtkGetMacro(PassThroughCellIds,int);
  vtkBooleanMacro(PassThroughCellIds,int);

protected:
  vtkGenericGeometryFilter();
  ~vtkGenericGeometryFilter();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void PolyDataExecute(); //special cases for performance
  void UnstructuredGridExecute();
  void StructuredGridExecute();
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int FillInputPortInformation(int, vtkInformation*);

  vtkIdType PointMaximum;
  vtkIdType PointMinimum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

  int Merging;
  vtkIncrementalPointLocator *Locator;

  // Used internal by vtkGenericAdaptorCell::Tessellate()
  vtkPointData *InternalPD;

  int PassThroughCellIds;

private:
  vtkGenericGeometryFilter(const vtkGenericGeometryFilter&);  // Not implemented.
  void operator=(const vtkGenericGeometryFilter&);  // Not implemented.
};

#endif


