/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExporter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

class VTK_EXPORT vtkExporter : public vtkObject 
{
public:
  vtkExporter();
  char *GetClassName() {return "vtkExporter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Write();
  void Update();

  // Description:
  // Set/Get the rendering window that contains the scene to be written.
  vtkSetObjectMacro(Input,vtkRenderWindow);
  vtkGetObjectMacro(Input,vtkRenderWindow);
  
  void SetStartWrite(void (*f)(void *), void *arg);
  void SetEndWrite(void (*f)(void *), void *arg);
  void SetStartWriteArgDelete(void (*f)(void *));
  void SetEndWriteArgDelete(void (*f)(void *));

protected:
  vtkRenderWindow *Input;
  virtual void WriteData() = 0;

  void (*StartWrite)(void *);
  void (*StartWriteArgDelete)(void *);
  void *StartWriteArg;
  void (*EndWrite)(void *);
  void (*EndWriteArgDelete)(void *);
  void *EndWriteArg;
};

#endif


