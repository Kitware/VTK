/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredExtent.h
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
// .NAME vtkStructuredExtent - Min/Max for each of the three axes.
// .SECTION Description
// Note:  This object is under development an might change in the future.
// vtkStructuredExtent contains information to specify update extents of 
// structured data sets like vtkImageData.

#ifndef __vtkStructuredExtent_h
#define __vtkStructuredExtent_h

#include "vtkExtent.h"

class VTK_EXPORT vtkStructuredExtent : public vtkExtent
{
public:
  static vtkStructuredExtent *New();

  vtkTypeMacro(vtkStructuredExtent,vtkExtent);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy information from one extent into another.
  // This tries to be smart if the types are different.
  void Copy(vtkExtent *in);

  // Description:
  // Access to the extent.  Note: I do not like the name "Extent" for this ivar
  // and will probably change it in the future.
  vtkSetVector6Macro(Extent, int);
  vtkGetVector6Macro(Extent, int);
  
  // Description:
  // Serialization provided for the multi-process ports.
  void ReadSelf(istream& is);
  void WriteSelf(ostream& os);

protected:
  vtkStructuredExtent();
  ~vtkStructuredExtent() {};
  vtkStructuredExtent(const vtkStructuredExtent&) {};
  void operator=(const vtkStructuredExtent&) {};

  // This is the way the extent was specified before these objects.
  int Extent[6];
};


#endif
