/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.h
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
// .NAME vtkImageData - topologically and geometrically regular array of data
// .SECTION Description
// vtkImageData is a data object that is a concrete implementation of
// vtkDataSet. vtkImageData represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vtkImageData_h
#define __vtkImageData_h

#include "vtkDataSet.h"
#include "vtkStructuredData.h"
class vtkVertex;
class vtkLine;
class vtkPixel;
class vtkVoxel;
class vtkImageToStructuredPoints;
class vtkExtent;
class vtkStructuredExtent;


class VTK_EXPORT vtkImageData : public vtkDataSet
{
public:
  static vtkImageData *New() {return new vtkImageData;};

  const char *GetClassName() {return "vtkImageData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a similar type object
  vtkDataObject *MakeObject() {return new vtkImageData;};

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_IMAGE_DATA;};

  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  int GetNumberOfCells();
  int GetNumberOfPoints();
  float *GetPoint(int ptId);
  void GetPoint(int id, float x[3]);
  vtkCell *GetCell(int cellId);
  void GetCell(int cellId, vtkGenericCell *cell);
  void GetCellBounds(int cellId, float bounds[6]);
  int FindPoint(float x[3]);
  int FindCell(float x[3], vtkCell *cell, int cellId, float tol2, int& subId, 
               float pcoords[3], float *weights);
  int FindCell(float x[3], vtkCell *cell, vtkGenericCell *gencell,
	       int cellId, float tol2, int& subId, 
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
  void GetVoxelGradient(int i,int j,int k, vtkScalars *s, vtkVectors *g);

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
  // For legacy compatibility. Do not use.
  void GetCellPoints(int cellId, vtkIdList &ptIds)
    {this->GetCellPoints(cellId, &ptIds);}
  void GetPointCells(int ptId, vtkIdList &cellIds)
    {this->GetPointCells(ptId, &cellIds);}
  void GetVoxelGradient(int i,int j,int k, vtkScalars *s, vtkVectors &g)
    {this->GetVoxelGradient(i, j, k, s, &g);}

  // Description:
  // Set/Get the whole extent of the data.
  void SetWholeExtent(int extent[6]);
  void SetWholeExtent(int xMin, int xMax,
		      int yMin, int yMax, int zMin, int zMax);
  void GetWholeExtent(int extent[6]);
  int *GetWholeExtent() {return this->WholeExtent;}
  void GetWholeExtent(int &xMin, int &xMax,
		      int &yMin, int &yMax, int &zMin, int &zMax);
  
  // Description:
  // This extent is used to request just a piece of the grid.
  // If the UpdateExtent is set before Update is called, then
  // the Update call may only generate the portion of the data 
  // requested.  The source has the option of generating more 
  // than the requested extent.  If it does, then it will
  // modify the UpdateExtent value to reflect the actual extent
  // in the data.
  void SetUpdateExtent(int extent[6]);
  void SetUpdateExtent(int xMin, int xMax,
		       int yMin, int yMax, int zMin, int zMax);
  void SetUpdateExtentToWholeExtent();
  void SetAxisUpdateExtent(int axis, int min, int max);
  int *GetUpdateExtent();
  void GetUpdateExtent(int ext[6]);
  void GetUpdateExtent(int &x1, int &x2, int &y1, int &y2, int &z1, int &z2);
  void GetAxisUpdateExtent(int axis, int &min, int &max);

  // Description:
  // Warning: This is still in develoment.  DataSetToDataSetFilters use
  // CopyUpdateExtent to pass the update extents up the pipeline.
  // In order to pass a generic update extent through a port we are going 
  // to need these methods (which should eventually replace the 
  // CopyUpdateExtent method).
  vtkExtent *GetGenericUpdateExtent() {return (vtkExtent*)this->UpdateExtent;}
  void CopyGenericUpdateExtent(vtkExtent *ext);
  
  // Description:
  // Required for the lowest common denominator for setting the UpdateExtent
  // (i.e. vtkDataSetToStructuredPointsFilter).  This assumes that WholeExtent
  // is valid (UpdateInformation has been called).
  void SetUpdateExtent(int piece, int numPieces);
  
  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int *extent);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVectorMacro(Extent,int,6);
  void GetExtent(int &x1, int &x2, int &y1, int &y2, int &z1, int &z2);

