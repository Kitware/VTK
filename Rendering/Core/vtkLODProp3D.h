/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLODProp3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLODProp3D - level of detail 3D prop
// .SECTION Description
// vtkLODProp3D is a class to support level of detail rendering for Prop3D.
// Any number of mapper/property/texture items can be added to this object.
// Render time will be measured, and will be used to select a LOD based on
// the AllocatedRenderTime of this Prop3D. Depending on the type of the
// mapper/property, a vtkActor or a vtkVolume will be created behind the
// scenes.

// .SECTION See Also
// vtkProp3D vtkActor vtkVolume vtkLODActor

#ifndef __vtkLODProp3D_h
#define __vtkLODProp3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkRenderer;
class vtkMapper;
class vtkAbstractVolumeMapper;
class vtkAbstractMapper3D;
class vtkImageMapper3D;
class vtkProperty;
class vtkVolumeProperty;
class vtkImageProperty;
class vtkTexture;
class vtkLODProp3DCallback;

typedef struct
{
  vtkProp3D   *Prop3D;
  int         Prop3DType;
  int         ID;
  double       EstimatedTime;
  int         State;
  double       Level;
} vtkLODProp3DEntry;

class VTKRENDERINGCORE_EXPORT vtkLODProp3D : public vtkProp3D
{
public:
  // Description:
  // Create an instance of this class.
  static vtkLODProp3D *New();

