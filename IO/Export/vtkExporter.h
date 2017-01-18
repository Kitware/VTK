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
/**
 * @class   vtkExporter
 * @brief   abstract class to write a scene to a file
 *
 * vtkExporter is an abstract class that exports a scene to a file. It
 * is very similar to vtkWriter except that a writer only writes out
 * the geometric and topological data for an object, where an exporter
 * can write out material properties, lighting, camera parameters etc.
 * The concrete subclasses of this class may not write out all of this
 * information. For example vtkOBJExporter writes out Wavefront obj files
 * which do not include support for camera parameters.
 *
 * vtkExporter provides the convenience methods StartWrite() and EndWrite().
 * These methods are executed before and after execution of the Write()
 * method. You can also specify arguments to these methods.
 * This class defines SetInput and GetInput methods which take or return
 * a vtkRenderWindow.
 * @warning
 * Every subclass of vtkExporter must implement a WriteData() method.
 *
 * @sa
 * vtkOBJExporter vtkRenderWindow vtkWriter
*/

#ifndef vtkExporter_h
#define vtkExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkObject.h"
class vtkRenderWindow;

class VTKIOEXPORT_EXPORT vtkExporter : public vtkObject
{
public:
  vtkTypeMacro(vtkExporter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Write data to output. Method executes subclasses WriteData() method, as
   * well as StartWrite() and EndWrite() methods.
   */
  virtual void Write();

  /**
   * Convenient alias for Write() method.
   */
  void Update();

  //@{
  /**
   * Set/Get the rendering window that contains the scene to be written.
   */
  virtual void SetRenderWindow(vtkRenderWindow*);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);
  //@}

  //@{
  /**
   * These methods are provided for backward compatibility. Will disappear
   * soon.
   */
  void SetInput(vtkRenderWindow *renWin) {this->SetRenderWindow(renWin);};
  vtkRenderWindow *GetInput() {return this->GetRenderWindow();};
  //@}

  /**
   * Specify a function to be called before data is written.  Function will
   * be called with argument provided.
   */
  void SetStartWrite(void (*f)(void *), void *arg);

  /**
   * Specify a function to be called after data is written.
   * Function will be called with argument provided.
   */
  void SetEndWrite(void (*f)(void *), void *arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetStartWriteArgDelete(void (*f)(void *));

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetEndWriteArgDelete(void (*f)(void *));

  /**
   * Returns the MTime also considering the RenderWindow.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkExporter();
  ~vtkExporter() VTK_OVERRIDE;

  vtkRenderWindow *RenderWindow;
  virtual void WriteData() = 0;

  void (*StartWrite)(void *);
  void (*StartWriteArgDelete)(void *);
  void *StartWriteArg;
  void (*EndWrite)(void *);
  void (*EndWriteArgDelete)(void *);
  void *EndWriteArg;
private:
  vtkExporter(const vtkExporter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExporter&) VTK_DELETE_FUNCTION;
};

#endif