  // Description:
  // Set/Get the data scalar type of the regions created by this cache.
  void SetScalarType(int);
  int GetScalarType();

  // Description:
  // These returns the minimum and maximum values the ScalarType can hold
  // without overflowing.
  double GetScalarTypeMin();
  double GetScalarTypeMax();
  
  // Description:
  // Set the size of the scalar type in bytes.
  int GetScalarSize();

  // Description:
  // Different ways to get the increments for moving around the data.
  vtkGetVector3Macro(Increments,int);
  
  // Description:
  // Different ways to get the increments for moving around the data.
  // They are store (Component, X, Y, Z). This method returns
  // increments that are suited for continuous incrementing of the
  // pointer in a Z, Y, X, C nested for loop.
  void GetContinuousIncrements(int extent[6], int &incX, int &incY,
			       int &incZ);
  
  // Description:
  // Access the native pointer for the scalar data
  void *GetScalarPointerForExtent(int coordinates[6]);
  void *GetScalarPointer(int coordinates[3]);
  void *GetScalarPointer(int x, int y, int z);
  void *GetScalarPointer();

  // Description:
  // For acces to data from tcl
  float GetScalarComponentAsFloat(int x, int y, int z, int component);
  
  // Description:
  // Allocate the vtkScalars object associated with this object.
  void AllocateScalars();
  
  // Description:
  // Set/Get the number of scalar components
  void SetNumberOfScalarComponents(int num);
  vtkGetMacro(NumberOfScalarComponents,int);
  
  // Description:
  // This method is passed a input and output region, and executes the filter
  // algorithm to fill the output from the input.
  // It just executes a switch statement to call the correct function for
  // the regions data types.
  void CopyAndCastFrom(vtkImageData *inData, int extent[6]);

  // Description:
  // Called by superclass to limit UpdateExtent to be less than or equal
  // to the WholeExtent.  It assumes that UpdateInformation has been 
  // called.
  void ClipUpdateExtentWithWholeExtent();

  // Description:
  // Copies the UpdateExtent from another vtkImageData.
  // Used by some filter superclasses during UpdateInformation to 
  // copy requested piece from output to input.  
  void CopyUpdateExtent(vtkDataObject *data); 
  
  // Description:  
  // This method is used translparently by the "SetInput(vtkImageCache *)"
  // method to connect the image pipeline to the visualization pipeline.
  vtkImageToStructuredPoints *MakeImageToStructuredPoints();

  // Description:
  // This class has a special UpdateInformation method
  // that automatically computes EstimatedWholeMemorySize.
  void UpdateInformation();

  // Description:
  // Return the amount of memory for the update piece.
  unsigned long GetEstimatedUpdateMemorySize();
  
  // Description:
  // Legacy.  Replaced with GetEstimatedUpdateMemorySize.
  long GetUpdateExtentMemorySize() 
    {
      vtkWarningMacro("Change GetUpdateExtentMemorySize to GetEstimatedUpdateMemorySize");
      return GetEstimatedUpdateMemorySize();
    }

  // Description:
  // Legacy.  With no cache, this method is no longer needed.
  vtkImageData *UpdateAndReturnData() 
    {
      vtkWarningMacro("UpdateAndReturnData will no longer be supported.  Just Update, and use the data.");
      this->Update();
      return this;
    }
  
  
protected:
  vtkImageData();
  vtkImageData(const vtkImageData& v);
  ~vtkImageData();

  vtkImageToStructuredPoints *ImageToStructuredPoints;

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;
  
  // What will be generated on the next call to Update.
  vtkStructuredExtent *UpdateExtent;

  int WholeExtent[6];
  // The extent of what is currently in the structured grid.
  int Extent[6];
  int Dimensions[3];
  int DataDescription;
  float Origin[3];
  float Spacing[3];
  int Increments[3];
  int ScalarType;
  int NumberOfScalarComponents;
  
  void ComputeIncrements();

  // Computes the estimated memory size from the Update extent,
  // ScalarType, and NumberOfComponents
  void ComputeEstimatedWholeMemorySize();
};

inline void vtkImageData::GetPoint(int id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline int vtkImageData::GetNumberOfCells() 
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

inline int vtkImageData::GetNumberOfPoints()
{
  return this->Dimensions[0]*this->Dimensions[1]*this->Dimensions[2];
}

inline int vtkImageData::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

#endif



