/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkLODActor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkLODActor - an actor that supports multiple levels of detail
// .SECTION Description
// vtkLODActor is an actor that stores multiple levels of detail (LOD) and
// can automatically switch between them. It selects which level of detail to
// use based on how much time it has been allocated to render.  Currently a
// very simple method of TotalTime/NumberOfActors is used.  (In the future
// this should be modified to dynamically allocate the rendering time between
// different actors based on their needs.)
//
// There are three levels of detail by default. The top level is just the
// normal data.  The lowest level of detail is a simple bounding box outline
// of the actor. The middle level of detail is a point cloud of a fixed
// number of points that have been randomly sampled from the mapper's input
// data.  Point attributes are copied over to the point cloud.  These two
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

// .SECTION see also
// vtkActor vtkRenderer vtkLODProp3D

#ifndef __vtkLODActor_h
#define __vtkLODActor_h

#include "vtkActor.h"
#include "vtkMaskPoints.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkMapperCollection.h"

class VTK_EXPORT vtkLODActor : public vtkActor
{
public:
  vtkTypeMacro(vtkLODActor,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a vtkLODActor with the following defaults: origin(0,0,0) 
  // position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0). NumberOfCloudPoints is set to 150.
  static vtkLODActor *New();

  // Description:
  // This causes the actor to be rendered. It, in turn, will render the actor's
  // property and then mapper.  
  virtual void Render(vtkRenderer *, vtkMapper *);

  // Description:
  // This method is used internally by the rendering process.
  // We overide the superclass method to properly set the estimated render time.
  int RenderOpaqueGeometry(vtkViewport *viewport);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Add another level of detail.  They do not have to be in any order
  // of complexity.
  void AddLODMapper(vtkMapper *mapper);

  // Description:
  // Set/Get the number of random points for the point cloud.
  vtkGetMacro(NumberOfCloudPoints,int);
  vtkSetMacro(NumberOfCloudPoints,int);

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
  vtkLODActor(const vtkLODActor&);
  void operator=(const vtkLODActor&);

  vtkActor            *Device;
  vtkMapperCollection *LODMappers;

  // stuff for creating our own LOD mappers
  vtkMaskPoints       *MaskPoints;
  vtkOutlineFilter    *OutlineFilter;
  vtkTimeStamp        BuildTime;
  int                 NumberOfCloudPoints;
  vtkPolyDataMapper   *LowMapper;
  vtkPolyDataMapper   *MediumMapper;

  void CreateOwnLODs();
  void UpdateOwnLODs();
  void DeleteOwnLODs();
};

#endif


