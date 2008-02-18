/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricLODActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadricLODActor - a specific level-of-detail strategy using the 
// quadric clustering decimation algorithm
// .SECTION Description
// vtkQuadricLODActor implements a specific strategy for level-of-detail
// using the vtkQuadricClustering decimation algorithm. It supports only two
// levels of detail: full resolution and a decimated version. The decimated
// LOD is generated using a tuned strategy to produce output consistent with
// the requested interactive frame rate (i.e., the
// vtkRenderWindowInteractor's DesiredUpdateRate). It also makes use of
// display lists for performance, and adjusts the vtkQuadricClustering
// algorithm to take into account the dimensionality of the data (e.g., 2D,
// x-y surfaces may be binned into n x n x 1 to reduce extra polygons in the
// z-direction). Finally, the filter may optionally be set in "Static" mode
// (this is analogous to the 
//
// .SECTION Caveats
// By default the algorithm is set up to pre-compute the LODs. That is, on
// the first render (whether a full resolution render or interactive render)
// the LOD is computed if necessary. This can be changed so that the LOD
// construction is deferred until the first interactive render.
//
// This class can be used as a direct replacement for vtkActor. It may also be
// used as a replacement for vtkFollower's (the ability to track a camera is 
// provided).

// .SECTION See Also
// vtkLODActor vtkQuadricClustering

#ifndef __vtkQuadricLODActor_h
#define __vtkQuadricLODActor_h

#include "vtkActor.h"

class vtkQuadricClustering;
class vtkPolyDataMapper;
class vtkCamera;
class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkQuadricLODActor : public vtkActor
{
public:
  // Description:
  // Creates a vtkQuadricLODActor. 
  static vtkQuadricLODActor *New();

