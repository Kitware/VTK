/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.h
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
// .NAME vtkGeometryFilter - extract geometry from data (or convert data to polygonal type)
// .SECTION Description
// vtkGeometryFilter is a general-purpose filter to extract geometry (and 
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
// use vtkStructuredPointsGeometryFilter, vtkStructuredGridGeometryFilter,
// vtkUnstructuredGridGeometryFilter, vtkRectilinearGridGeometryFilter, or 
// vtkExtractVOI.)

// .SECTION Caveats
// When vtkGeometryFilter extracts cells (or boundaries of cells) it
// will (by default) merge duplicate vertices. This may cause problems
// in some cases. For example, if you've run vtkPolyDataNormals to
// generate normals, which may split meshes and create duplicate
// vertices, vtkGeometryFilter will merge these points back
// together. Turn merging off to prevent this from occurring.

// .SECTION See Also
// vtkStructuredPointsGeometryFilter vtkStructuredGridGeometryFilter
// vtkExtractGeometry vtkExtractVOI

#ifndef __vtkGeometryFilter_h
#define __vtkGeometryFilter_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkGeometryFilter : public vtkDataSetToPolyDataFilter
{
public:
  static vtkGeometryFilter *New();
  vtkTypeMacro(vtkGeometryFilter,vtkDataSetToPolyDataFilter);
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
  vtkSetClampMacro(PointMinimum,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(PointMinimum,int);

  // Description:
  // Specify the maximum point id for point id selection.
  vtkSetClampMacro(PointMaximum,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(PointMaximum,int);

  // Description:
  // Specify the minimum cell id for point id selection.
  vtkSetClampMacro(CellMinimum,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(CellMinimum,int);

  // Description:
  // Specify the maximum cell id for point id selection.
  vtkSetClampMacro(CellMaximum,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(CellMaximum,int);

  // Description:
  // Specify a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
  void SetExtent(float xMin, float xMax, float yMin, float yMax, 
                 float zMin, float zMax);

  // Description:
  // Set / get a (xmin,xmax, ymin,ymax, zmin,zmax) bounding box to clip data.
  void SetExtent(float extent[6]);
  float *GetExtent() { return this->Extent;};

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
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

  // Description:
  // Return the MTime also considering the locator.
  unsigned long GetMTime();

protected:
  vtkGeometryFilter();
  ~vtkGeometryFilter();
  vtkGeometryFilter(const vtkGeometryFilter&) {};
  void operator=(const vtkGeometryFilter&) {};

  void Execute();
  void PolyDataExecute(); //special cases for performance
  void UnstructuredGridExecute();
  void StructuredGridExecute();
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void ExecuteInformation();

  int PointMaximum;
  int PointMinimum;
  int CellMinimum;
  int CellMaximum;
  float Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

  int Merging;
  vtkPointLocator *Locator;
};

#endif


