/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThresholdPoints.h
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
// .NAME vtkThresholdPoints - extracts points whose scalar value satisfies threshold criterion
// .SECTION Description
// vtkThresholdPoints is a filter that extracts points from a dataset that 
// satisfy a threshold criterion. The criterion can take three forms:
// 1) greater than a particular value; 2) less than a particular value; or
// 3) between a particular value. The output of the filter is polygonal data.
// .SECTION See Also
// vtkThreshold

#ifndef __vtkThresholdPoints_h
#define __vtkThresholdPoints_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkThresholdPoints : public vtkDataSetToPolyDataFilter
{
public:
  static vtkThresholdPoints *New();
  vtkTypeMacro(vtkThresholdPoints,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Criterion is cells whose scalars are less than lower threshold.
  void ThresholdByLower(float lower);

  // Description:
  // Criterion is cells whose scalars are less than upper threshold.
  void ThresholdByUpper(float upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds.
  void ThresholdBetween(float lower, float upper);

  // Description:
  // Get the upper and lower thresholds.
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

protected:
  vtkThresholdPoints();
  ~vtkThresholdPoints() {};
  vtkThresholdPoints(const vtkThresholdPoints&) {};
  void operator=(const vtkThresholdPoints&) {};

  // Usual data generation method
  void Execute();

  float LowerThreshold;
  float UpperThreshold;

  //BTX
  int (vtkThresholdPoints::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif
