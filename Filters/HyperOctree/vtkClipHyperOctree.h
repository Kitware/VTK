/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipHyperOctree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClipHyperOctree - clip an hyperoctree with user-specified implicit function or input scalar data
// .SECTION Description
// vtkClipHyperOctree is a filter that clips an hyperoctree using either
// any subclass of vtkImplicitFunction, or the input scalar
// data. Clipping means that it actually "cuts" through the leaves (cells) of
// the hyperoctree, returning everything inside of the specified implicit
// function (or greater than the scalar value) including "pieces" of
// a cell. (Compare this with vtkExtractGeometry, which pulls out
// entire, uncut cells.) The output of this filter is an unstructured
// grid.
//
// To use this filter, you must decide if you will be clipping with an
// implicit function, or whether you will be using the input scalar
// data.  If you want to clip with an implicit function, you must:
// 1) define an implicit function
// 2) set it with the SetClipFunction method
// 3) apply the GenerateClipScalarsOn method
// If a ClipFunction is not specified, or GenerateClipScalars is off
// (the default), then the input's scalar data will be used to clip
// the polydata.
//
// You can also specify a scalar value, which is used to decide what is
// inside and outside of the implicit function. You can also reverse the
// sense of what inside/outside is by setting the InsideOut instance
// variable. (The clipping algorithm proceeds by computing an implicit
// function value or using the input scalar data for each point in the
// dataset. This is compared to the scalar value to determine
// inside/outside.)
//
// This filter can be configured to compute a second output. The
// second output is the part of the cell that is clipped away. Set the
// GenerateClippedData boolean on if you wish to access this output data.

// .SECTION Caveats
// vtkClipHyperOctree will triangulate all types of 3D cells (i.e., create
// tetrahedra). This is true even if the cell is not actually cut. This
// is necessary to preserve compatibility across face neighbors. 2D cells
// will only be triangulated if the cutting function passes through them.

// .SECTION See Also
// vtkImplicitFunction vtkCutter vtkClipVolume vtkClipPolyData

#ifndef __vtkClipHyperOctree_h
#define __vtkClipHyperOctree_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkImplicitFunction;
class vtkIncrementalPointLocator;
class vtkHyperOctreeCursor;
class vtkHyperOctree;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
class vtkCellArray;
class vtkCellData;
class vtkPointData;
class vtkOrderedTriangulator;
class vtkDoubleArray;
class vtkTetra;
class vtkPoints;
class vtkPolygon;
class vtkHyperOctreeClipCutPointsGrabber;

class VTK_GRAPHICS_EXPORT vtkClipHyperOctree : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkClipHyperOctree,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; InsideOut turned off;
  // value set to 0.0; and generate clip scalars turned off.
  static vtkClipHyperOctree *New();

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or scalar value (if clipping with
  // scalars). The default value is 0.0. 
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);
  
  // Description:
  // Set/Get the InsideOut flag. When off, a vertex is considered
  // inside the implicit function if its value is greater than the
  // Value ivar. When InsideOutside is turned on, a vertex is
  // considered inside the implicit function if its implicit function
  // value is less than or equal to the Value ivar.  InsideOut is off
  // by default.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description
  // Specify the implicit function with which to perform the
  // clipping. If you do not define an implicit function, 
  // then the selected input scalar data will be used for clipping.
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be 
  // interpolated from the implicit function values, and not the 
  // input scalar data. If you enable this flag but do not provide an
  // implicit function an error will be reported.
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);

  // Description:
  // Control whether a second output is generated. The second output
  // contains the polygonal data that's been clipped away.
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);
  
  // Description:
  // Return the Clipped output.
  vtkUnstructuredGrid *GetClippedOutput();

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Return the mtime also considering the locator and clip function.
  unsigned long GetMTime();

protected:
  vtkClipHyperOctree(vtkImplicitFunction *cf=NULL);
  ~vtkClipHyperOctree();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  // Description:
  // Clip the sub-hierarchy pointed by cursor.
  // \pre cursor_exists: cursor!=0
  // \pre positive_level: level>=0
  void ClipNode(vtkHyperOctreeCursor *cursor,
                int level,
                double bounds[6]);
  
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  vtkImplicitFunction *ClipFunction;
  
  vtkIncrementalPointLocator *Locator;
  vtkIncrementalPointLocator *Locator2; // used for the clipped output
  
  int InsideOut;
  double Value;
  int GenerateClipScalars;

  int GenerateClippedOutput;
  
  vtkHyperOctree *Input;
  vtkUnstructuredGrid *Output;
  vtkUnstructuredGrid *ClippedOutput;
  
  vtkUnsignedCharArray *Types[2];
  vtkIdTypeArray *Locs[2];
  vtkCellArray *Conn[2];
  vtkCellData *InCD;
  vtkCellData *OutCD[2];
  vtkPointData *OutPD[2];
  vtkOrderedTriangulator *Triangulator;
  
  vtkHyperOctreeCursor *Sibling; // to avoid allocation in the loop
  
  vtkDoubleArray *CellScalars;
  vtkTetra *Tetra;
  vtkDoubleArray *TetScalars;
  
  vtkPoints *Pts;
  vtkPolygon *Polygon;
  
  vtkIdType CellTypeCounter[65536]; // up-to-65536 points per octant
  vtkIdType TotalCounter;
  vtkIdType TemplateCounter; // record the number of octants that succceed
  // to use the template triangulator
  
  vtkHyperOctreeClipCutPointsGrabber *Grabber;
  
private:
  vtkClipHyperOctree(const vtkClipHyperOctree&);  // Not implemented.
  void operator=(const vtkClipHyperOctree&);  // Not implemented.
};

#endif
