/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVRMLImporter - imports VRML 2.0 files.
// .SECTION Description
//
// vtkVRMLImporter imports VRML 2.0 files into VTK.
// .SECTION Caveats
//
// These nodes are currently supported:
//      Appearance                              IndexedFaceSet
//      Box                                     IndexedLineSet
//      Color                                   Material
//      Cone                                    Shape
//      Coordinate                              Sphere
//      Cylinder                                Transform
//      DirectionalLight
//
// As you can see this implementation focuses on getting the geometry
// translated.  The routes and scripting nodes are ignored since they deal
// with directly accessing a nodes internal structure based on the VRML
// spec. Since this is a translation the internal data structures differ
// greatly from the VRML spec and the External Authoring Interface (see the
// VRML spec). The DEF/USE mechanism does allow the VTK user to extract
// objects from the scene and directly manipulate them using the native
// language (Tcl, Python, Java, or whatever language VTK is wrapped
// in). This, in a way, removes the need for the route and script mechanism
// (not completely though).
// Texture coordinates are attached to the mesh is available but
// image textures are not loaded.
// Viewpoints (camera presets) are not imported.
//
// .SECTION Thanks
//  Thanks to Russ Coucher of Areva for numerous bug fixes and a new test.
//
// .SECTION See Also
// vtkImporter

/* ======================================================================

   Importer based on BNF Yacc and Lex parser definition from:

        **************************************************
        * VRML 2.0 Parser
        * Copyright (C) 1996 Silicon Graphics, Inc.
        *
        * Author(s) :    Gavin Bell
        *                Daniel Woods (first port)
        **************************************************

  Ported to VTK By:     Thomas D. Citriniti
                        Rensselaer Polytechnic Institute
                        citrit@rpi.edu

=======================================================================*/

#ifndef vtkVRMLImporter_h
#define vtkVRMLImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkImporter.h"

class vtkActor;
class vtkAlgorithm;
class vtkProperty;
class vtkLight;
class vtkTransform;
class vtkLookupTable;
class vtkFloatArray;
class vtkPolyDataMapper;
class vtkPoints;
class vtkIdTypeArray;
class vtkVRMLImporterInternal;
class vtkVRMLYaccData;
class vtkCellArray;

class VTKIOIMPORT_EXPORT vtkVRMLImporter : public vtkImporter
{
public:
  static vtkVRMLImporter *New();

  vtkTypeMacro(vtkVRMLImporter, vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify the resolution for Sphere, Cone and Cylinder shape sources.
  // Default is 12.
  vtkSetMacro(ShapeResolution, int);
  vtkGetMacro(ShapeResolution, int);

  // Description:
  // In the VRML spec you can DEF and USE nodes (name them),
  // This routine will return the associated VTK object which
  // was created as a result of the DEF mechanism
  // Send in the name from the VRML file, get the VTK object.
  // You will have to check and correctly cast the object since
  // this only returns vtkObjects.
  vtkObject* GetVRMLDEFObject(const char *name);

protected:
  vtkVRMLImporter();
  ~vtkVRMLImporter();

  int OpenImportFile();
  virtual int ImportBegin();
  virtual void ImportEnd();
  virtual void ImportActors(vtkRenderer*) {}
  virtual void ImportCameras(vtkRenderer*) {}
  virtual void ImportLights(vtkRenderer*) {}
  virtual void ImportProperties(vtkRenderer*) {}

  // Description:
  // Needed by the yacc/lex grammar used
  virtual void enterNode(const char*);
  virtual void exitNode();
  virtual void enterField(const char*);
  virtual void exitField();
  virtual void useNode(const char*);

  // Description:
  // Return the file pointer to the open file.
  FILE *GetFileFD() { return this->FileFD; }

  char *FileName;
  FILE *FileFD;
  int ShapeResolution;

  friend class vtkVRMLYaccData;

private:
  vtkPoints* PointsNew();
  vtkFloatArray* FloatArrayNew();
  vtkIdTypeArray* IdTypeArrayNew();

  void DeleteObject(vtkObject*);

  vtkVRMLImporterInternal* Internal;
  vtkVRMLYaccData* Parser;
  vtkActor* CurrentActor;
  vtkProperty* CurrentProperty;
  vtkLight* CurrentLight;
  vtkTransform* CurrentTransform;
  vtkAlgorithm* CurrentSource;
  vtkPoints* CurrentPoints;
  vtkFloatArray* CurrentNormals;
  vtkCellArray* CurrentNormalCells;
  vtkFloatArray* CurrentTCoords;
  vtkCellArray* CurrentTCoordCells;
  vtkLookupTable* CurrentLut;
  vtkFloatArray* CurrentScalars;
  vtkPolyDataMapper* CurrentMapper;

private:
  vtkVRMLImporter(const vtkVRMLImporter&);  // Not implemented.
  void operator=(const vtkVRMLImporter&);  // Not implemented.
};

#endif
