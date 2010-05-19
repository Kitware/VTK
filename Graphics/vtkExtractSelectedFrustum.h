/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedFrustum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedFrustum - Returns the portion of the input dataset that 
// lies within a selection frustum.
//
// .SECTION Description
// This class intersects the input DataSet with a frustum and determines which
// cells and points lie within the frustum. The frustum is defined with a 
// vtkPlanes containing six cutting planes. The output is a DataSet that is 
// either a shallow copy of the input dataset with two new "vtkInsidedness" 
// attribute arrays, or a completely new UnstructuredGrid that contains only 
// the cells and points of the input that are inside the frustum. The 
// PreserveTopology flag controls which occurs. When PreserveTopology is off 
// this filter adds a scalar array called vtkOriginalCellIds that says what 
// input cell produced each output cell. This is an example of a Pedigree ID 
// which helps to trace back results.
//
// .SECTION See Also
// vtkExtractGeometry, vtkAreaPicker, vtkExtractSelection, vtkSelection

#ifndef __vtkExtractSelectedFrustum_h
#define __vtkExtractSelectedFrustum_h

#include "vtkExtractSelectionBase.h"

class vtkPlanes;
class vtkInformation;
class vtkInformationVector;
class vtkCell;
class vtkPoints;
class vtkDoubleArray;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedFrustum : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectedFrustum *New();
  vtkTypeMacro(vtkExtractSelectedFrustum, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime taking into account changes to the Frustum
  unsigned long GetMTime();

  // Description:
  // Set the selection frustum. The planes object must contain six planes.
  virtual void SetFrustum(vtkPlanes*);
  vtkGetObjectMacro(Frustum,vtkPlanes);

  // Description:
  // Given eight vertices, creates a frustum.
  // each pt is x,y,z,1
  // in the following order
  // near lower left, far lower left
  // near upper left, far upper left
  // near lower right, far lower right
  // near upper right, far upper right
  void CreateFrustum(double vertices[32]);

  // Description:
  // Return eight points that define the selection frustum. Valid if
  // create Frustum was used, invalid if SetFrustum was.
  vtkGetObjectMacro(ClipPoints, vtkPoints);
  
  // Description:
  // Sets/gets the intersection test type.
  vtkSetMacro(FieldType,int);
  vtkGetMacro(FieldType,int);

  // Description:
  // Sets/gets the intersection test type. Only meaningful when fieldType is 
  // vtkSelection::POINT
  vtkSetMacro(ContainingCells,int);
  vtkGetMacro(ContainingCells,int);

  // Description:
  // Does a quick test on the AABBox defined by the bounds.
  int OverallBoundsTest(double *bounds);

  // Description:
  // When On, this returns an unstructured grid that outlines selection area.
  // Off is the default.
  vtkSetMacro(ShowBounds,int);
  vtkGetMacro(ShowBounds,int);
  vtkBooleanMacro(ShowBounds,int);
  
  // Description:
  // When on, extracts cells outside the frustum instead of inside.
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);

protected:
  vtkExtractSelectedFrustum(vtkPlanes *f=NULL);
  ~vtkExtractSelectedFrustum();

  // sets up output dataset
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  //execution
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, vtkInformationVector *);
  int ABoxFrustumIsect(double bounds[], vtkCell *cell);
  int FrustumClipPolygon(int nverts, 
                         double *ivlist, double *wvlist, double *ovlist);
  void PlaneClipPolygon(int nverts, double *ivlist, 
                        int pid, int &noverts, double *ovlist);
  void PlaneClipEdge(double *V0, double *V1, 
                     int pid, int &noverts, double *overts);
  int IsectDegenerateCell(vtkCell *cell);


  //used in CreateFrustum
  void ComputePlane(int idx, 
                    double v0[3], double v1[2], double v2[3], 
                    vtkPoints *points, vtkDoubleArray *norms);

  //modes
  int FieldType;
  int ContainingCells;
  int InsideOut;

  //used internally
  vtkPlanes *Frustum;
  int np_vertids[6][2];

  //for debugging
  vtkPoints *ClipPoints;
  int NumRejects;
  int NumIsects;
  int NumAccepts;
  int ShowBounds;
  
private:
  vtkExtractSelectedFrustum(const vtkExtractSelectedFrustum&);  // Not implemented.
  void operator=(const vtkExtractSelectedFrustum&);  // Not implemented.

};

#endif


