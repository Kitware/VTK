/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFeatureAnd.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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


=========================================================================*/
// .NAME vtkImageFeatureAnd - Connectivity with seeds in second image.
// .SECTION Description
// vtkImageFeatureAnd implements a connectivity filter on the first input.
// Seeds are computed by taking the intersection of the 
// first image with the second image.
// Connectivity is performed on non zero pixels of the input.
// Input and output have to have scalar type unsigned char. 

#ifndef __vtkImageConnectivity_h
#define __vtkImageConnectivity_h


#include "vtkImageConnector.h"
#include "vtkImageTwoInputFilter.h"
class vtkImageRegion;


class VTK_EXPORT vtkImageFeatureAnd : public vtkImageTwoInputFilter
{
public:
  vtkImageFeatureAnd();
  ~vtkImageFeatureAnd();
  static vtkImageFeatureAnd *New() {return new vtkImageFeatureAnd;};
  const char *GetClassName() {return "vtkImageFeatureAnd";};
  
  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes, int);

  vtkSetMacro(OutputConnectedValue, int);
  vtkGetMacro(OutputConnectedValue, int);
  vtkSetMacro(OutputUnconnectedValue, int);
  vtkGetMacro(OutputUnconnectedValue, int);

  void InterceptCacheUpdate();
  
private:
  unsigned char OutputConnectedValue;
  unsigned char OutputUnconnectedValue;
  vtkImageConnector *Connector;

  void Execute(vtkImageRegion *in1Region, vtkImageRegion *in2Region,
		vtkImageRegion *outRegion);
};


#endif

  
