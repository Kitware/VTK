/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSinglePassVolumeMapper_h
#define __vtkSinglePassVolumeMapper_h

#include "vtkVolumeModule.h" // For export macro

#include <vtkVolumeMapper.h>
#include <vtkSetGet.h>

//----------------------------------------------------------------------------
///
/// \brief The vtkSinglePassVolumeMapper class
///
class VTKVOLUME_EXPORT vtkSinglePassVolumeMapper : public vtkVolumeMapper
{
  public:
    static vtkSinglePassVolumeMapper* New();

    vtkTypeMacro(vtkSinglePassVolumeMapper, vtkVolumeMapper);
    void PrintSelf( ostream& os, vtkIndent indent );

    virtual void Render(vtkRenderer *ren, vtkVolume *vol);

    /// Set/Get the distance between samples used for rendering
    /// Initial value is 1.0.
    vtkSetMacro(SampleDistance, double);
    vtkGetMacro(SampleDistance, double);

  protected:
    vtkSinglePassVolumeMapper();
    ~vtkSinglePassVolumeMapper();

    int ValidateRender(vtkRenderer* ren, vtkVolume* vol);
    void GPURender(vtkRenderer *ren, vtkVolume *vol);

    double SampleDistance;

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkSinglePassVolumeMapper(const vtkSinglePassVolumeMapper&);  // Not implemented.
    void operator=(const vtkSinglePassVolumeMapper&);  // Not implemented.
};

#endif // __vtkSinglePassVolumeMapper_h
