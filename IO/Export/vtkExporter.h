/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExporter - abstract class to write a scene to a file
// .SECTION Description
// vtkExporter is an abstract class that exports a scene to a file. It
// is very similar to vtkWriter except that a writer only writes out
// the geometric and topological data for an object, where an exporter
// can write out material properties, lighting, camera parameters etc.
// The concrete subclasses of this class may not write out all of this
// information. For example vtkOBJExporter writes out Wavefront obj files
// which do not include support for camera parameters.
//
// vtkExporter provides the convenience methods StartWrite() and EndWrite().
// These methods are executed before and after execution of the Write()
// method. You can also specify arguments to these methods.
// This class defines SetInput and GetInput methods which take or return
// a vtkRenderWindow.
// .SECTION Caveats
// Every subclass of vtkExporter must implement a WriteData() method.

// .SECTION See Also
// vtkOBJExporter vtkRenderWindow vtkWriter

#ifndef __vtkExporter_h
#define __vtkExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkObject.h"
class vtkRenderWindow;

class VTKIOEXPORT_EXPORT vtkExporter : public vtkObject
{
public:
  vtkTypeMacro(vtkExporter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Write data to output. Method executes subclasses WriteData() method, as
  // well as StartWrite() and EndWrite() methods.
  virtual void Write();

  // Description:
  // Convenient alias for Write() method.
  void Update();

  // Description:
  // Set/Get the rendering window that contains the scene to be written.
  virtual void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // These methods are provided for backward compatibility. Will disappear
  // soon.
  void SetInput(vtkRenderWindow *renWin) {this->SetRenderWindow(renWin);};
  vtkRenderWindow *GetInput() {return this->GetRenderWindow();};

  // Description:
  // Specify a function to be called before data is written.  Function will
  // be called with argument provided.
  void SetStartWrite(void (*f)(void *), void *arg);

  // Description:
  // Specify a function to be called after data is written.
  // Function will be called with argument provided.
  void SetEndWrite(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetStartWriteArgDelete(void (*f)(void *));

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetEndWriteArgDelete(void (*f)(void *));

  // Description:
  // Returns the MTime also considering the RenderWindow.
  unsigned long GetMTime();

protected:
  vtkExporter();
  ~vtkExporter();

  vtkRenderWindow *RenderWindow;
  virtual void WriteData() = 0;

  void (*StartWrite)(void *);
  void (*StartWriteArgDelete)(void *);
  void *StartWriteArg;
  void (*EndWrite)(void *);
  void (*EndWriteArgDelete)(void *);
  void *EndWriteArg;
private:
  vtkExporter(const vtkExporter&);  // Not implemented.
  void operator=(const vtkExporter&);  // Not implemented.
};

#endif


