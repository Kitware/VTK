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

  protected:
    vtkSinglePassVolumeMapper();
    ~vtkSinglePassVolumeMapper();

    class vtkInternal;
    vtkInternal* Implementation;

private:
    vtkSinglePassVolumeMapper(const vtkSinglePassVolumeMapper&);  // Not implemented.
    void operator=(const vtkSinglePassVolumeMapper&);  // Not implemented.
};

#endif // __vtkSinglePassVolumeMapper_h