  vtkTypeMacro(vtkLODProp3D, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard vtkProp method to get 3D bounds of a 3D prop
  double *GetBounds();
  void GetBounds(double bounds[6])
    { this->vtkProp3D::GetBounds( bounds ); }

  // Description:
  // Add a level of detail with a given mapper, property, backface property,
  // texture, and guess of rendering time.  The property and texture fields
  // can be set to NULL (the other methods are included for script access
  // where null variables are not allowed). The time field can be set to 0.0
  // indicating that no initial guess for rendering time is being supplied.
  // The returned integer value is an ID that can be used later to delete
  // this LOD, or set it as the selected LOD.
  int AddLOD( vtkMapper *m, vtkProperty *p, vtkProperty *back,
              vtkTexture *t, double time );
  int AddLOD( vtkMapper *m, vtkProperty *p, vtkTexture *t, double time );
  int AddLOD( vtkMapper *m, vtkProperty *p, vtkProperty *back, double time );
  int AddLOD( vtkMapper *m, vtkProperty *p, double time );
  int AddLOD( vtkMapper *m, vtkTexture *t, double time );
  int AddLOD( vtkMapper *m, double time );
  int AddLOD( vtkAbstractVolumeMapper *m, vtkVolumeProperty *p, double time );
  int AddLOD( vtkAbstractVolumeMapper *m, double time );
  int AddLOD( vtkImageMapper3D *m, vtkImageProperty *p, double time );
  int AddLOD( vtkImageMapper3D *m, double time );

  // Description:
  // Get the current number of LODs.
  vtkGetMacro(NumberOfLODs, int);

  // Description:
  // Get the current index, used to determine the ID of the next LOD that is
  // added.  Useful for guessing what IDs have been used (with NumberOfLODs,
  // without depending on the constructor initialization to 1000.
  vtkGetMacro(CurrentIndex, int);

  // Description:
  // Delete a level of detail given an ID. This is the ID returned by the
  // AddLOD method
  void RemoveLOD(int id);

  // Description:
  // Methods to set / get the property of an LOD. Since the LOD could be
  // a volume or an actor, you have to pass in the pointer to the property
  // to get it. The returned property will be NULL if the id is not valid,
  // or the property is of the wrong type for the corresponding Prop3D.
  void SetLODProperty(int id, vtkProperty *p);
  void GetLODProperty(int id, vtkProperty **p);
  void SetLODProperty(int id, vtkVolumeProperty *p);
  void GetLODProperty(int id, vtkVolumeProperty **p);
  void SetLODProperty(int id, vtkImageProperty *p);
  void GetLODProperty(int id, vtkImageProperty **p);

  // Description:
  // Methods to set / get the mapper of an LOD. Since the LOD could be
  // a volume or an actor, you have to pass in the pointer to the mapper
  // to get it. The returned mapper will be NULL if the id is not valid,
  // or the mapper is of the wrong type for the corresponding Prop3D.
  void SetLODMapper(int id, vtkMapper *m);
  void GetLODMapper(int id, vtkMapper **m);
  void SetLODMapper(int id, vtkAbstractVolumeMapper *m);
  void GetLODMapper(int id, vtkAbstractVolumeMapper **m);
  void SetLODMapper(int id, vtkImageMapper3D *m);
  void GetLODMapper(int id, vtkImageMapper3D **m);

  // Description:
  // Get the LODMapper as an vtkAbstractMapper3D.  It is the user's
  // respondibility to safe down cast this to a vtkMapper or vtkVolumeMapper
  // as appropriate.
  vtkAbstractMapper3D *GetLODMapper(int id);

  // Description:
  // Methods to set / get the backface property of an LOD. This method is only
  // valid for LOD ids that are Actors (not Volumes)
  void SetLODBackfaceProperty(int id, vtkProperty *t);
  void GetLODBackfaceProperty(int id, vtkProperty **t);

  // Description:
  // Methods to set / get the texture of an LOD. This method is only
  // valid for LOD ids that are Actors (not Volumes)
  void SetLODTexture(int id, vtkTexture *t);
  void GetLODTexture(int id, vtkTexture **t);

  // Description:
  // Enable / disable a particular LOD. If it is disabled, it will not
  // be used during automatic selection, but can be selected as the
  // LOD if automatic LOD selection is off.
  void EnableLOD(int id);
  void DisableLOD(int id);
  int IsLODEnabled(int id);

  // Description:
  // Set the level of a particular LOD. When a LOD is selected for
  // rendering because it has the largest render time that fits within
  // the allocated time, all LOD are then checked to see if any one can
  // render faster but has a lower (more resolution/better) level.
  // This quantity is a double to ensure that a level can be inserted
  // between 2 and 3.
  void SetLODLevel(int id, double level);
  double GetLODLevel(int id );
  double GetLODIndexLevel(int index);

  // Description:
  // Access method that can be used to find out the estimated render time
  // (the thing used to select an LOD) for a given LOD ID or index.
  // Value is returned in seconds.
  double GetLODEstimatedRenderTime(int id);
  double GetLODIndexEstimatedRenderTime(int index);

  // Description:
  // Turn on / off automatic selection of LOD.
  // This is on by default. If it is off, then the SelectedLODID is
  // rendered regardless of rendering time or desired update rate.
  vtkSetClampMacro(AutomaticLODSelection, int, 0, 1);
  vtkGetMacro(AutomaticLODSelection, int);
  vtkBooleanMacro(AutomaticLODSelection, int);

  // Description:
  // Set the id of the LOD that is to be drawn when automatic LOD selection
  // is turned off.
  vtkSetMacro(SelectedLODID, int);
  vtkGetMacro(SelectedLODID, int);

  // Description:
  // Get the ID of the previously (during the last render) selected LOD index
  int GetLastRenderedLODID();

  // Description:
  // Get the ID of the appropriate pick LOD index
  int GetPickLODID(void);

  // Description:
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors(vtkPropCollection *);
  virtual void GetVolumes(vtkPropCollection *);

  // Description:
  // Set the id of the LOD that is to be used for picking when  automatic
  // LOD pick selection is turned off.
  void SetSelectedPickLODID(int id);
  vtkGetMacro(SelectedPickLODID, int);

  // Description:
  // Turn on / off automatic selection of picking LOD.
  // This is on by default. If it is off, then the SelectedLODID is
  // rendered regardless of rendering time or desired update rate.
  vtkSetClampMacro(AutomaticPickLODSelection, int, 0, 1);
  vtkGetMacro(AutomaticPickLODSelection, int);
  vtkBooleanMacro(AutomaticPickLODSelection, int);

  // Description:
  // Shallow copy of this vtkLODProp3D.
  void ShallowCopy(vtkProp *prop);

//BTX

  // Description:
  // Support the standard render methods.
  int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry( vtkViewport *ren);
  virtual int RenderVolumetricGeometry( vtkViewport *ren);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Used by the culler / renderer to set the allocated render time for this
  // prop. This is based on the desired update rate, and possibly some other
  // properties such as potential screen coverage of this prop.
  void SetAllocatedRenderTime( double t, vtkViewport *vp );

  // Description:
  // Used when the render process is aborted to restore the previous
  // estimated render time. Overridden here to allow previous time for a
  // particular LOD to be restored - otherwise the time for the last rendered
  // LOD will be copied into the currently selected LOD.
  void RestoreEstimatedRenderTime( );

  // Description:
  // Override method from vtkProp in order to push this call down to the
  // selected LOD as well.
  virtual void AddEstimatedRenderTime( double t, vtkViewport *vp );

//ETX

protected:
  vtkLODProp3D();
  ~vtkLODProp3D();

  int GetAutomaticPickPropIndex(void);

  vtkLODProp3DEntry *LODs;
  int NumberOfEntries;
  int NumberOfLODs;
  int CurrentIndex;

  int GetNextEntryIndex();
  int ConvertIDToIndex( int id );
  int SelectedLODIndex;

  int AutomaticLODSelection;
  int SelectedLODID;
  int SelectedPickLODID;
  int AutomaticPickLODSelection;
  vtkLODProp3DCallback *PickCallback;

private:
  vtkLODProp3D(const vtkLODProp3D&);  // Not implemented.
  void operator=(const vtkLODProp3D&);  // Not implemented.
};

#endif
