/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridSource.h
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
// .NAME vtkRectilinearGridSource - Abstract class whose subclasses generates rectilinear grid data
// .SECTION Description
// vtkRectilinearGridSource is an abstract class whose subclasses generate
// rectilinear grid data.

// .SECTION See Also
// vtkRectilinearGridReader

#ifndef __vtkRectilinearGridSource_h
#define __vtkRectilinearGridSource_h

#include "vtkSource.h"
#include "vtkRectilinearGrid.h"

class VTK_EXPORT vtkRectilinearGridSource : public vtkSource
{
public:
  static vtkRectilinearGridSource *New();
  vtkTypeMacro(vtkRectilinearGridSource,vtkSource);

  // Description:
  // Get the output of this source.
  vtkRectilinearGrid *GetOutput();
  vtkRectilinearGrid *GetOutput(int idx)
    {return (vtkRectilinearGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkRectilinearGrid *output);

protected:
  vtkRectilinearGridSource();
  ~vtkRectilinearGridSource() {};
  vtkRectilinearGridSource(const vtkRectilinearGridSource&);
  void operator=(const vtkRectilinearGridSource&);

  // Used by streaming: The extent of the output being processed
  // by the execute method. Set in the ComputeInputUpdateExtent method.
  int ExecuteExtent[6];
};

#endif


