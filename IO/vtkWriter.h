/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.h
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
// .NAME vtkWriter - abstract class to write data to file(s)
// .SECTION Description
// vtkWriter is an abstract class for mapper objects that write their data
// to disk (or into a communications port). All writers respond to Write()
// method. This method insures that there is input and input is up to date.
//
// Since vtkWriter is a subclass of vtkProcessObject, StartMethod(), 
// EndMethod(), and ProgressMethod() are all available to writers.
// These methods are executed before and after execution of the Write() 
// method. You can also specify arguments to these methods.

// .SECTION Caveats
// Every subclass of vtkWriter must implement a WriteData() method. Most likely
// will have to create SetInput() method as well.

// .SECTION See Also
// vtkBYUWriter vtkDataWriter vtkSTLWriter vtkVoxelWriter vtkMCubesWriter

#ifndef __vtkWriter_h
#define __vtkWriter_h

#include "vtkProcessObject.h"
#include "vtkDataObject.h"

#define VTK_ASCII 1
#define VTK_BINARY 2

class VTK_IO_EXPORT vtkWriter : public vtkProcessObject
{
public:
  vtkTypeMacro(vtkWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Write data to output. Method executes subclasses WriteData() method, as 
  // well as StartMethod() and EndMethod() methods.
  virtual void Write();

  // Description:
  // Convenient alias for Write() method.
  void Update();
  
//BTX
  vtkDataObject *GetInput();
//ETX
protected:
  vtkWriter();
  ~vtkWriter();

  virtual void WriteData() = 0; //internal method subclasses must respond to
  vtkTimeStamp WriteTime;
private:
  vtkWriter(const vtkWriter&);  // Not implemented.
  void operator=(const vtkWriter&);  // Not implemented.
};

#endif


