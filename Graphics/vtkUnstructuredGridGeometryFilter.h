/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridGeometryFilter - extract geometry from an unstructured grid
// .SECTION Description
// vtkUnstructuredGridGeometryFilter is a filter that extracts
// geometry (and associated data) from an unstructured grid. It differs from
// vtkGeometryFilter by not tessellating higher order faces: 2D faces of
// quadratic 3D cells will be quadratic. A quadratic edge is extracted as a
// quadratic edge. For that purpose, the output of this filter is an
// unstructured grid, not a polydata.
// Also, the face of a voxel is a pixel, not a quad.
// Geometry is obtained as follows: all 0D, 1D, and 2D cells are extracted.
// All 2D faces that are used by only one 3D cell (i.e., boundary faces) are
// extracted. It also is possible to specify conditions on point ids, cell ids,
// and on bounding box (referred to as "Extent") to control the extraction
// process.
//
// .SECTION Caveats
// When vtkUnstructuredGridGeometryFilter extracts cells (or boundaries of
// cells) it will (by default) merge duplicate vertices. This may cause
// problems in some cases. Turn merging off to prevent this from occurring.

// .SECTION See Also
// vtkGeometryFilter

#ifndef __vtkUnstructuredGridGeometryFilter_h
#define __vtkUnstructuredGridGeometryFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;
class vtkHashTableOfSurfels; // internal class

class VTK_GRAPHICS_EXPORT vtkUnstructuredGridGeometryFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkUnstructuredGridGeometryFilter *New();
  vtkTypeMacro(vtkUnstructuredGridGeometryFilter,vtkUnstructuredGridAlgorithm);
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

protected:
  vtkUnstructuredGridGeometryFilter();
  ~vtkUnstructuredGridGeometryFilter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

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
  
  vtkHashTableOfSurfels *HashTable;
  
private:
  vtkUnstructuredGridGeometryFilter(const vtkUnstructuredGridGeometryFilter&);  // Not implemented.
  void operator=(const vtkUnstructuredGridGeometryFilter&);  // Not implemented.
};

#endif
