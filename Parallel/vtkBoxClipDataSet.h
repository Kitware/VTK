/*=========================================================================

  Program:   ParaView
  Module:    vtkBoxClipDataSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

//.NAME vtkBoxClipDataSet - clip an unstructured grid
//Clipping means that is actually 'cuts' through the cells of the dataset,
//returning tetrahedral cells inside of the box.
//The output of this filter is an unstructured grid.
//
// This filter can be configured to compute a second output. The
// second output is the part of the cell that is clipped away. Set the
// GenerateClippedData boolean on if you wish to access this output data.
//
//The vtkBoxClipDataSet will triangulate all types of 3D cells (i.e, create tetrahedra).
//This is necessary to preserve compatibility across face neighbors.
//
//To use this filter,you can decide if you will be clipping with a box or a hexahedral box.
//1) Set orientation 
//   if(SetOrientation(0)): box (parallel with coordinate axis)
//      SetBoxClip(xmin,xmax,ymin,ymax,zmin,zmax)  
//   if(SetOrientation(1)): hexahedral box (Default)
//      SetBoxClip(n[0],o[0],n[1],o[1],n[2],o[2],n[3],o[3],n[4],o[4],n[5],o[5])  
//      n[] normal of each plane
//      o[] point on the plane 
//2) Apply the GenerateClipScalarsOn() 
//3) Execute clipping  Update();

#ifndef __vtkBoxClipDataSet_h
#define __vtkBoxClipDataSet_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkGenericCell;            
class vtkCell3D;                     
class vtkDataArray;                    
class vtkCellArray;                   
class vtkPointData;                   
class vtkCellData;                    
class vtkPoints;                      
class vtkIdList;                    
class vtkPointLocator;

class VTK_EXPORT vtkBoxClipDataSet : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkBoxClipDataSet,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor of the clipping box.
  static vtkBoxClipDataSet *New();

  // Description
  // Specify the Box with which to perform the clipping. 
  // If the box is not parallel to axis, you need to especify  
  // normal vector of each plane and a point on the plane. 
  
  void SetBoxClip(float xmin,float xmax,float ymin,float ymax,float zmin,float zmax);
  void SetBoxClip(double xmin,double xmax,double ymin,double ymax,double zmin,double zmax);

  void SetBoxClip(float *n0,float *o0,float *n1,float *o1,float *n2,float *o2,float *n3,float *o3,float *n4,float *o4,float *n5,float *o5);
  void SetBoxClip(double *n0,double *o0,double *n1,double *o1,double *n2,double *o2,double *n3,double *o3,double *n4,double *o4,double *n5,double *o5);
  

  // Description:
  // If this flag is enabled, then the output scalar values will be 
  // interpolated, and not the input scalar data.
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);

  // Description:
  // Control whether a second output is generated. The second output
  // contains the polygonal data that's been clipped away.
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);

  // Description:
  // Set the tolerance for merging clip intersection points that are near
  // the vertices of cells. This tolerance is used to prevent the generation
  // of degenerate primitives. Note that only 3D cells actually use this
  // instance variable.
  vtkSetClampMacro(MergeTolerance,double,0.0001,0.25);
  vtkGetMacro(MergeTolerance,double);
  
  // Description:
  // Return the Clipped output.
  vtkUnstructuredGrid *GetClippedOutput();
  virtual int GetNumberOfOutputs();

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Return the mtime also considering the locator.
  unsigned long GetMTime();

  // Description:
  // If you want to clip by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

  void SetOrientation(unsigned int i)
          {Orientation = i; };

  unsigned int GetOrientation()
          {return  Orientation;};
  void MinEdgeF(unsigned int *id_v, vtkIdType *cellIds,unsigned int *edgF );
  void PyramidToTetra(vtkIdType *pyramId, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void WedgeToTetra(vtkIdType *wedgeId, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void CellGrid(vtkIdType typeobj, vtkIdType npts, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void CreateTetra(vtkIdType npts,vtkIdType *cellIds,vtkCellArray *newCellArray);
  void ClipBox(vtkPoints *newPoints,vtkGenericCell *cell, 
               vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
               vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
               vtkCellData *outCD);
  void ClipHexahedron(vtkPoints *newPoints,vtkGenericCell *cell,
      vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
      vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
      vtkCellData *outCD);
  void ClipBoxInOut(vtkPoints *newPoints,vtkGenericCell *cell, 
           vtkPointLocator *locator, vtkCellArray **tets,vtkPointData *inPD, 
           vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
           vtkCellData **outCD);
  void ClipHexahedronInOut(vtkPoints *newPoints,vtkGenericCell *cell,
           vtkPointLocator *locator, vtkCellArray **tets,vtkPointData *inPD, 
           vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
           vtkCellData **outCD);
  void ClipBox2D(vtkPoints *newPoints,vtkGenericCell *cell, 
           vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
           vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
           vtkCellData *outCD);
  void ClipBoxInOut2D(vtkPoints *newPoints,vtkGenericCell *cell, 
              vtkPointLocator *locator, vtkCellArray **tets,vtkPointData *inPD, 
              vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
              vtkCellData **outCD);
  void ClipHexahedron2D(vtkPoints *newPoints,vtkGenericCell *cell,
         vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
         vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
         vtkCellData *outCD);
  void ClipHexahedronInOut2D(vtkPoints *newPoints,vtkGenericCell *cell,
         vtkPointLocator *locator, vtkCellArray **tets,vtkPointData *inPD, 
         vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
         vtkCellData **outCD);
protected:
  vtkBoxClipDataSet();
  ~vtkBoxClipDataSet();

  void Execute();
  
  vtkPointLocator *Locator;
  int GenerateClipScalars;

  int GenerateClippedOutput;

  double MergeTolerance;

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  double BoundBoxClip[3][2];
  unsigned int Orientation;
  double n_pl[6][3];
  double o_pl[6][3];

private:
  vtkBoxClipDataSet(const vtkBoxClipDataSet&);  // Not implemented.
  void operator=(const vtkBoxClipDataSet&);  // Not implemented.
};

#endif
