/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkOBJImporter.h
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkOBJImporter - import from .obj wavefront files
// .SECTION Description - suports import of polydata, textures, and actors
//                        from Wavefront .obj & associated .mtl files.
// .SECTION Thanks - Peter Karasev (Georgia Tech / Keysight Technologies Inc),
//                   Allen Tannenbaum (SUNY Stonybrook), Patricio Vela (Georgia Tech)
// .SECTION See Also
//  vtkImporter

#ifndef vtkOBJImporter_h
#define vtkOBJImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkSmartPointer.h"
#include "vtkImporter.h"

class vtkRenderWindow;
class vtkRenderer;
class vtkPolydata;
class vtkOBJPolyDataProcessor;

/** @note{updated by peter karasev, 2015 to read texture coordinates + material properties}
    @note{An example of a supported (*).mtl file is show below.
             Lighting values and texture images are specified, and a corresponding vtkActor
             with properties and vtkTexture will be created upon import. }

        # Wavefront material file saved from Meshlab
        newmtl material_0
        Ka 0.400000 0.400000 0.400000
        Kd 0.5 0.5 0.5
        Ks 0.85 0.9 0.9
        illum 2
        Ns 0.000000
        map_Kd map1024.png

        newmtl material_1
        Ka 0.200000 0.200000 0.200000
        Kd 0.666667 0.666667 0.666667
        Ks 1.000000 0.9 1.000000
        illum 2
        Ns 0.000000
        map_Kd flare.jpg

   **/
class VTKIOIMPORT_EXPORT vtkOBJImporter : public vtkImporter
{
public:
  static vtkOBJImporter *New();

  vtkTypeMacro(vtkOBJImporter,vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the file to read.
  void SetFileName(const char* arg);
  void SetFileNameMTL(const char* arg);
  void SetTexturePath(const char* path);
  const char* GetFileName() const;
  const char* GetFileNameMTL() const;
  const char* GetTexturePath() const;

protected:
  vtkOBJImporter();
  ~vtkOBJImporter();

  virtual int  ImportBegin() /*override*/;
  virtual void ImportEnd () /*override*/;
  virtual void ReadData() /* override */;

  vtkSmartPointer<vtkOBJPolyDataProcessor>   Impl;

private:
  vtkOBJImporter(const vtkOBJImporter&);  // Not implemented.
  void operator=(const vtkOBJImporter&);  // Not implemented.
};



#endif
// VTK-HeaderTest-Exclude: vtkOBJImporter.h
