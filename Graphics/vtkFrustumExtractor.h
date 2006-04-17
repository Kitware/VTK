/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrustumExtractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFrustumExtractor - Returns the portion of the input dataset that 
// lies within a selection frustum.
//
// .SECTION Description
// This class intersects the input DataSet with a frustum and determines which
// cells and points lie within the frustum. The frustum is defined with a 
// vtkPlanes containing six cutting planes. The output is a DataSet that is 
// either a shallow copy of the input dataset with two new "vtkInsidedness" 
// attribute arrays, or a completely new UnstructuredGrid that contains only 
// the cells and points of the input that are inside the frustum. The 
// PassThrough flag controls which occurs.
//
// You have two choices for what cells are considered to be inside.
// ExactTestOff treats a cell as inside only if all of its points are 
// inside. This is equivalent to the inside only mode of vtkExtractGeometry 
// filter. ExactTestOn treats a cell as inside if any part of it is 
// inside. Points are considered to be inside if they are part of an inside 
// cell. An example of a cell that is treated differently in the two modes 
// is a line segment that crosses the frustum but has both vertices outside.
//
// .SECTION See Also
// vtkExtractGeometry, vtkAreaPicker

#ifndef __vtkFrustumExtractor_h
#define __vtkFrustumExtractor_h

#include "vtkDataSetAlgorithm.h"

class vtkPlanes;
class vtkInformation;
class vtkInformationVector;
class vtkCell;
class vtkPoints;
class vtkDoubleArray;

class VTK_GRAPHICS_EXPORT vtkFrustumExtractor : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkFrustumExtractor,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkFrustumExtractor *New();

  // Description:
  // Return the MTime taking into account changes to the Frustum
  unsigned long GetMTime();

  // Description:
  // Set the selection frustum. The planes object must contain six planes.
  virtual void SetFrustum(vtkPlanes*);
  vtkGetObjectMacro(Frustum,vtkPlanes);

  // Description:
  // Given eight vertices, creates a frustum.
  void CreateFrustum(double vertices[24]);

  // Description:
  // Return eight points that define the selection frustum. Valid if
  // create Frustum was used, invalid if SetFrustum was.
  vtkGetObjectMacro(ClipPoints, vtkPoints);
  
  // Description:
  // Sets/Gets the output data type.
  // If On, the input data set is shallow copied through, and two new
  // "vtkInsidedness" attribute arrays are added. If Off the output is 
  // a new vtkUnstructuredGrid containing only the structure that is inside.
  // Off is the default.
  vtkSetMacro(PassThrough,int);
  vtkGetMacro(PassThrough,int);
  vtkBooleanMacro(PassThrough,int);
  
  // Description:
  // Sets/gets the intersection test type.
  // Off extracts only those points and cells that are completely within the 
  // frustum. On extracts all of the above as well as those cells that cross 
  // the frustum along with all of their points.
  // On is the default.
  vtkSetMacro(ExactTest,int);
  vtkGetMacro(ExactTest,int);
  vtkBooleanMacro(ExactTest,int);

  // Description:
  // Does a quick test on the AABBox defined by the bounds.
  int OverallBoundsTest(double *bounds);
  
protected:
  vtkFrustumExtractor(vtkPlanes *f=NULL);
  ~vtkFrustumExtractor();

  //sets up output data type
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  //execution
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int ABoxFrustumIsect(double bounds[], vtkCell *cell);
  int FrustumClipPolygon(int nverts, double *ivlist, double *wvlist, double *ovlist);
  void PlaneClipPolygon(int nverts, double *ivlist, 
                        int pid, int &noverts, double *ovlist);
  void PlaneClipEdge(double *V0, double *V1, 
                     int pid, int &noverts, double *overts);
  int IsectDegenerateCell(vtkCell *cell);


  //used in CreateFrustum
  void ComputePlane(int idx, double v0[3], double v1[2], double v2[3], vtkPoints *points, vtkDoubleArray *norms);

  //modes
  int PassThrough;
  int ExactTest;

  //used internally
  vtkPlanes *Frustum;
  int np_vertids[6][2];

  //for debugging
  vtkPoints *ClipPoints;
  int NumRejects;
  int NumIsects;
  int NumAccepts;

private:
  vtkFrustumExtractor(const vtkFrustumExtractor&);  // Not implemented.
  void operator=(const vtkFrustumExtractor&);  // Not implemented.
};

#endif


