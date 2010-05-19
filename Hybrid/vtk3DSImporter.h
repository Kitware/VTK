/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DSImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtk3DSImporter - imports 3D Studio files.
// .SECTION Description
// vtk3DSImporter imports 3D Studio files into vtk.

// .SECTION See Also
// vtkImporter


#ifndef __vtk3DSImporter_h
#define __vtk3DSImporter_h

#include "vtkImporter.h"
#include "vtk3DS.h"  // Needed for all the 3DS structures

class vtkPolyData;

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

  vtk3DSOmniLight *OmniList;
  vtk3DSSpotLight *SpotLightList;
  vtk3DSCamera    *CameraList;
  vtk3DSMesh      *MeshList;
  vtk3DSMaterial  *MaterialList;
  vtk3DSMatProp   *MatPropList;

protected:
  vtk3DSImporter();
  ~vtk3DSImporter();

  virtual int ImportBegin ();
  virtual void ImportEnd ();
  virtual void ImportActors (vtkRenderer *renderer);
  virtual void ImportCameras (vtkRenderer *renderer);
  virtual void ImportLights (vtkRenderer *renderer);
  virtual void ImportProperties (vtkRenderer *renderer);
  vtkPolyData *GeneratePolyData (vtk3DSMesh *meshPtr);
  int Read3DS ();

  char *FileName;
  FILE *FileFD;
  int ComputeNormals;
private:
  vtk3DSImporter(const vtk3DSImporter&);  // Not implemented.
  void operator=(const vtk3DSImporter&);  // Not implemented.
};

#endif

