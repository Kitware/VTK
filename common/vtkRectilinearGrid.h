/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGrid.h
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
// .NAME vtkRectilinearGrid - a datset that is topologically regular with variable spacing in the three coordinate directions
// .SECTION Description
// vtkRectilinearGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkRectilinearGrid represents a geometric structure that is 
// topologically regular with variable spacing in the three coordinate
// directions x-y-z.
//
// To define a vtkRectilinearGrid, you must specify the dimensions of the
// data and provide three arrays of values specifying the coordinates 
// along the x-y-z axes. The coordinate arrays are specified using three 
// vtkScalars objects (one for x, one for y, one for z).

// .SECTION Caveats
// Make sure that the dimensions of the grid match the number of coordinates
// in the x-y-z directions. If not, unpredictable results (including
// program failure) may result. Also, you must supply coordinates in all
// three directions, even if the dataset topology is 2D, 1D, or 0D. Finally,
// the coordinates values in each direction must be montonically increasing.

#ifndef __vtkRectilinearGrid_h
#define __vtkRectilinearGrid_h

#include "vtkDataSet.h"
#include "vtkStructuredData.h"
class vtkVertex;
class vtkLine;
class vtkPixel;
class vtkVoxel;

class VTK_EXPORT vtkRectilinearGrid : public vtkDataSet
{
public:
  vtkRectilinearGrid();
  vtkRectilinearGrid(const vtkRectilinearGrid& v);
  ~vtkRectilinearGrid();
  static vtkRectilinearGrid *New() {return new vtkRectilinearGrid;};
  const char *GetClassName() {return "vtkRectilinearGrid";};
  int GetDataSetType() {return VTK_RECTILINEAR_GRID;};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return new vtkRectilinearGrid;};

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Restore object to initial state. Release memory back to system.
  void Initialize();

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  int GetNumberOfCells();
  int GetNumberOfPoints();
  float *GetPoint(int ptId);
  void GetPoint(int id, float x[3]);
  vtkCell *GetCell(int cellId);
  int FindPoint(float x[3]);
  int FindCell(float x[3], vtkCell *cell, int cellId, float tol2, int& subId, 
               float pcoords[3], float *weights);
  vtkCell *FindAndGetCell(float x[3], vtkCell *cell, int cellId, 
               float tol2, int& subId, float pcoords[3], float *weights);
  int GetCellType(int cellId);
  void GetCellPoints(int cellId, vtkIdList *ptIds)
    {vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
				      this->Dimensions);}
  void GetPointCells(int ptId, vtkIdList *cellIds)
    {vtkStructuredData::GetPointCells(ptId,cellIds,this->Dimensions);}
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Description:
  // Set dimensions of rectilinear grid dataset.
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);

  // Description:
  // Get dimensions of this rectilinear grid dataset.
  vtkGetVectorMacro(Dimensions,int,3);

  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The cell is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the grid, and a 1 if inside the grid.
  int ComputeStructuredCoordinates(float x[3], int ijk[3], float pcoords[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), return the point id.
  int ComputePointId(int ijk[3]);

  // Description:
  // Given a location in structured coordinates (i-j-k), return the cell id.
  int ComputeCellId(int ijk[3]);

  // Description:
  // Specify the grid coordinates in the x-direction.
  vtkSetReferenceCountedObjectMacro(XCoordinates,vtkScalars);
  vtkGetObjectMacro(XCoordinates,vtkScalars);

  // Description:
  // Specify the grid coordinates in the y-direction.
  vtkSetReferenceCountedObjectMacro(YCoordinates,vtkScalars);
  vtkGetObjectMacro(YCoordinates,vtkScalars);

  // Description:
  // Specify the grid coordinates in the z-direction.
  vtkSetReferenceCountedObjectMacro(ZCoordinates,vtkScalars);
  vtkGetObjectMacro(ZCoordinates,vtkScalars);

  // Description:
  // For legacy compatability. Do not use.
  void GetCellPoints(int cellId, vtkIdList &ptIds)
    {this->GetCellPoints(cellId, &ptIds);}
  void GetPointCells(int ptId, vtkIdList &cellIds)
    {this->GetPointCells(ptId, &cellIds);}

protected:
  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;
  
  int Dimensions[3];
  int DataDescription;

  vtkScalars *XCoordinates;
  vtkScalars *YCoordinates;
  vtkScalars *ZCoordinates;
};

inline void vtkRectilinearGrid::GetPoint(int id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline int vtkRectilinearGrid::GetNumberOfCells() 
{
  int nCells=1;
  int i;

  for (i=0; i<3; i++)
    {
    if (this->Dimensions[i] > 1)
      {
      nCells *= (this->Dimensions[i]-1);
      }
    }

  return nCells;
}

inline int vtkRectilinearGrid::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

inline int vtkRectilinearGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

inline int vtkRectilinearGrid::ComputePointId(int ijk[3])
{
  return vtkStructuredData::ComputePointId(this->Dimensions,ijk);
}

inline int vtkRectilinearGrid::ComputeCellId(int ijk[3])
{
  return vtkStructuredData::ComputeCellId(this->Dimensions,ijk);
}

#endif
