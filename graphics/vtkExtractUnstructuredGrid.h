/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractUnstructuredGrid.h
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
// .NAME vtkExtractUnstructuredGrid - extract subset of unstructured grid geometry
// .SECTION Description
// vtkExtractUnstructuredGrid is a general-purpose filter to
// extract geometry (and associated data) from an unstructured grid
// dataset. The extraction process is controlled by specifying a range
// of point ids, cell ids, or a bounding box (referred to as "Extent").
// 

// .SECTION Caveats
// The input points are copied through to the output. This means
// unused points may be present in the output data.

// .SECTION See Also
// vtkStructuredPointsGeometryFilter vtkStructuredGridGeometryFilter
// vtkRectilinearGridGeometryFilter
// vtkExtractGeometry vtkExtractVOI

#ifndef __vtkExtractUnstructuredGrid_h
#define __vtkExtractUnstructuredGrid_h

#include "vtkUnstructuredGridFilter.h"

class VTK_EXPORT vtkExtractUnstructuredGrid : public vtkUnstructuredGridFilter
{
public:
  vtkExtractUnstructuredGrid();
  ~vtkExtractUnstructuredGrid();
  static vtkExtractUnstructuredGrid *New() {return new vtkExtractUnstructuredGrid;};
  const char *GetClassName() {return "vtkExtractUnstructuredGrid";};
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

  // Specify x-y-z box for geometric region clipping
  void SetExtent(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
  void SetExtent(float *extent);
  float *GetExtent() { return this->Extent;};

  // Description:
  // Get the output of this filter.
  vtkUnstructuredGrid *GetOutput() {return (vtkUnstructuredGrid *)this->Output;};

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


