/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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

#include "vtkObject.h"
#include "vtkRenderWindow.h"

class VTK_RENDERING_EXPORT vtkExporter : public vtkObject 
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
  vtkSetObjectMacro(RenderWindow,vtkRenderWindow);
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


