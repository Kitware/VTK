/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPicker.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
//
// vtkPicker has hooks for methods to call during the picking process.  These
// methods are StartPickMethod(), PickMethod(), and EndPickMethod() which are
// invoked prior to picking, when something is picked, and after all picking
// candidates have been tested. Note that during the pick process the
// PickAction of vtkProp (and its subclasses such as vtkActor) is called
// prior to the pick action of vtkPicker.

// .SECTION Caveats
// vtkPicker and its subclasses will not pick actors that are "unpickable" 
// (see vtkActor) or are fully transparent.

// .SECTION See Also
// vtkPicker is used for quick picking. If you desire to pick points or
// cells, use the subclass vtkPointPicker or vtkCellPicker, respectively.  Or
// you may use hardware picking to pick any type of vtkProp - see
// vtkPropPicker.

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
  static vtkPicker *New();
  vtkTypeMacro(vtkPicker,vtkObject);
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

  // Description:
  // Return a collection of all the actors that were intersected.
  // This collection is not sorted.
  vtkActorCollection *GetActors();

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

  // Description: Perform pick operation with selection point
  // provided. Normally the first two values for the selection point
  // are x-y pixel coordinate, and the third value is =0. Return
  // non-zero if something was successfully picked.
  int Pick(float selectionPt[3], vtkRenderer *renderer);  

  // Description: Specify function to be called as picking operation
  // begins.
  void SetStartPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Specify function to be called when something is picked.
  void SetPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Specify function to be called after all picking operations have been
  // performed.
  void SetEndPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Set a method to delete user arguments for StartPickMethod.
  void SetStartPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Set a method to delete user arguments for PickMethod.
  void SetPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Set a method to delete user arguments for EndPickMethod.
  void SetEndPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Use these methods to control whether to limit the picking to this list
  // (rather than renderer's actors). Make sure that the pick list contains 
  // actors that refered to by the picker's renderer.
  vtkSetMacro(PickFromList,int);
  vtkGetMacro(PickFromList,int);
  vtkBooleanMacro(PickFromList,int);

  // Description:
  // Initialize list of actors in pick list.
  void InitializePickList();

  // Description:
  // Add an actor to the pick list.
  void AddPickList(vtkActor *a);

  // Description:
  // Delete an actor from the pick list.
  void DeletePickList(vtkActor *a);
  
  // Description:
  // Return the list of actors in the PickList.
  vtkActorCollection *GetPickList() {return this->PickList;}

protected:
  vtkPicker();
  ~vtkPicker();
  vtkPicker(const vtkPicker&) {};
  void operator=(const vtkPicker&) {};

  void MarkPicked(vtkActor *assem, vtkActor *a, vtkMapper *m, 
                  float tMin, float mapperPos[3]);
  virtual float IntersectWithLine(float p1[3], float p2[3], float tol, 
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
  vtkTransform *Transform; //use to perform ray transformation
  vtkActorCollection *Actors; //candidate actors (based on bounding box)
  vtkPoints *PickedPositions; // candidate positions
  
  // the following are used to manage invocation of pick methods
  void (*StartPickMethod)(void *);
  void (*StartPickMethodArgDelete)(void *);
  void *StartPickMethodArg;
  void (*PickMethod)(void *);
  void *PickMethodArg;
  void (*PickMethodArgDelete)(void *);
  void (*EndPickMethod)(void *);
  void (*EndPickMethodArgDelete)(void *);
  void *EndPickMethodArg;
  
  // use the following to control picking from a list
  int PickFromList;
  vtkActorCollection *PickList;
  
};

inline vtkActorCollection* vtkPicker::GetActors() {return this->Actors;}

inline int vtkPicker::Pick(float selectionPt[3], vtkRenderer *renderer)
{
  return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], renderer);
}

#endif


