/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractVolumeMapper.h
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
// .NAME vtkAbstractVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkAbstractVolumeMapper is the abstract definition of a volume mapper.  
// Specific subclasses deal with different specific types of data input

// .SECTION see also
// vtkVolumeMapper vtkUnstructuredGridVolumeMapper

#ifndef __vtkAbstractVolumeMapper_h
#define __vtkAbstractVolumeMapper_h

#include "vtkAbstractMapper3D.h"

class vtkRenderer;
class vtkVolume;
class vtkWindow;
class vtkDataSet;

class VTK_RENDERING_EXPORT vtkAbstractVolumeMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeRevisionMacro(vtkAbstractVolumeMapper,vtkAbstractMapper3D);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();

  // Description:
  // Set/Get the input data
  virtual void SetInput( vtkDataSet * );
  vtkDataSet *GetDataSetInput();

  // Description:
  // Return bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual double *GetBounds();
  virtual void GetBounds(double bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };
  
//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual float GetGradientMagnitudeScale() {return 1.0;};
  virtual float GetGradientMagnitudeBias()  {return 0.0;};
  virtual float GetGradientMagnitudeScale(int) {return 1.0;};
  virtual float GetGradientMagnitudeBias(int)  {return 0.0;};
  

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
  
//ETX

protected:
  vtkAbstractVolumeMapper();
  ~vtkAbstractVolumeMapper();
  
private:
  vtkAbstractVolumeMapper(const vtkAbstractVolumeMapper&);  // Not implemented.
  void operator=(const vtkAbstractVolumeMapper&);  // Not implemented.
};


#endif


