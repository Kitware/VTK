/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProbeF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.


=========================================================================*/
// .NAME vtkProbeFilter - compute data values at specified point locations
// .SECTION Description
// vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at point positions in the input. The point positions
// are obtained from the points in the source object.

#ifndef __vtkProbeFilter_h
#define __vtkProbeFilter_h

#include "DS2DSF.hh"

class vtkProbeFilter : public vtkDataSetToDataSetFilter
{
public:
  vtkProbeFilter();
  ~vtkProbeFilter() {};
  char *GetClassName() {return "vtkProbeFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Update();
  void Initialize();

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  vtkSetObjectMacro(Source,vtkDataSet);
  vtkGetObjectMacro(Source,vtkDataSet);

protected:
  void Execute();
  vtkDataSet *Source;

};

#endif


