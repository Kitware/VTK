/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDashedStreamLine.h
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
// .NAME vtkDashedStreamLine - generate constant-time dashed streamline in arbitrary dataset
// .SECTION Description
// vtkDashedStreamLine is a filter that generates a "dashed" streamline for 
// an arbitrary dataset. The streamline consists of a series of dashes, each 
// of which represents (approximately) a constant time increment. Thus, in the
// resulting visual representation, relatively long dashes represent areas of 
// high velocity, and small dashes represent areas of low velocity.
//
// vtkDashedStreamLine introduces the instance variable DashFactor. 
// DashFactor interacts with its superclass' instance variable StepLength to
// create the dashes. DashFactor is the percentage of the StepLength line 
// segment that is visible. Thus, if the DashFactor=0.75, the dashes will be 
// "three-quarters on" and "one-quarter off".

// .SECTION See Also
// vtkStreamer vtkStreamLine vtkStreamPoints

#ifndef __vtkDashedStreamLine_h
#define __vtkDashedStreamLine_h

#include "vtkStreamLine.h"

class VTK_EXPORT vtkDashedStreamLine : public vtkStreamLine
{
public:
  static vtkDashedStreamLine *New();
  vtkTypeMacro(vtkDashedStreamLine,vtkStreamLine);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For each dash, specify the fraction of the dash that is "on". A factor
  // of 1.0 will result in a continuous line, a factor of 0.5 will result in 
  // dashed that are half on and half off.
  vtkSetClampMacro(DashFactor,float,0.01,1.0);
  vtkGetMacro(DashFactor,float);

protected:
  vtkDashedStreamLine();
  ~vtkDashedStreamLine() {};
  vtkDashedStreamLine(const vtkDashedStreamLine&);
  void operator=(const vtkDashedStreamLine&);

  // Convert streamer array into vtkPolyData
  void Execute();

  // the fraction of on versus off in dash
  float DashFactor;
  
};

#endif


