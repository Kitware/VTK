/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkClipVolume - clip volume data with user-specified implicit function or input scalar data
// .SECTION Description
// vtkClipVolume is a filter that clips volume data (i.e., structured points)
// using either: any subclass of vtkImplicitFunction; or the input scalar
// data. Clipping means that it actually "cuts" through the cells of the
// dataset, returning everything inside of the specified implicit function (or
// greater than the scalar value) including "pieces" of a cell. (Compare this
// with vtkExtractGeometry or vtkGeometryFilter, which pulls out entire,
// uncut cells.) The output of this filter is a 3D unstructured grid (e.g.,
// tetrahedra).
//
// To use this filter, you must decide if you will be clipping with an
// implicit function, or whether you will be using the input scalar data.  If
// you want to clip with an implicit function, you must first define and then
// set the implicit function with the SetClipFunction() method. Otherwise,
// you must make sure input scalar data is available. You can also specify a
// scalar value, which is used to decide what is inside and outside of the
// implicit function. You can also reverse the sense of what inside/outside
// is by setting the InsideOut instance variable. (The cutting algorithm
// proceeds by computing an implicit function value or using the input scalar
// data for each point in the dataset. This is compared to the scalar value
// to determine inside/outside.)
//
// This filter can be configured to compute a second output. The
// second output is the portion of the volume that is clipped away. Set the
// GenerateClippedData boolean on if you wish to access this output data.

// .SECTION Caveats
// This filter is designed to function with 3D structured points. Clipping
// 2D images can be better done by converting the image to polygonal data
// and using vtkClipPolyData,

// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData vtkGeometryFilter vtkExtractGeometry

#ifndef __vtkClipVolume_h
#define __vtkClipVolume_h

#include "vtkStructuredPointsToUnstructuredGridFilter.h"
#include "vtkImplicitFunction.h"

class vtkMergePoints;
class vtkOrderedTriangulator;

class VTK_EXPORT vtkClipVolume : public vtkStructuredPointsToUnstructuredGridFilter
{
public:
  vtkTypeMacro(vtkClipVolume,vtkStructuredPointsToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; InsideOut turned off;
  // value set to 0.0; and generate clip scalars turned off. The merge
  // tolerance is set to 0.01.
  static vtkClipVolume *New();

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or scalar value (if clipping with scalars). The
  // default value is 0.0.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);
  
  // Description:
  // Set/Get the InsideOut flag. When off, a vertex is considered inside the
  // implicit function if its value is greater than the Value ivar. When
  // InsideOutside is turned on, a vertex is considered inside the implicit
  // function if its implicit function value is less than or equal to the
  // Value ivar.  InsideOut is off by default.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

  // Description
  // Specify the implicit function with which to perform the clipping. If you
  // do not define an implicit function, then the input scalar data will be
  // used for clipping.
  vtkSetObjectMacro(ClipFunction,vtkImplicitFunction);
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
  // contains the unstructured grid that's been clipped away.
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);

  // Description:
  // Return the clipped output.
  vtkUnstructuredGrid *GetClippedOutput();

  // Description:
  // Set the tolerance for merging clip intersection points that are near
  // the corners of voxels. This tolerance is used to prevent the generation
  // of degenerate tetrahedra.
  vtkSetClampMacro(MergeTolerance,float,0.0001,0.25);
  vtkGetMacro(MergeTolerance,float);
  
  // Description:
  // Set / Get a spatial locator for merging points. By default, 
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Return the mtime also considering the locator and clip function.
  unsigned long int GetMTime();

protected:
  vtkClipVolume(vtkImplicitFunction *cf=NULL);
  ~vtkClipVolume();
  vtkClipVolume(const vtkClipVolume&);
  void operator=(const vtkClipVolume&);

  void Execute();
  void ClipVoxel(float value, vtkDataArray *cellScalars, int flip,
                 float origin[3], float spacing[3], vtkIdList *cellIds,
		 vtkPoints *cellPts, vtkPointData *inPD, vtkPointData *outPD,
		 vtkCellData *inCD, vtkIdType cellId, vtkCellData *outCD, 
		 vtkCellData *clippedCD);
  vtkImplicitFunction *ClipFunction;
  vtkPointLocator *Locator;
  int InsideOut;
  float Value;
  int GenerateClipScalars;
  float MergeTolerance;

  int GenerateClippedOutput;
  vtkUnstructuredGrid *ClippedOutput;
  
private:
  vtkUnstructuredGrid    *Mesh;
  vtkOrderedTriangulator *Triangulator;
  
};

#endif
