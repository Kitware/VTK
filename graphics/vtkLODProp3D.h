/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLODProp3D.h
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

#include "vtkProp3D.h"
#include "vtkTransform.h"

class vtkRenderer;
class vtkMapper;
class vtkVolumeMapper;
class vtkProperty;
class vtkVolumeProperty;
class vtkTexture;

typedef struct
{
  vtkProp3D   *Prop3D;
  int         Prop3DType;
  int         ID;
  float       EstimatedTime;
} vtkLODProp3DEntry;

class VTK_EXPORT vtkLODProp3D : public vtkProp3D
{
public:
  static vtkLODProp3D *New() {return new vtkLODProp3D;};

  const char *GetClassName() {return "vtkLODProp3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard vtkProp method to get 3D bounds of a 3D prop
  float *GetBounds();

  // Description:
  // Do we need to ray cast this prop?
  int RequiresRayCasting();

  // Description:
  // Does this prop render into an image?
  int RequiresRenderingIntoImage();
 
  // Description:
  // Add a level of detail with a given mapper, property, texture, and
  // guess of rendering time.  The property and texture fields can be set
  // to NULL (the other methods are included for script access where null
  // variables are not allowed). The time field can be set to 0.0 indicating
  // that no initial guess for rendering time is being supplied.
  // The returned integer value is an ID that can be used later to delete
  // this LOD, or set it as the selected LOD.
  int AddLOD( vtkMapper *m, vtkProperty *p, vtkTexture *t, float time );
  int AddLOD( vtkMapper *m, vtkProperty *p, float time );
  int AddLOD( vtkMapper *m, vtkTexture *t, float time );
  int AddLOD( vtkMapper *m, float time );
  int AddLOD( vtkVolumeMapper *m, vtkVolumeProperty *p, float time );
  int AddLOD( vtkVolumeMapper *m, float time );

  // Description:
  // Delete a level of detail given an ID. This is the ID returned by the
  // AddLOD method
  void RemoveLOD( int id );

  // Description:
  // Methods to set / get the property of an LOD. Since the LOD could be
  // a volume or an actor, you have to pass in the pointer to the property
  // to get it. The returned property will be NULL if the id is not valid,
  // or the property is of the wrong type for the corresponding Prop3D.
  void SetLODProperty( int id, vtkProperty  *p );
  void GetLODProperty( int id, vtkProperty  *p );
  void SetLODProperty( int id, vtkVolumeProperty  *p );
  void GetLODProperty( int id, vtkVolumeProperty  *p );

  // Description:
  // Methods to set / get the mapper of an LOD. Since the LOD could be
  // a volume or an actor, you have to pass in the pointer to the mapper
  // to get it. The returned mapper will be NULL if the id is not valid,
  // or the mapper is of the wrong type for the corresponding Prop3D.
  void SetLODMapper( int id, vtkMapper  *m );
  void GetLODMapper( int id, vtkMapper  *m );
  void SetLODMapper( int id, vtkVolumeMapper  *m );
  void GetLODMapper( int id, vtkVolumeMapper  *m );

  // Description:
  // Methods to set / get the texture of an LOD. This method is only
  // valid for LOD ids that are Actors (not Volumes)
  void SetLODTexture( int id, vtkTexture *t );
  void GetLODTexture( int id, vtkTexture *t );

  // Description:
  // Access method that can be used to find out the estimated render time
  // (the thing used to select an LOD) for a given LOD ID or index. 
  // Value is returned in seconds.
  float GetLODEstimatedRenderTime( int id );
  float GetLODIndexEstimatedRenderTime( int index );

  // Description:
  // Turn on / off automatic selection of LOD. 
  // This is on by default. If it is off, then the SelectedLODID is 
  // rendered regarless of rendering time or desired update rate. 
  vtkSetClampMacro( AutomaticLODSelection, int, 0, 1 );
  vtkGetMacro( AutomaticLODSelection, int );
  vtkBooleanMacro( AutomaticLODSelection, int );

  // Description:
  // Set the id of the LOD that is to be drawn when automatic LOD selection
  // is turned off.
  vtkSetMacro( SelectedLODID, int );
  vtkGetMacro( SelectedLODID, int );

  // Description:
  // Get the ID of the previously (during the last render) selected LOD index
  int GetLastRenderedLODID();

//BTX

  // Description:
  // Support the standard render methods.
  int RenderOpaqueGeometry(vtkViewport *viewport);
  int RenderTranslucentGeometry(vtkViewport *viewport);
  int RenderIntoImage(vtkViewport *viewport);
  int CastViewRay( VTKRayCastRayInfo *rayInfo );
  int InitializeRayCasting( vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Used by the culler / renderer to set the allocated render time for this
  // prop. This is based on the desired update rate, and possibly some other
  // properties such as potential screen coverage of this prop.
  void SetAllocatedRenderTime( float t );

  // Description:
  // Override method from vtkProp in order to push this call down to the
  // selected LOD as well.
  virtual void AddEstimatedRenderTime( float t );

//ETX

protected:
  vtkLODProp3D();
  ~vtkLODProp3D();
  vtkLODProp3D(const vtkLODProp3D&) {};
  void operator=(const vtkLODProp3D&) {};

  vtkLODProp3DEntry *LODs;
  int               NumberOfEntries;
  int               NumberOfLODs;
  int               CurrentIndex;

  int               GetNextEntryIndex();
  int               ConvertIDToIndex( int id );
  int               SelectedLODIndex;

  int               AutomaticLODSelection;
  int               SelectedLODID;
};

#endif

