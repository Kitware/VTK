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
// vtkVRMLImporter imports VRML 2.0 files into vtk.
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
// VRML spec). The DEF/USE mechanism does allow the Vtk user to extract
// objects from the scene and directly manipulate them using the native
// language (Tcl, Python, Java, or whatever language Vtk is wrapped
// in). This, in a way, removes the need for the route and script mechanism
// (not completely though).
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

#ifndef __vtkVRMLImporter_h
#define __vtkVRMLImporter_h

// Includes for the yacc/lex parser
#include "vtkImporter.h"

class vtkActor;
class vtkAlgorithm;
class vtkProperty;
class vtkCamera;
class vtkLight;
class vtkTransform;
class vtkLookupTable;
class vtkFloatArray;
class vtkPolyDataMapper;
class vtkPoints;
class vtkIdTypeArray;
class vtkVRMLImporterInternal;
class vtkCellArray;

class VTK_HYBRID_EXPORT vtkVRMLImporter : public vtkImporter
{
public:
  static vtkVRMLImporter *New();

  vtkTypeMacro(vtkVRMLImporter,vtkImporter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // In the VRML spec you can DEF and USE nodes (name them),
  // This routine will return the associated VTK object which
  // was created as a result of the DEF mechanism
  // Send in the name from the VRML file, get the VTK object.
  // You will have to check and correctly cast the object since
  // this only returns vtkObjects.
  vtkObject *GetVRMLDEFObject(const char *name);

  // Description:
  // Needed by the yacc/lex grammar used
  void enterNode(const char *);
  void exitNode();
  void enterField(const char *);
  void exitField();
  void useNode(const char *);

  // Description:
  // Specify the name of the file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Return the file pointer to the open file.
  FILE *GetFileFD() {return this->FileFD;};

//BTX

  friend int yylex ( vtkVRMLImporter* );

//ETX

protected:
  vtkVRMLImporter();
  ~vtkVRMLImporter();

  virtual int ImportBegin ();
  virtual void ImportEnd ();
  virtual void ImportActors (vtkRenderer *) {};
  virtual void ImportCameras (vtkRenderer *) {};
  virtual void ImportLights (vtkRenderer *) {};
  virtual void ImportProperties (vtkRenderer *) {};

  int OpenImportFile();
  char *FileName;
  FILE *FileFD;

private:
  vtkActor             *CurrentActor;
  vtkProperty          *CurrentProperty;
  vtkCamera            *CurrentCamera;
  vtkLight             *CurrentLight;
  vtkTransform         *CurrentTransform;
  vtkAlgorithm         *CurrentSource;
  vtkPoints            *CurrentPoints;
  vtkFloatArray        *CurrentNormals;
  vtkCellArray         *CurrentNormalCells;
  vtkFloatArray        *CurrentTCoords;
  vtkCellArray         *CurrentTCoordCells;
  vtkLookupTable       *CurrentLut;
  vtkFloatArray        *CurrentScalars;
  vtkPolyDataMapper    *CurrentMapper;

  vtkPoints* PointsNew();
  vtkFloatArray* FloatArrayNew();
  vtkIdTypeArray* IdTypeArrayNew();

  void DeleteObject(vtkObject*);

  vtkVRMLImporterInternal* Internal;

private:
  vtkVRMLImporter(const vtkVRMLImporter&);  // Not implemented.
  void operator=(const vtkVRMLImporter&);  // Not implemented.
};

#endif

