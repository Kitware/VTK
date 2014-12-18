/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPOVExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPOVExporter.h

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// .NAME vtkPOVExporter - Export scene into povray format.
//
// .SECTION Description
// This Exporter can be attached to a render window in order to generate
// scene description files for the Persistence of Vision Raytracer
// www.povray.org.
//
// .SECTION Thanks
// Li-Ta Lo (ollie@lanl.gov) and Jim Ahrens (ahrens@lanl.gov)
// Los Alamos National Laboratory

#ifndef vtkPOVExporter_h
#define vtkPOVExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkRenderer;
class vtkActor;
class vtkCamera;
class vtkLight;
class vtkPolyData;
class vtkProperty;
class vtkTexture;
class vtkPOVInternals;

class VTKIOEXPORT_EXPORT vtkPOVExporter : public vtkExporter
{
public:
    static vtkPOVExporter *New();
    vtkTypeMacro(vtkPOVExporter, vtkExporter);
    void PrintSelf(ostream& os, vtkIndent indent);

    //Description:
    //The filename to save into.
    vtkSetStringMacro(FileName);
    vtkGetStringMacro(FileName);

protected:
    vtkPOVExporter();
    ~vtkPOVExporter();

    void WriteData();
    virtual void WriteHeader(vtkRenderer *renderer);
    void WriteCamera(vtkCamera *camera);
    void WriteLight(vtkLight *light);
    void WriteProperty(vtkProperty *property);
    void WritePolygons(vtkPolyData *polydata, bool scalar_visible);
    void WriteTriangleStrips(vtkPolyData *strip, bool scalar_visible);

    virtual void WriteActor(vtkActor *actor);

    char *FileName;
    FILE *FilePtr;

private:
    vtkPOVExporter(const vtkPOVExporter&);  // Not implemented.
    void operator=(const vtkPOVExporter&);  // Not implemented.

    vtkPOVInternals *Internals;
};

#endif
