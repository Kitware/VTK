/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiGroupPolyDataMapper - a class that renders hierarchical polygonal data
// .SECTION Description
// This class uses a set of vtkPolyDataMappers to render input data
// which may be hierarchical. The input to this mapper may be
// either vtkPolyData or a vtkMultiGroupDataSet built from 
// polydata. If something other than vtkPolyData is encountered,
// an error message will be produced.
//
// .SECTION see also
// vtkPolyDataMapper

#ifndef __vtkMultiGroupPolyDataMapper_h
#define __vtkMultiGroupPolyDataMapper_h

#include "vtkMapper.h"

class vtkPolyDataMapper;
class vtkInformation;
class vtkRenderer;
class vtkActor;
class vtkMultiGroupPolyDataMapperInternals;

class VTK_RENDERING_EXPORT vtkMultiGroupPolyDataMapper : public vtkMapper 
{

public:
  static vtkMultiGroupPolyDataMapper *New();
  vtkTypeRevisionMacro(vtkMultiGroupPolyDataMapper, vtkMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Standard method for rendering a mapper. This method will be 
  // called by the actor.
  void Render(vtkRenderer *ren, vtkActor *a);
  
  // Description:
  // Standard vtkProp method to get 3D bounds of a 3D prop
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->Superclass::GetBounds( bounds ); };  

  // Description:
  // Release the underlying resources associated with this mapper  
  void ReleaseGraphicsResources(vtkWindow *);


protected:
  vtkMultiGroupPolyDataMapper();
  ~vtkMultiGroupPolyDataMapper();
  
  // Description:
  // We need to override this method because the standard streaming
  // demand driven pipeline is not what we want - we are expecting
  // hierarchical data as input
  vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual int FillInputPortInformation(int port, vtkInformation* info);
  
  // Description:
  // This is the build method for creating the internal polydata
  // mapper that do the actual work
  void BuildPolyDataMapper();
  
  // Description:
  // Need to loop over the hierarchy to compute bounds
  void ComputeBounds();

  // Description:
  // Time stamp for computation of bounds.
  vtkTimeStamp BoundsMTime;

  // Description:
  // These are the internal polydata mapper that do the
  // rendering. We save then so that they can keep their
  // display lists.
  vtkMultiGroupPolyDataMapperInternals *Internal;
  
  // Description:
  // Time stamp for when we need to update the 
  // internal mappers
  vtkTimeStamp InternalMappersBuildTime; 
  
private:
  vtkMultiGroupPolyDataMapper(const vtkMultiGroupPolyDataMapper&);  // Not implemented.
  void operator=(const vtkMultiGroupPolyDataMapper&);    // Not implemented.
};

#endif
