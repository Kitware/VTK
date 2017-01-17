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
/**
 * @class   vtk3DSImporter
 * @brief   imports 3D Studio files.
 *
 * vtk3DSImporter imports 3D Studio files into vtk.
 *
 * @sa
 * vtkImporter
*/

#ifndef vtk3DSImporter_h
#define vtk3DSImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"
#include "vtk3DS.h"  // Needed for all the 3DS structures

class vtkPolyData;

class VTKIOIMPORT_EXPORT vtk3DSImporter : public vtkImporter
{
public:
  static vtk3DSImporter *New();

  vtkTypeMacro(vtk3DSImporter,vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify the name of the file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. If on, imported geometry will
   * be run through vtkPolyDataNormals.
   */
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);
  //@}

  /**
   * Return the file pointer to the open file.
   */
  FILE *GetFileFD() {return this->FileFD;};

  vtk3DSOmniLight *OmniList;
  vtk3DSSpotLight *SpotLightList;
  vtk3DSCamera    *CameraList;
  vtk3DSMesh      *MeshList;
  vtk3DSMaterial  *MaterialList;
  vtk3DSMatProp   *MatPropList;

protected:
  vtk3DSImporter();
  ~vtk3DSImporter() VTK_OVERRIDE;

  int ImportBegin () VTK_OVERRIDE;
  void ImportEnd () VTK_OVERRIDE;
  void ImportActors (vtkRenderer *renderer) VTK_OVERRIDE;
  void ImportCameras (vtkRenderer *renderer) VTK_OVERRIDE;
  void ImportLights (vtkRenderer *renderer) VTK_OVERRIDE;
  void ImportProperties (vtkRenderer *renderer) VTK_OVERRIDE;
  vtkPolyData *GeneratePolyData (vtk3DSMesh *meshPtr);
  int Read3DS ();

  char *FileName;
  FILE *FileFD;
  int ComputeNormals;
private:
  vtk3DSImporter(const vtk3DSImporter&) VTK_DELETE_FUNCTION;
  void operator=(const vtk3DSImporter&) VTK_DELETE_FUNCTION;
};

#endif

