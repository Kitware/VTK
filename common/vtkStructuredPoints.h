/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.h
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
// .NAME vtkStructuredPoints - topologically and geometrically regular array of data
// .SECTION Description
// vtkStructuredPoints is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredPoints represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "vtkDataSet.h"
#include "vtkStructuredData.h"
class vtkStructuredPointsToImage;


class VTK_EXPORT vtkStructuredPoints : public vtkDataSet
{
public:
  vtkStructuredPoints();
  vtkStructuredPoints(const vtkStructuredPoints& v);
  ~vtkStructuredPoints();
  static vtkStructuredPoints *New() {return new vtkStructuredPoints;};
  const char *GetClassName() {return "vtkStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return new vtkStructuredPoints;};

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  int GetDataSetType() {return VTK_STRUCTURED_POINTS;};

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
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int i, int j, int k);

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int dim[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  vtkGetVectorMacro(Dimensions,int,3);

  // Description:
  // Set the spacing (width,height,length) of the cubical cells that
  // compose the structured point set.
  vtkSetVector3Macro(Spacing,float);
  vtkGetVectorMacro(Spacing,float,3);

  // Description:
  // Set the origin of the data. The origin plus spacing determine the
  // position in space of the structured points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  int ComputeStructuredCoordinates(float x[3], int ijk[3], float pcoords[3]);
  
  // Description:
  // Given structured coordinates (i,j,k) for a voxel cell, compute the eight 
  // gradient values for the voxel corners. The order in which the gradient
  // vectors are arranged corresponds to the ordering of the voxel points. 
  // Gradient vector is computed by central differences (except on edges of 
  // volume where forward difference is used). The scalars s are the scalars
  // from which the gradient is to be computed. This method will treat 
  // only 3D structured point datasets (i.e., volumes).
  void GetVoxelGradient(int i,int j,int k, vtkScalars *s, vtkVectors& g);

  // Description:
  // Given structured coordinates (i,j,k) for a point in a structured point 
  // dataset, compute the gradient vector from the scalar data at that point. 
  // The scalars s are the scalars from which the gradient is to be computed.
  // This method will treat structured point datasets of any dimension.
  void GetPointGradient(int i, int j, int k, vtkScalars *s, float g[3]);

  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Given a location in structured coordinates (i-j-k), return the point id.
  int ComputePointId(int ijk[3]) {
    return vtkStructuredData::ComputePointId(this->Dimensions,ijk);};

  // Description:
  // Given a location in structured coordinates (i-j-k), return the cell id.
  int ComputeCellId(int ijk[3]) {
    return vtkStructuredData::ComputeCellId(this->Dimensions,ijk);};

  // Description:  
  // This method is used translparently by the 
  // "SetInput(vtkStructuredPoints *)"
  // method to connect the visualization pipeline to the image pipeline..
  vtkStructuredPointsToImage *GetStructuredPointsToImage();

protected:
  int Dimensions[3];
  int DataDescription;
  float Origin[3];
  float Spacing[3];
  vtkStructuredPointsToImage *StructuredPointsToImage;
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
    {
    if (this->Dimensions[i] > 1)
      {
      nCells *= (this->Dimensions[i]-1);
      }
    }

  return nCells;
}

inline int vtkStructuredPoints::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

inline int vtkStructuredPoints::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

inline void vtkStructuredPoints::GetCellPoints(int cellId, vtkIdList& ptIds)
{
  vtkStructuredData::GetCellPoints(cellId,ptIds,this->DataDescription,
                                     this->Dimensions);
}

inline void vtkStructuredPoints::GetPointCells(int ptId, vtkIdList& cellIds)
{
  vtkStructuredData::GetPointCells(ptId,cellIds,this->Dimensions);
}


#endif