  // Description:
  // Standard class methods.
  vtkTypeRevisionMacro(vtkQuadricLODActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify whether to build the LOD immediately (i.e., on the first render)
  // or to wait until the LOD is requested in a subsequent render. By default,
  // LOD construction is not deferred (DeferLODConstruction is false).
  vtkSetMacro(DeferLODConstruction,int);
  vtkGetMacro(DeferLODConstruction,int);
  vtkBooleanMacro(DeferLODConstruction,int);

  // Description:
  // Turn on/off a flag to control whether the underlying pipeline is static.
  // If static, this means that the data pipeline executes once and then not 
  // again until the user manually modifies this class. By default, Static is
  // off because trying to debug this is tricky, and you should only use it
  // when you know what you are doing.
  vtkSetMacro(Static,int);
  vtkGetMacro(Static,int);
  vtkBooleanMacro(Static,int);

  // Description:
  // Provide a hint as to the dimension of the data that is being processed
  // into a LOD. This allows the vtkQuadricClustering method to do a better
  // job of decimating the data. The hint will only be followed if the input
  // data is consistent with the CollapseDimensionRatio.
  vtkSetClampMacro(DataDimension,int,0,3);
  vtkGetMacro(DataDimension,int);

  // Description:
  // Specify the ratio of short edge of input bounding box, to long edge, which
  // is used to collapse the data dimension (and set the quadric bin size to
  // one). By default, this value is 0.10.
  vtkSetClampMacro(CollapseDimensionRatio,double,0.0,1.0);
  vtkGetMacro(CollapseDimensionRatio,double);

//BTX
  enum DataConfigurationEnum
  {
    UNKNOWN=0,
    XLINE,YLINE,ZLINE,
    XYPLANE,XZPLANE,YZPLANE,
    XYZVOLUME
  };
//ETX
  // Description:
  // Force the binning of the quadric clustering according to application 
  // knowledge relative to the dimension of the data. For example, if you
  // know your data lies in a 2D x-y plane, the performance of the quadric
  // clustering algorithm can be greatly improved by indicating this (i.e.,
  // the number of resulting triangles, and the quality of the decimation
  // version is better). Setting this parameter overrides the data dimension
  // hint.
  vtkSetClampMacro(DataConfiguration,int,UNKNOWN,XYZVOLUME);
  vtkGetMacro(DataConfiguration,int);
  void SetDataConfigurationToUnknown()
    {this->SetDataConfiguration(UNKNOWN);}
  void SetDataConfigurationToXLine()
    {this->SetDataConfiguration(XLINE);}
  void SetDataConfigurationToYLine()
    {this->SetDataConfiguration(YLINE);}
  void SetDataConfigurationToZLine()
    {this->SetDataConfiguration(ZLINE);}
  void SetDataConfigurationToXYPlane()
    {this->SetDataConfiguration(XYPLANE);}
  void SetDataConfigurationToYZPlane()
    {this->SetDataConfiguration(YZPLANE);}
  void SetDataConfigurationToXZPlane()
    {this->SetDataConfiguration(XZPLANE);}
  void SetDataConfigurationToXYZVolume()
    {this->SetDataConfiguration(XYZVOLUME);}

  // Description:
  // This class will create a vtkQuadricClustering algorithm automatically.
  // However, if you would like to specify the filter to use, or to access it
  // and configure it, these method provide access to the filter.
  void SetLODFilter(vtkQuadricClustering *lodFilter);
  vtkGetObjectMacro(LODFilter,vtkQuadricClustering);

  // Description:
  // Specify the maximum display list size. This variable is used to determine
  // whether to use display lists (ImmediateModeRenderingOff) or not. 
  // Controlling display list size is important to prevent program crashes (i.e.,
  // overly large display lists on some graphics hardware will cause faults).
  // The display list size is the length of the vtkCellArray representing the
  // topology of the input vtkPolyData.
  vtkSetClampMacro(MaximumDisplayListSize,int,1000,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumDisplayListSize,int);

//BTX
  enum PropTypeEnum
  {
    FOLLOWER=0,
    ACTOR
  };
//ETX
  // Description:
  // Indicate that this actor is actually a follower. By default, the prop
  // type is a vtkActor.
  vtkSetClampMacro(PropType,int,FOLLOWER,ACTOR);
  vtkGetMacro(PropType,int);
  void SetPropTypeToFollower()
    {this->SetPropType(FOLLOWER);}
  void SetPropTypeToActor()
    {this->SetPropType(ACTOR);}

  // Description:
  // Set/Get the camera to follow. This method is only applicable when the 
  // prop type is set to a vtkFollower.
  void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera,vtkCamera);

  // Description:
  // This causes the actor to be rendered. Depending on the frame rate request,
  // it will use either a full resolution render or an interactive render (i.e.,
  // it will use the decimated geometry).
  virtual void Render(vtkRenderer *, vtkMapper *);

  // Description:
  // This method is used internally by the rendering process. We overide 
  // the superclass method to properly set the estimated render time.
  int RenderOpaqueGeometry(vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkQuadricLODActor();
  ~vtkQuadricLODActor();

  // Renders the LOD
  vtkActor          *LODActor;
  vtkPolyDataMapper *LODMapper;

  // Keep track of the requested interactive frame rate
  double CachedInteractiveFrameRate;

  // Support various strategies
  vtkQuadricClustering *LODFilter;

  // Specify whether the mapper's should be set in to Static mode.
  int Static;
  
  // The dimension of the data
  int DataDimension;
  double CollapseDimensionRatio;
  int DataConfiguration;
  
  // Control whether this is a follower or regular actor
  int        PropType;
  vtkCamera *Camera;

  // Control what size (in terms of number of graphics primitives)
  // where display lists should be used.
  int MaximumDisplayListSize;

  // Specify to defer construction of the LOD.
  int DeferLODConstruction;

  // Keep track of building
  vtkTimeStamp BuildTime;

  // Helper function determines display list size
  vtkIdType GetDisplayListSize(vtkPolyData *pd);
  
private:
  vtkQuadricLODActor(const vtkQuadricLODActor&);  // Not implemented.
  void operator=(const vtkQuadricLODActor&);  // Not implemented.
};

#endif


