/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.h
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
// .NAME vtk3DSImporter - imports 3D Studio files.
// .SECTION Description
// vtk3DSImporter imports 3D Studio files into vtk.

// .SECTION See Also
// vtkImporter


#ifndef __vtk3DSImporter_h
#define __vtk3DSImporter_h

#include <stdio.h>
#include "vtkImporter.h"
#include "vtk3DS.h"
#include "vtkPolyData.h"

class VTK_HYBRID_EXPORT vtk3DSImporter : public vtkImporter
{
public:
  static vtk3DSImporter *New();

  vtkTypeMacro(vtk3DSImporter,vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the computation of normals. If on, imported geometry will
  // be run through vtkPolyDataNormals.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Return the file pointer to the open file.
  FILE *GetFileFD() {return this->FileFD;};

  OmniLight *OmniList;
  SpotLight *SpotLightList;
  Camera    *CameraList;
  Mesh      *MeshList;
  Material  *MaterialList;
  MatProp   *MatPropList;

protected:
  vtk3DSImporter();
  ~vtk3DSImporter();

  virtual int ImportBegin ();
  virtual void ImportEnd ();
  virtual void ImportActors (vtkRenderer *renderer);
  virtual void ImportCameras (vtkRenderer *renderer);
  virtual void ImportLights (vtkRenderer *renderer);
  virtual void ImportProperties (vtkRenderer *renderer);
  vtkPolyData *GeneratePolyData (Mesh *meshPtr);
  int Read3DS ();

  char *FileName;
  FILE *FileFD;
  int ComputeNormals;
private:
  vtk3DSImporter(const vtk3DSImporter&);  // Not implemented.
  void operator=(const vtk3DSImporter&);  // Not implemented.
};

#endif

