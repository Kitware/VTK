/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.h
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
// .NAME vtkThreshold - extracts cells where scalar value of every point in cell satisfies threshold criterion
// .SECTION Description
// vtkThreshold is a filter that extracts cells from any dataset type that
// satisfy a threshold criterion. A cell satisfies the criterion if the
// scalar value of (every or any) point satisfies the criterion. The
// criterion can take three forms: 1) greater than a particular value; 2)
// less than a particular value; or 3) between two values. The output of this
// filter is an unstructured grid.
// .SECTION See Also
// vtkThresholdPoints vtkThresholdTextureCoords

#ifndef __vtkThreshold_h
#define __vtkThreshold_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class VTK_EXPORT vtkThreshold : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkThreshold();
  vtkThreshold *New() {return new vtkThreshold;};
  char *GetClassName() {return "vtkThreshold";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ThresholdByLower(float lower);
  void ThresholdByUpper(float upper);
  void ThresholdBetween(float lower, float upper);
  
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  // Description:
  // The test must be satisfied for all scalars of the cell
  // or just one.
  vtkSetMacro(AllScalars,int);
  vtkGetMacro(AllScalars,int);
  vtkBooleanMacro(AllScalars,int);
  
protected:
  // Usual data generation method
  void Execute();

  int   AllScalars;
  float LowerThreshold;
  float UpperThreshold;

  //BTX
  int (vtkThreshold::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};
};

#endif


