/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class VTK_EXPORT vtk3DSImporter : public vtkImporter
{
public:
  vtk3DSImporter();
  static vtk3DSImporter *New() {return new vtk3DSImporter;};
  char *GetClassName() {return "vtk3DSImporter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  OmniLight *OmniList;
  SpotLight *SpotLightList;
  Camera    *CameraList;
  Mesh      *MeshList;
  Material  *MaterialList;
  MatProp   *MatPropList;

protected:
  int ImportBegin ();
  void ImportActors (vtkRenderer *renderer);
  void ImportCameras (vtkRenderer *renderer);
  void ImportLights (vtkRenderer *renderer);
  void ImportProperties (vtkRenderer *renderer);
  vtkPolyData *GeneratePolyData (Mesh *meshPtr);
  int Read3DS ();
};

#endif

