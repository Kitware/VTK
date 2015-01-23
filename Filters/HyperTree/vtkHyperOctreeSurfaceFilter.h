/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctreeSurfaceFilter - Extracts outer (polygonal) surface.
// .SECTION Description
// vtkHyperOctreeSurfaceFilter extracts the surface of an hyperoctree.

// .SECTION See Also
// vtkGeometryFilter vtkStructuredGridGeometryFilter.

#ifndef vtkHyperOctreeSurfaceFilter_h
#define vtkHyperOctreeSurfaceFilter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkHyperOctreeCursor;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkIncrementalPointLocator;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperOctreeSurfaceFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkHyperOctreeSurfaceFilter *New();
  vtkTypeMacro(vtkHyperOctreeSurfaceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkHyperOctreeSurfaceFilter();
  ~vtkHyperOctreeSurfaceFilter();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  void GenerateLines(double bounds[2],
                     vtkIdType ptIds[2]);
  void GenerateQuads(double bounds[4],
                     vtkIdType ptIds[4]);
  void GenerateFaces(double bounds[6],
                     vtkIdType ptIds[8],
                     int onFace[6]);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

  int Merging;
  vtkIncrementalPointLocator *Locator;

  // Variables used by generate recursively.
  // It avoids to pass to much argument.
  vtkDataSetAttributes *InputCD;

  vtkHyperOctreeCursor *Cursor;
  vtkPoints *OutPts;
  vtkCellArray *OutCells;
  vtkCellData *OutputCD;

  int PassThroughCellIds;
  void RecordOrigCellId(vtkIdType destIndex, vtkIdType originalId);
  vtkIdTypeArray *OriginalCellIds;

private:
  vtkHyperOctreeSurfaceFilter(const vtkHyperOctreeSurfaceFilter&);  // Not implemented.
  void operator=(const vtkHyperOctreeSurfaceFilter&);  // Not implemented.
};

#endif
