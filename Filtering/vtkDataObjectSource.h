/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectSource.h
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
// .NAME vtkDataObjectSource - abstract class specifies interface for
//  field source (or objects that generate field output)

// .SECTION Description
// vtkDataObjectSource is an abstract object that specifies behavior and
// interface of field source objects. Field source objects are source objects
// that create vtkFieldData (field data) on output.
//
// Concrete subclasses of vtkDataObjectSource must define Update() and
// Execute() methods. The public method Update() invokes network execution
// and will bring the network up-to-date. The protected Execute() method
// actually does the work of data creation/generation. The difference between
// the two methods is that Update() implements input consistency checks and
// modified time comparisons and then invokes the Execute() which is an
// implementation of a particular algorithm.
//
// vtkDataObjectSource provides a mechanism for invoking the methods
// StartMethod() and EndMethod() before and after object execution (via
// Execute()). These are convenience methods you can use for any purpose
// (e.g., debugging info, highlighting/notifying user interface, etc.) These
// methods accept a single void* pointer that can be used to send data to the
// methods. It is also possible to specify a function to delete the argument
// via StartMethodArgDelete and EndMethodArgDelete.
//
// Another method, ProgressMethod() can be specified. Some filters invoke this 
// method periodically during their execution. The use is similar to that of 
// StartMethod() and EndMethod().
//
// An important feature of subclasses of vtkDataObjectSource is that it is
// possible to control the memory-management model (i.e., retain output
// versus delete output data). If enabled the ReleaseDataFlag enables the
// deletion of the output data once the downstream process object finishes
// processing the data (please see text).

// .SECTION See Also
// vtkSource vtkFilter vtkFieldDataFilter

#ifndef __vtkDataObjectSource_h
#define __vtkDataObjectSource_h

#include "vtkSource.h"

class vtkDataObject;

class VTK_FILTERING_EXPORT vtkDataObjectSource : public vtkSource
{
public:
  static vtkDataObjectSource *New();
  vtkTypeMacro(vtkDataObjectSource,vtkSource);

  // Description:
  // Get the output field of this source.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx)
    {return (vtkDataObject *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkDataObject *);
  
protected:
  vtkDataObjectSource();
  ~vtkDataObjectSource() {};

private:
  vtkDataObjectSource(const vtkDataObjectSource&);  // Not implemented.
  void operator=(const vtkDataObjectSource&);  // Not implemented.
};

#endif

