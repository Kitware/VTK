/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImporter
 * @brief   importer abstract class
 *
 * vtkImporter is an abstract class that specifies the protocol for
 * importing actors, cameras, lights and properties into a
 * vtkRenderWindow. The following takes place:
 * 1) Create a RenderWindow and Renderer if none is provided.
 * 2) Call ImportBegin, if ImportBegin returns False, return
 * 3) Call ReadData, which calls:
 *  a) Import the Actors
 *  b) Import the cameras
 *  c) Import the lights
 *  d) Import the Properties
 * 7) Call ImportEnd
 *
 * Subclasses optionally implement the ImportActors, ImportCameras,
 * ImportLights and ImportProperties or ReadData methods. An ImportBegin and
 * ImportEnd can optionally be provided to perform Importer-specific
 * initialization and termination.  The Read method initiates the import
 * process. If a RenderWindow is provided, its Renderer will contained
 * the imported objects. If the RenderWindow has no Renderer, one is
 * created. If no RenderWindow is provided, both a RenderWindow and
 * Renderer will be created. Both the RenderWindow and Renderer can be
 * accessed using Get methods.
 *
 * @sa
 * vtk3DSImporter vtkExporter
*/

#ifndef vtkImporter_h
#define vtkImporter_h

#include "vtkIOImportModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderWindow;
class vtkRenderer;

class VTKIOIMPORT_EXPORT vtkImporter : public vtkObject
{
public:
  vtkTypeMacro(vtkImporter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;


  //@{
  /**
   * Get the renderer that contains the imported actors, cameras and
   * lights.
   */
  vtkGetObjectMacro(Renderer,vtkRenderer);
  //@}

  //@{
  /**
   * Set the vtkRenderWindow to contain the imported actors, cameras and
   * lights, If no vtkRenderWindow is set, one will be created and can be
   * obtained with the GetRenderWindow method. If the vtkRenderWindow has been
   * specified, the first vtkRenderer it has will be used to import the
   * objects. If the vtkRenderWindow has no Renderer, one will be created and
   * can be accessed using GetRenderer.
   */
  virtual void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);
  //@}


  //@{
  /**
   * Import the actors, cameras, lights and properties into a vtkRenderWindow.
   */
  void Read();
  void Update() {this->Read();};
  //@}


protected:
  vtkImporter();
  ~vtkImporter() override;

  virtual int ImportBegin () {return 1;};
  virtual void ImportEnd () {}
  virtual void ImportActors (vtkRenderer*) {}
  virtual void ImportCameras (vtkRenderer*) {}
  virtual void ImportLights (vtkRenderer*) {}
  virtual void ImportProperties (vtkRenderer*) {}

  vtkRenderer *Renderer;
  vtkRenderWindow *RenderWindow;

  virtual void ReadData();

private:
  vtkImporter(const vtkImporter&) = delete;
  void operator=(const vtkImporter&) = delete;
};

#endif




