/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAreaPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAreaPicker - Picks props behind a selection rectangle on a viewport.
//
// .SECTION Description
// The vtkAreaPicker picks all vtkProp3Ds that lie behind the screen space 
// rectangle from x0,y0 and x1,y1. 
//
// Like vtkPicker, a pick results in a list of Prop3Ds because many props may 
// lie within the pick frustum. You can also get an AssemblyPath, which in this 
// case is defined to be the path to the one particular prop in the Prop3D list
// that lies nearest to the near plane. 
//
// This picker also returns the selection frustum, defined as either a
// vtkPlanes, or a set of eight corner vertices in world space. The vtkPlanes 
// version is an ImplicitFunction, which is suitable for use with the
// vtkExtractGeometry, vtkClipDataSet and vtkCutter.
//
// Because this picker picks everything within a volume, the world pick point 
// result is ill-defined. Therefore if you ask this class for the world pick 
// position, you will get the centroid of the pick frustum. This may be outside 
// of all props in the prop list.
//
// .SECTION See Also
// vtkInteractorStyleRubberBandPick, vtkExtractGeometry, vtkClipDataSet, vtkCutter.

#ifndef __vtkAreaPicker_h
#define __vtkAreaPicker_h

#include "vtkAbstractPropPicker.h"

class vtkRenderer;
class vtkPoints;
class vtkPlanes;
class vtkDoubleArray;
class vtkProp3DCollection;
class vtkAbstractMapper3D;
class vtkDataSet;

class VTK_HYBRID_EXPORT vtkAreaPicker : public vtkAbstractPropPicker
{
public:
  static vtkAreaPicker *New();
  vtkTypeRevisionMacro(vtkAreaPicker,vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return mapper that was picked (if any).
  vtkGetObjectMacro(Mapper,vtkAbstractMapper3D);

  // Description:
  // Get a pointer to the dataset that was picked (if any). If nothing 
  // was picked then NULL is returned.
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Return a collection of all the prop 3D's that were intersected
  // by the pick ray. This collection is not sorted.
  vtkProp3DCollection *GetProp3Ds() {return this->Prop3Ds;};

  // Description:
  // Return eight points that define the selection frustum.
  vtkGetObjectMacro(ClipPoints, vtkPoints);

  // Description:
  // Return the six planes that define the selection frustum. The implicit 
  // function defined by the planes evaluates to negative inside and positive
  // outside.
  vtkGetObjectMacro(Planes, vtkPlanes);

  // Description:
  // Perform pick operation in volume behind the given screen coordinates.
  // Props intersecting the selection frustum will be accesible via GetProp3D.
  // GetPlanes returns a vtkImplicitFunciton suitable for vtkExtractGeometry.
  virtual int AreaPick(double x0, double y0, double x1, double y1, vtkRenderer *renderer);

  // Description:
  // Perform pick operation in volume behind the given screen coordinate.
  // This makes a thin frustum around the selected pixel.
  // Note: this ignores Z in order to pick everying in a volume from z=0 to z=1.
  virtual int Pick(double x0, double y0, double z0, vtkRenderer *renderer)
    {return this->AreaPick(x0-1, y0-1, x0+1, y0+1, renderer);};

protected:
  vtkAreaPicker();
  ~vtkAreaPicker();

  virtual void Initialize();

  void ComputePlane(int idx, int p0, int p1, int p2);
  void DefineFrustum(double x0, double y0, double x1, double y1, vtkRenderer *renderer);

  int PickProps(double x0, double y0, double x1, double y1, vtkRenderer *renderer);  
  int ABoxFrustumIsect(double bounds[], double &mindist);

  vtkPoints *ClipPoints;
  vtkPlanes *Planes;
  vtkPoints *Pts;
  vtkDoubleArray *Norms;


  vtkProp3DCollection *Prop3Ds; //candidate actors (based on bounding box)
  vtkAbstractMapper3D *Mapper; //selected mapper (if the prop has a mapper)
  vtkDataSet *DataSet; //selected dataset (if there is one)

private:
  vtkAreaPicker(const vtkAreaPicker&);  // Not implemented.
  void operator=(const vtkAreaPicker&);  // Not implemented.
};

#endif


