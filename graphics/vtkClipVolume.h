/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkClipVolume - clip volume data with user-specified implicit function or input scalar data
// .SECTION Description
// vtkClipVolume is a filter that clips volume data (i.e., structured points)
// using either: any subclass of vtkImplicitFunction; or the input scalar
// data. Clipping means that it actually "cuts" through the cells of the
// dataset, returning everthing inside of the specified implicit function (or
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
class vtkDelaunay3D;

class VTK_EXPORT vtkClipVolume : public vtkStructuredPointsToUnstructuredGridFilter
{
public:
  vtkClipVolume(vtkImplicitFunction *cf=NULL);
  ~vtkClipVolume();
  static vtkClipVolume *New() {return new vtkClipVolume;};
  const char *GetClassName() {return "vtkClipVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or scalar value (if clipping with
  // scalars). The default value is 0.0. 
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);
  
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
  // clipping. If you do not define an implicit function, then the input
  // scalar data will be used for clipping.
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

  vtkUnstructuredGrid *GetClippedOutput() {return this->ClippedOutput;};

  // Description:
  // Set the tolerance for merging clip intersection points that are near
  // the corners of voxels. This tolerance is used to prevent the generation
  // of degenerate tetrahedra.
  vtkSetClampMacro(MergeTolerance,float,0.0001,0.25);
  vtkGetMacro(MergeTolerance,float);
  
  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  unsigned long int GetMTime();

protected:
  void Execute();
  void ClipVoxel(float value, vtkScalars& cellScalars, int flip,
                 float origin[3], float spacing[3], vtkIdList& cellIds,
		 vtkPoints& cellPts, vtkPointData *inPD, vtkPointData *outPD,
		 vtkCellData *inCD, int cellId, vtkCellData *outCD, 
		 vtkCellData *clippedCD);
  vtkImplicitFunction *ClipFunction;
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  int InsideOut;
  float Value;
  int GenerateClipScalars;
  float MergeTolerance;

  int GenerateClippedOutput;
  vtkUnstructuredGrid *ClippedOutput;
  
private:
  vtkUnstructuredGrid *Mesh;
  vtkMergePoints *MeshLocator;
  vtkDelaunay3D *Triangulator;
  
};

#endif
