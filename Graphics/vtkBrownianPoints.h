/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBrownianPoints.h
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
// .NAME vtkBrownianPoints - assign random vector to points
// .SECTION Description
// vtkBrownianPoints is a filter object that assigns a random vector (i.e.,
// magnitude and direction) to each point. The minimum and maximum speed
// values can be controlled by the user.

#ifndef __vtkBrownianPoints_h
#define __vtkBrownianPoints_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkBrownianPoints : public vtkDataSetToDataSetFilter
{
public:
  // Description:
  // Create instance with minimum speed 0.0, maximum speed 1.0.
  static vtkBrownianPoints *New();

  vtkTypeMacro(vtkBrownianPoints,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the minimum speed value.
  vtkSetClampMacro(MinimumSpeed,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MinimumSpeed,float);

  // Description:
  // Set the maximum speed value.
  vtkSetClampMacro(MaximumSpeed,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumSpeed,float);

protected:
  vtkBrownianPoints();
  ~vtkBrownianPoints() {};
  vtkBrownianPoints(const vtkBrownianPoints&);
  void operator=(const vtkBrownianPoints&);

  void Execute();
  float MinimumSpeed;
  float MaximumSpeed;
};

#endif


