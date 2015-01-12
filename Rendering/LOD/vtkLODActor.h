/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLODActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLODActor - an actor that supports multiple levels of detail
// .SECTION Description
// vtkLODActor is an actor that stores multiple levels of detail (LOD) and
// can automatically switch between them. It selects which level of detail to
// use based on how much time it has been allocated to render. Currently a
// very simple method of TotalTime/NumberOfActors is used. (In the future
// this should be modified to dynamically allocate the rendering time between
// different actors based on their needs.)
//
// There are three levels of detail by default. The top level is just the
// normal data. The lowest level of detail is a simple bounding box outline
// of the actor. The middle level of detail is a point cloud of a fixed
// number of points that have been randomly sampled from the mapper's input
// data. Point attributes are copied over to the point cloud. These two
// lower levels of detail are accomplished by creating instances of a
// vtkOutlineFilter (low-res) and vtkMaskPoints (medium-res). Additional
// levels of detail can be add using the AddLODMapper() method.
//
// To control the frame rate, you typically set the vtkRenderWindowInteractor
// DesiredUpdateRate and StillUpdateRate. This then will cause vtkLODActor
// to adjust its LOD to fulfill the requested update rate.
//
// For greater control on levels of detail, see also vtkLODProp3D. That
// class allows arbitrary definition of each LOD.

// .SECTION Caveats
// If you provide your own mappers, you are responsible for setting its
// ivars correctly, such as ScalarRange, LookupTable, and so on.
//
// On some systems the point cloud rendering (the default, medium level of
// detail) can result in points so small that they can hardly be seen. In
// this case, use the GetProperty()->SetPointSize() method to increase the
// rendered size of the points.

// .SECTION see also
// vtkActor vtkRenderer vtkLODProp3D

#ifndef vtkLODActor_h
#define vtkLODActor_h

#include "vtkRenderingLODModule.h" // For export macro
#include "vtkActor.h"

class vtkMapper;
class vtkMapperCollection;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkViewport;
class vtkWindow;

class VTKRENDERINGLOD_EXPORT vtkLODActor : public vtkActor
{
public:
  vtkTypeMacro(vtkLODActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a vtkLODActor with the following defaults: origin(0,0,0)
  // position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0). NumberOfCloudPoints is set to 150.
  static vtkLODActor* New();

  // Description:
  // This causes the actor to be rendered.
  // It, in turn, will render the actor's property and then mapper.
  virtual void Render(vtkRenderer *, vtkMapper *);

  // Description:
  // This method is used internally by the rendering process. We overide
  // the superclass method to properly set the estimated render time.
  int RenderOpaqueGeometry(vtkViewport* viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow*);

  // Description:
  // Add another level of detail.
  // They do not have to be in any order of complexity.
  void AddLODMapper(vtkMapper* mapper);

  // Description:
  // You may plug in your own filters to decimate/subsample the input.
  // The default is to use a vtkOutlineFilter (low-res) and vtkMaskPoints
  // (medium-res).
  virtual void SetLowResFilter(vtkPolyDataAlgorithm*);
  virtual void SetMediumResFilter(vtkPolyDataAlgorithm*);
  vtkGetObjectMacro(LowResFilter, vtkPolyDataAlgorithm);
  vtkGetObjectMacro(MediumResFilter, vtkPolyDataAlgorithm);

  // Description:
  // Set/Get the number of random points for the point cloud.
  vtkGetMacro(NumberOfCloudPoints, int);
  vtkSetMacro(NumberOfCloudPoints, int);

  // Description:
  // All the mappers for different LODs are stored here.
  // The order is not important.
  vtkGetObjectMacro(LODMappers, vtkMapperCollection);

  // Description:
  // When this objects gets modified, this method also modifies the object.
  void Modified();

  // Description:
  // Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

protected:
  vtkLODActor();
  ~vtkLODActor();

  vtkActor* Device;
  vtkMapperCollection* LODMappers;

  // We can create our own LOD filters. The default is to use a
  //
  vtkPolyDataAlgorithm* LowResFilter;
  vtkPolyDataAlgorithm* MediumResFilter;
  vtkPolyDataMapper* LowMapper;
  vtkPolyDataMapper* MediumMapper;

  vtkTimeStamp BuildTime;
  int NumberOfCloudPoints;

  virtual void CreateOwnLODs();
  virtual void UpdateOwnLODs();
  virtual void DeleteOwnLODs();

private:
  vtkLODActor(const vtkLODActor&);  // Not implemented.
  void operator=(const vtkLODActor&);  // Not implemented.
};

#endif
