/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLImporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tom Citriniti who implemented and contributed this class


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
#include "vtkVRML.h"

#include "vtkImporter.h"
class  vtkActor;
class  vtkProperty;
class  vtkCamera;
class  vtkLight;
class  vtkTransform;
class  vtkSource;
class  vtkLookupTable;
class  vtkScalars;
class  vtkPolyDataMapper;
class vtkNormals;
class vtkPoints;

class VTK_EXPORT vtkVRMLImporter : public vtkImporter
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
  vtkVRMLImporter(const vtkVRMLImporter&) {};
  void operator=(const vtkVRMLImporter&) {};

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
  vtkSource            *CurrentSource;
  vtkPoints            *CurrentPoints;
  vtkNormals           *CurrentNormals;
  vtkLookupTable       *CurrentLut;
  vtkScalars           *CurrentScalars;
  vtkPolyDataMapper    *CurrentMapper;

  vtkPoints* PointsNew();
  vtkIntArray* IntArrayNew();

  void DeleteObject(vtkObject*);

//BTX

#ifdef WIN32
#pragma warning( disable : 4251 )
#endif

  VectorType<vtkObject*> Heap;

#ifdef WIN32
#pragma warning( default : 4251 )
#endif

//ETX

};

#endif

