/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVRMLImporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkVRMLImporter - imports VRML 2.0 files.
// .SECTION Description
// vtkVRMLImporter imports VRML 2.0 files into vtk.
// .SECTION Caveats
// These nodes are currently supported:
//
//	Appearance				IndexedFaceSet
//	Box								IndexedLineSet
//	Color							Material
//	Cone							Shape
//	Coordinate				Sphere
//	Cylinder					Transform
//	DirectionalLight
//
// As you can see this implementation focuses on getting the geometry translated.
// The routes and scripting nodes are ignored since they deal with directly
// accessing a nodes internal structure based on the VRML spec. Since this is a
// translation the internal datastructures differ greatly from the VRML spec and
// the External Authoring Interface (see the VRML spec). The DEF/USE mechanism does
// allow the Vtk user to extract objects from the scene and directly manipulate them
// using the native language (Tcl, Python, Java, or whatever language Vtk is wrapped
// in). This, in a way, removes the need for the route and script mechanism 
// (not completely though).
// .SECTION See Also
// vtkImporter

/* ======================================================================
 
   Importer based on BNF Yacc and Lex parser definition from:

    **************************************************
	* VRML 2.0 Parser
	* Copyright (C) 1996 Silicon Graphics, Inc.
	*
	* Author(s) :	 Gavin Bell
	*                Daniel Woods (first port)
	**************************************************

  Ported to VTK By:	Thomas D. Citriniti
					Rensselaer Polytechnic Institute
					citrit@rpi.edu

=======================================================================*/



#ifndef __vtkVRMLImporter_h
#define __vtkVRMLImporter_h

// Includes for the yacc/lex parser
#include "vtkVRML.h"

#include <stdio.h>

#include "vtkImporter.h"
#include "vtkPolyDataMapper.h"
#include "vtkFloatNormals.h"
#include "vtkFloatPoints.h"

class VTK_EXPORT vtkVRMLImporter : public vtkImporter
{
public:
  vtkVRMLImporter();
  ~vtkVRMLImporter();
  static vtkVRMLImporter *New() {return new vtkVRMLImporter;};
  const char *GetClassName() {return "vtkVRMLImporter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // In the VRML spec you can DEF and USE nodes (name them),
  // This routine will return the associated VTK object which
  // was created as a result of the DEF mechanism
  // Send in the name from the VRML file, get the VTK object.
	// You will have to cast the object since this only returns 
	// vtkObjects.
  vtkObject *GetVRMLDEFObject(const char *name);

  // Description:
  // Needed by the yacc/lex grammar used
  void enterNode(const char *);
  void exitNode();
  void enterField(const char *);
  void exitField();
  void useNode(const char *);

protected:
  int ImportBegin ();
  void ImportActors (vtkRenderer *renderer) {};
  void ImportCameras (vtkRenderer *renderer) {};
  void ImportLights (vtkRenderer *renderer) {};
  void ImportProperties (vtkRenderer *renderer) {};

private:
  vtkActor					*curActor;
  vtkProperty				*curProperty;
  vtkCamera					*curCamera;
  vtkLight					*curLight;
  vtkTransform			*curTransform;
  vtkSource					*curSource;
  vtkFloatPoints		*curPoints;
  vtkFloatNormals		*curNormals;
  vtkLookupTable		*curLut;
  vtkScalars				*curScalars;
  vtkPolyDataMapper *curMapper;
};

#endif

