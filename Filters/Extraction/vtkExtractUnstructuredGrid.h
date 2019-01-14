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
/**
 * @class   vtkExtractUnstructuredGrid
 * @brief   extract subset of unstructured grid geometry
 *
 * vtkExtractUnstructuredGrid is a general-purpose filter to
 * extract geometry (and associated data) from an unstructured grid
 * dataset. The extraction process is controlled by specifying a range
 * of point ids, cell ids, or a bounding box (referred to as "Extent").
 * Those cells laying within these regions are sent to the output.
 * The user has the choice of merging coincident points (Merging is on)
 * or using the original point set (Merging is off).
 *
 * @warning
 * If merging is off, the input points are copied through to the
 * output. This means unused points may be present in the output data.
 * If merging is on, then coincident points with different point attribute
 * values are merged.
 *
 * @sa
 * vtkImageDataGeometryFilter vtkStructuredGridGeometryFilter
 * vtkRectilinearGridGeometryFilter
 * vtkExtractGeometry vtkExtractVOI
*/

#ifndef vtkExtractUnstructuredGrid_h
#define vtkExtractUnstructuredGrid_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIncrementalPointLocator;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractUnstructuredGrid,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with all types of clipping turned off.
   */
  static vtkExtractUnstructuredGrid *New();

  //@{
  /**
   * Turn on/off selection of geometry by point id.
   */
  vtkSetMacro(PointClipping,vtkTypeBool);
  vtkGetMacro(PointClipping,vtkTypeBool);
  vtkBooleanMacro(PointClipping,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry by cell id.
   */
  vtkSetMacro(CellClipping,vtkTypeBool);
  vtkGetMacro(CellClipping,vtkTypeBool);
  vtkBooleanMacro(CellClipping,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off selection of geometry via bounding box.
   */
  vtkSetMacro(ExtentClipping,vtkTypeBool);
  vtkGetMacro(ExtentClipping,vtkTypeBool);
  vtkBooleanMacro(ExtentClipping,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify the minimum point id for point id selection.
   */
  vtkSetClampMacro(PointMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMinimum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum point id for point id selection.
   */
  vtkSetClampMacro(PointMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(PointMaximum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the minimum cell id for point id selection.
   */
  vtkSetClampMacro(CellMinimum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMinimum,vtkIdType);
  //@}

  //@{
  /**
   * Specify the maximum cell id for point id selection.
   */
  vtkSetClampMacro(CellMaximum,vtkIdType,0,VTK_ID_MAX);
  vtkGetMacro(CellMaximum,vtkIdType);
  //@}

  /**
   * Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double xMin, double xMax, double yMin, double yMax,
                 double zMin, double zMax);

  //@{
  /**
   * Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
   */
  void SetExtent(double extent[6]);
  double *GetExtent() VTK_SIZEHINT(6) { return this->Extent;};
  //@}

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging,vtkTypeBool);
  vtkGetMacro(Merging,vtkTypeBool);
  vtkBooleanMacro(Merging,vtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By
   * default an instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator();

  /**
   * Return the MTime also considering the locator.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkExtractUnstructuredGrid();
  ~vtkExtractUnstructuredGrid() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  vtkIdType PointMinimum;
  vtkIdType PointMaximum;
  vtkIdType CellMinimum;
  vtkIdType CellMaximum;
  double Extent[6];
  vtkTypeBool PointClipping;
  vtkTypeBool CellClipping;
  vtkTypeBool ExtentClipping;

  vtkTypeBool Merging;
  vtkIncrementalPointLocator *Locator;
private:
  vtkExtractUnstructuredGrid(const vtkExtractUnstructuredGrid&) = delete;
  void operator=(const vtkExtractUnstructuredGrid&) = delete;
};

#endif


