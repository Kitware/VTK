/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometryFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// or vtkExtractVOI.)

// .SECTION Caveats
// When vtkGeometryFilter extracts cells (or boundaries of cells) it may create
// duplicate points. Use vtkCleanPolyData to merge duplicate points.

// .SECTION See Also
// vtkStructuredPointsGeometryFilter vtkStructuredGridGeometryFilter
// vtkExtractGeometry vtkExtractVOI

#ifndef __vtkGeometryFilter_h
#define __vtkGeometryFilter_h

#include "vtkDataSetToPolyFilter.h"

class VTK_EXPORT vtkGeometryFilter : public vtkDataSetToPolyFilter
{
public:
  vtkGeometryFilter();
  static vtkGeometryFilter *New() {return new vtkGeometryFilter;};
  char *GetClassName() {return "vtkGeometryFilter";};
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

  void SetExtent(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
  void SetExtent(float *extent);
  float *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int PointMinimum;
  int PointMaximum;
  int CellMinimum;
  int CellMaximum;
  float Extent[6];
  int PointClipping;
  int CellClipping;
  int ExtentClipping;

};

#endif


