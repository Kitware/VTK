/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkStructuredPoints - topologically and geometrically regular array of data
// .SECTION Description
// vtkStructuredPoints is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredPoints represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "vtkDataSet.hh"
#include "vtkStructuredData.hh"

class vtkStructuredPoints : public vtkDataSet
{
public:
  vtkStructuredPoints();
  vtkStructuredPoints(const vtkStructuredPoints& v);
  ~vtkStructuredPoints();
  char *GetClassName() {return "vtkStructuredPoints";};
  char *GetDataType() {return "vtkStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkStructuredPoints(*this);};
  void CopyStructure(vtkDataSet *ds);
  int GetNumberOfCells();
  int GetNumberOfPoints();
  float *GetPoint(int ptId);
  void GetPoint(int id, float x[3]);
  vtkCell *GetCell(int cellId);
  int FindPoint(float x[3]);
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3], float *weights);
  vtkCell *FindAndGetCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3], float *weights);
  int GetCellType(int cellId);
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // specific to structured data
  void GetVoxelGradient(int i,int j,int k, vtkScalars *s, vtkFloatVectors& g);
  void GetPointGradient(int i, int j, int k, vtkScalars *s, float g[3]);
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);
  int GetDataDimension();

  // Description:
  // Get dimensions of this structured points dataset.
  vtkGetVectorMacro(Dimensions,int,3);

  // Description:
  // Set the aspect ratio of the cubical cells that compose the structured
  // point set.
  vtkSetVector3Macro(AspectRatio,float);
  vtkGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Set the origin of the data. The origin plus aspect ratio determine the
  // position in space of the structured points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

protected:
  int Dimensions[3];
  int DataDescription;
  float Origin[3];
  float AspectRatio[3];

  vtkStructuredData StructuredData; //helper class for structured data
};

inline void vtkStructuredPoints::GetPoint(int id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline int vtkStructuredPoints::GetNumberOfCells() 
{
  int nCells=1;
  int i;

  for (i=0; i<3; i++)
    if (this->Dimensions[i] > 1)
      nCells *= (this->Dimensions[i]-1);

  return nCells;
}

inline int vtkStructuredPoints::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

inline int vtkStructuredPoints::GetDataDimension()
{
  return this->StructuredData.GetDataDimension(this->DataDescription);
}

inline void vtkStructuredPoints::GetCellPoints(int cellId, vtkIdList& ptIds)
{
  this->StructuredData.GetCellPoints(cellId,ptIds,this->DataDescription,
                                     this->Dimensions);
}

inline void vtkStructuredPoints::GetPointCells(int ptId, vtkIdList& cellIds)
{
  this->StructuredData.GetPointCells(ptId,cellIds,this->Dimensions);
}

#endif
