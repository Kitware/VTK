/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPicker - superclass for 3D geometric pickers (uses ray cast)
// .SECTION Description
// vtkPicker is used to select instances of vtkProp3D by shooting a ray 
// into a graphics window and intersecting with the actor's bounding box. 
// The ray is defined from a point defined in window (or pixel) coordinates, 
// and a point located from the camera's position.
//
// vtkPicker may return more than one vtkProp3D, since more than one bounding 
// box may be intersected. vtkPicker returns the list of props that were hit, 
// the pick coordinates in world and untransformed mapper space, and the 
// prop (vtkProp3D) and mapper that are "closest" to the camera. The closest 
// prop is the one whose center point (i.e., center of bounding box) 
// projected on the ray is closest to the camera.

// .SECTION See Also
// vtkPicker is used for quick geometric picking. If you desire to pick
// points or cells, use the subclass vtkPointPicker or vtkCellPicker,
// respectively.  Or you may use hardware picking to pick any type of vtkProp
// - see vtkPropPicker or vtkWorldPointPicker.

#ifndef __vtkPicker_h
#define __vtkPicker_h

#include "vtkObject.h"
#include "vtkAbstractPropPicker.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkProp3DCollection.h"
#include "vtkMapper.h"
#include "vtkTransform.h"

class VTK_RENDERING_EXPORT vtkPicker : public vtkAbstractPropPicker
{
public:
  static vtkPicker *New();
  vtkTypeRevisionMacro(vtkPicker,vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify tolerance for performing pick operation. Tolerance is specified
  // as fraction of rendering window size. (Rendering window size is measured
  // across diagonal.)
  vtkSetMacro(Tolerance,float);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Return position in mapper (i.e., non-transformed) coordinates of 
  // pick point.
  vtkGetVectorMacro(MapperPosition,float,3);

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
  // Return a collection of all the actors that were intersected.
  // This collection is not sorted. (This is a convenience method
  // to maintain backward compatibility.)
  vtkActorCollection *GetActors() {
    if (this->Actors->GetNumberOfItems() != 
        this->PickedPositions->GetNumberOfPoints()) {
      vtkWarningMacro(<<"Not all Prop3Ds are actors, use GetProp3Ds instead");}
    return this->Actors; };

  // Description:
  // Return a list of the points the the actors returned by GetActors
  // were intersected at. The order of this list will match the order of
  // GetActors.
  vtkPoints *GetPickedPositions() {return this->PickedPositions;};
  
  // Description:
  // Perform pick operation with selection point provided. Normally the 
  // first two values for the selection point are x-y pixel coordinate, and
  // the third value is =0. Return non-zero if something was successfully 
  // picked.
  virtual int Pick(float selectionX, float selectionY, float selectionZ, 
                   vtkRenderer *renderer);  

  // Description: 
  // Perform pick operation with selection point provided. Normally the first
  // two values for the selection point are x-y pixel coordinate, and the
  // third value is =0. Return non-zero if something was successfully picked.
  int Pick(float selectionPt[3], vtkRenderer *ren)
    {return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], ren);};
      
protected:
  vtkPicker();
  ~vtkPicker();

  void MarkPicked(vtkAssemblyPath *path, vtkProp3D *p, vtkAbstractMapper3D *m, 
                  float tMin, float mapperPos[3]);
  virtual float IntersectWithLine(float p1[3], float p2[3], float tol, 
                                  vtkAssemblyPath *path, vtkProp3D *p, 
                                  vtkAbstractMapper3D *m);
  virtual void Initialize();

  float Tolerance; //tolerance for computation (% of window)
  float MapperPosition[3]; //selection point in untransformed coordinates

  vtkAbstractMapper3D *Mapper; //selected mapper (if the prop has a mapper)
  vtkDataSet *DataSet; //selected dataset (if there is one)

  float GlobalTMin; //parametric coordinate along pick ray where hit occured
  vtkTransform *Transform; //use to perform ray transformation
  vtkActorCollection *Actors; //candidate actors (based on bounding box)
  vtkProp3DCollection *Prop3Ds; //candidate actors (based on bounding box)
  vtkPoints *PickedPositions; // candidate positions
  
private:
  vtkPicker(const vtkPicker&);  // Not implemented.
  void operator=(const vtkPicker&);  // Not implemented.
};


#endif


