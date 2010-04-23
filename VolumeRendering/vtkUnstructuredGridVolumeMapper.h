/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridVolumeMapper - Abstract class for a unstructured grid volume mapper

// .SECTION Description
// vtkUnstructuredGridVolumeMapper is the abstract definition of a volume mapper for 
// unstructured data (vtkUnstructuredGrid).  Several  basic types of volume mappers 
// are supported as subclasses

// .SECTION see also
// vtkUnstructuredGridVolumeRayCastMapper

#ifndef __vtkUnstructuredGridVolumeMapper_h
#define __vtkUnstructuredGridVolumeMapper_h

#include "vtkAbstractVolumeMapper.h"

class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGrid;
class vtkWindow;


class VTK_VOLUMERENDERING_EXPORT vtkUnstructuredGridVolumeMapper : public vtkAbstractVolumeMapper
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeMapper,vtkAbstractVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/Get the input data
  virtual void SetInput( vtkUnstructuredGrid * );
  virtual void SetInput( vtkDataSet * );
  vtkUnstructuredGrid *GetInput();

  vtkSetMacro( BlendMode, int );
  void SetBlendModeToComposite()
    { this->SetBlendMode( vtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND ); }
  void SetBlendModeToMaximumIntensity()
    { this->SetBlendMode( vtkUnstructuredGridVolumeMapper::MAXIMUM_INTENSITY_BLEND ); }
  vtkGetMacro( BlendMode, int );
  

//BTX  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};
  
  enum 
  {
    COMPOSITE_BLEND,
    MAXIMUM_INTENSITY_BLEND
  };
  
//ETX
  
protected:
  vtkUnstructuredGridVolumeMapper();
  ~vtkUnstructuredGridVolumeMapper();

  int   BlendMode;

  virtual int FillInputPortInformation(int, vtkInformation*);

private:
  vtkUnstructuredGridVolumeMapper(const vtkUnstructuredGridVolumeMapper&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeMapper&);  // Not implemented.
};


#endif


