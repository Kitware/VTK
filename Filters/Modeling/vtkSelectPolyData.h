/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectPolyData - select portion of polygonal mesh; generate selection scalars
// .SECTION Description
// vtkSelectPolyData is a filter that selects polygonal data based on
// defining a "loop" and indicating the region inside of the loop. The
// mesh within the loop consists of complete cells (the cells are not
// cut). Alternatively, this filter can be used to generate scalars.
// These scalar values, which are a distance measure to the loop, can
// be used to clip, contour. or extract data (i.e., anything that an
// implicit function can do).
//
// The loop is defined by an array of x-y-z point coordinates.
// (Coordinates should be in the same coordinate space as the input
// polygonal data.) The loop can be concave and non-planar, but not
// self-intersecting. The input to the filter is a polygonal mesh
// (only surface primitives such as triangle strips and polygons); the
// output is either a) a portion of the original mesh laying within
// the selection loop (GenerateSelectionScalarsOff); or b) the same
// polygonal mesh with the addition of scalar values
// (GenerateSelectionScalarsOn).
//
// The algorithm works as follows. For each point coordinate in the
// loop, the closest point in the mesh is found. The result is a loop
// of closest point ids from the mesh. Then, the edges in the mesh
// connecting the closest points (and laying along the lines forming
// the loop) are found. A greedy edge tracking procedure is used as
// follows. At the current point, the mesh edge oriented in the
// direction of and whose end point is closest to the line is
// chosen. The edge is followed to the new end point, and the
// procedure is repeated. This process continues until the entire loop
// has been created.
//
// To determine what portion of the mesh is inside and outside of the
// loop, three options are possible. 1) the smallest connected region,
// 2) the largest connected region, and 3) the connected region
// closest to a user specified point. (Set the ivar SelectionMode.)
//
// Once the loop is computed as above, the GenerateSelectionScalars
// controls the output of the filter. If on, then scalar values are
// generated based on distance to the loop lines. Otherwise, the cells
// laying inside the selection loop are output. By default, the mesh
// laying within the loop is output; however, if InsideOut is on, then
// the portion of the mesh laying outside of the loop is output.
//
// The filter can be configured to generate the unselected portions of
// the mesh as output by setting GenerateUnselectedOutput. Use the
// method GetUnselectedOutput to access this output. (Note: this flag
// is pertinent only when GenerateSelectionScalars is off.)

// .SECTION Caveats
// Make sure that the points you pick are on a connected surface. If
// not, then the filter will generate an empty or partial result. Also,
// self-intersecting loops will generate unpredictable results.
//
// During processing of the data, non-triangular cells are converted to
// triangles if GenerateSelectionScalars is off.

// .SECTION See Also
// vtkImplicitSelectionLoop

#ifndef vtkSelectPolyData_h
#define vtkSelectPolyData_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_INSIDE_SMALLEST_REGION 0
#define VTK_INSIDE_LARGEST_REGION 1
#define VTK_INSIDE_CLOSEST_POINT_REGION 2

class vtkCharArray;
class vtkPoints;
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkSelectPolyData : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Instantiate object with InsideOut turned off, and
  // GenerateSelectionScalars turned off. The unselected output
  // is not generated, and the inside mode is the smallest region.
  static vtkSelectPolyData *New();

  vtkTypeMacro(vtkSelectPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the flag to control behavior of the filter. If
  // GenerateSelectionScalars is on, then the output of the filter
  // is the same as the input, except that scalars are generated.
  // If off, the filter outputs the cells laying inside the loop, and
  // does not generate scalars.
  vtkSetMacro(GenerateSelectionScalars,int);
  vtkGetMacro(GenerateSelectionScalars,int);
  vtkBooleanMacro(GenerateSelectionScalars,int);

  // Description:
  // Set/Get the InsideOut flag. When off, the mesh within the loop is
  // extracted. When on, the mesh outside the loop is extracted.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description:
  // Set/Get the array of point coordinates defining the loop. There must
  // be at least three points used to define a loop.
  virtual void SetLoop(vtkPoints*);
  vtkGetObjectMacro(Loop,vtkPoints);

  // Description:
  // Set/Get the point used in SelectionModeToClosestPointRegion.
  vtkSetVector3Macro(ClosestPoint,double);
  vtkGetVector3Macro(ClosestPoint,double);

  // Description:
  // Control how inside/outside of loop is defined.
  vtkSetClampMacro(SelectionMode,int,
             VTK_INSIDE_SMALLEST_REGION,VTK_INSIDE_CLOSEST_POINT_REGION);
  vtkGetMacro(SelectionMode,int);
  void SetSelectionModeToSmallestRegion()
    {this->SetSelectionMode(VTK_INSIDE_SMALLEST_REGION);};
  void SetSelectionModeToLargestRegion()
    {this->SetSelectionMode(VTK_INSIDE_LARGEST_REGION);};
  void SetSelectionModeToClosestPointRegion()
    {this->SetSelectionMode(VTK_INSIDE_CLOSEST_POINT_REGION);};
  const char *GetSelectionModeAsString();

  // Description:
  // Control whether a second output is generated. The second output
  // contains the polygonal data that's not been selected.
  vtkSetMacro(GenerateUnselectedOutput,int);
  vtkGetMacro(GenerateUnselectedOutput,int);
  vtkBooleanMacro(GenerateUnselectedOutput,int);

  // Description:
  // Return output that hasn't been selected (if GenreateUnselectedOutput is
  // enabled).
  vtkPolyData *GetUnselectedOutput();

  // Description:
  // Return the (mesh) edges of the selection region.
  vtkPolyData *GetSelectionEdges();

  // Overload GetMTime() because we depend on Loop
  unsigned long int GetMTime();

protected:
  vtkSelectPolyData();
  ~vtkSelectPolyData();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int GenerateSelectionScalars;
  int InsideOut;
  vtkPoints *Loop;
  int SelectionMode;
  double ClosestPoint[3];
  int GenerateUnselectedOutput;

private:
  vtkPolyData *Mesh;
  void GetPointNeighbors (vtkIdType ptId, vtkIdList *nei);
private:
  vtkSelectPolyData(const vtkSelectPolyData&);  // Not implemented.
  void operator=(const vtkSelectPolyData&);  // Not implemented.
};

// Description:
// Return the method of determining in/out of loop as a string.
inline const char *vtkSelectPolyData::GetSelectionModeAsString(void)
{
  if ( this->SelectionMode == VTK_INSIDE_SMALLEST_REGION )
    {
    return "InsideSmallestRegion";
    }
  else if ( this->SelectionMode == VTK_INSIDE_LARGEST_REGION )
    {
    return "InsideLargestRegion";
    }
  else
    {
    return "InsideClosestPointRegion";
    }
}

#endif


