/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkPicker - select an actor by shooting a ray into a graphics window
// .SECTION Description
// vtkPicker is used to select actors by shooting a ray into a graphics window
// and intersecting with the actor's bounding box. The ray is defined from a 
// point defined in window (or pixel) coordinates, and a point located from 
// the camera's position.
//
// vtkPicker may return more than one actor, since more than one bounding 
// box may be intersected. vtkPicker returns the list of actors that were hit, 
// the pick coordinates in world and untransformed mapper space, and the 
// actor and mapper that are "closest" to the camera. The closest actor is 
// the one whose center point (i.e., center of bounding box) projected on the
// ray is closest to the camera.
// .SECTION Caveats
// vtkPicker and its subclasses will not pick actors that are "unpickable" 
// (see vtkActor) or are fully transparent.
// .SECTION See Also
// vtkPicker is used for quick picking. If you desire to pick points
// or cells, use the subclass vtkPointPicker or vtkCellPicker, respectively.

#ifndef __vtkPicker_h
#define __vtkPicker_h

#include "vtkObject.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkMapper.h"
#include "vtkTransform.h"

class VTK_EXPORT vtkPicker : public vtkObject
{
public:
  vtkPicker();
  static vtkPicker *New() {return new vtkPicker;};
  const char *GetClassName() {return "vtkPicker";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the renderer in which pick event occurred.
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // Get the selection point in screen (pixel) coordinates. The third
  // value is related to z-buffer depth. (Normally should be =0.)
  vtkGetVectorMacro(SelectionPoint,float,3);

  // Description:
  // Specify tolerance for performing pick operation. Tolerance is specified
  // as fraction of rendering window size. (Rendering window size is measured
  // across diagonal.)
  vtkSetMacro(Tolerance,float);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Return position in global coordinates of pick point.
  vtkGetVectorMacro(PickPosition,float,3);

  // Description:
  // Return position in mapper (i.e., non-transformed) coordinates of 
  // pick point.
  vtkGetVectorMacro(MapperPosition,float,3);

  // Description:
  // Return assembly that was picked. The assembly may be the same as the 
  // actor.
  vtkGetObjectMacro(Assembly,vtkActor);

  // Description:
  // Return actor that was picked.
  vtkGetObjectMacro(Actor,vtkActor);

  // Description:
  // Return mapper that was picked.
  vtkGetObjectMacro(Mapper,vtkMapper);

  // Description:
  // Get a pointer to the dataset that was picked. If nothing was picked then
  // NULL is returned.
  vtkGetObjectMacro(DataSet,vtkDataSet);

  vtkActorCollection *GetActors();

  virtual int Pick(float selectionX, float selectionY, float selectionZ, 
           vtkRenderer *renderer);  
  int Pick(float selectionPt[3], vtkRenderer *renderer);  

protected:
  void MarkPicked(vtkActor *assem, vtkActor *a, vtkMapper *m, 
                  float tMin, float mapperPos[3]);
  virtual void IntersectWithLine(float p1[3], float p2[3], float tol, 
                                 vtkActor *assem, vtkActor *a, vtkMapper *m);
  virtual void Initialize();

  vtkRenderer *Renderer; //pick occurred in this renderer's viewport
  float SelectionPoint[3]; //selection point in window (pixel) coordinates
  float Tolerance; //tolerance for computation (% of window)
  float PickPosition[3]; //selection point in world coordinates
  float MapperPosition[3]; //selection point in untransformed coordinates
  vtkActor *Assembly; //selected assembly
  vtkActor *Actor; //selected actor
  vtkMapper *Mapper; //selected mapper
  vtkDataSet *DataSet; //selected dataset
  float GlobalTMin; //parametric coordinate along pick ray where hit occured
  vtkTransform Transform; //use to perform ray transformation
  vtkActorCollection Actors; //candidate actors (based on bounding box)
};

inline vtkActorCollection* vtkPicker::GetActors() {return &(this->Actors);}

inline int vtkPicker::Pick(float selectionPt[3], vtkRenderer *renderer)
{
  return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], renderer);
}

#endif


