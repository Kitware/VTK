/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtent.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkExtent - Generalizes imaging extents into graphics.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// vtkExtent contains information to specify update extents of vtkDataSets.
// This is a superclass. Two subclasses exist.  One for structured data,
// and one for unstructured data.  The extent object will also indicate
// whether the request is for points or cells.

#ifndef __vtkExtent_h
#define __vtkExtent_h

#include "vtkObject.h"
class vtkStructuredExtent;
class vtkUnstructuredExtent;


class VTK_EXPORT vtkExtent : public vtkObject
{
public:
  static vtkExtent *New();

  vtkTypeMacro(vtkExtent,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy information from one extent into another.
  // Subclasses override this method, and try to be smart
  // if the types are different.
  virtual void Copy(vtkExtent *in);


  // Description:
  // We can use streaming to processes series of data sets one at a time.
  vtkSetMacro(SeriesIndex, int);
  vtkGetMacro(SeriesIndex, int);
  
  // Description:
  // Serialization provided for the multi-process ports.
  virtual void ReadSelf(istream& is);
  virtual void WriteSelf(ostream& os);

protected:
  vtkExtent();
  ~vtkExtent() {};
  vtkExtent(const vtkExtent&) {};
  void operator=(const vtkExtent&) {};

  int SeriesIndex;
};


#endif
