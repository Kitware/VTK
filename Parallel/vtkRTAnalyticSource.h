/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRTAnalyticSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRTAnalyticSource - Create an image for regression testing
// .SECTION Description
// vtkRTAnalyticSource just produces images with pixel values determined 
// by a Maximum*Gaussian*XMag*sin(XFreq*x)*sin(YFreq*y)*cos(ZFreq*z)


#ifndef __vtkRTAnalyticSource_h
#define __vtkRTAnalyticSource_h

#include "vtkImageSource.h"

class VTK_PARALLEL_EXPORT vtkRTAnalyticSource : public vtkImageSource
{
public:
  static vtkRTAnalyticSource *New();
  vtkTypeRevisionMacro(vtkRTAnalyticSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);
  vtkGetVector6Macro(WholeExtent, int);
  
  // Description:
  // Set/Get the center of function.
  vtkSetVector3Macro(Center, float);
  vtkGetVector3Macro(Center, float);

  // Description:
  // Set/Get the Maximum value of the function.
  vtkSetMacro(Maximum, float);
  vtkGetMacro(Maximum, float);

  // Description:
  // Set/Get the standard deviation of the function.
  vtkSetMacro(StandardDeviation, float);
  vtkGetMacro(StandardDeviation, float);

  // Description:
  // Set the natural frequencies in x,y and z
  vtkSetMacro(XFreq, float);
  vtkGetMacro(XFreq, float);
  vtkSetMacro(YFreq, float);
  vtkGetMacro(YFreq, float);
  vtkSetMacro(ZFreq, float);
  vtkGetMacro(ZFreq, float);

  vtkSetMacro(XMag, float);
  vtkGetMacro(XMag, float);
  vtkSetMacro(YMag, float);
  vtkGetMacro(YMag, float);
  vtkSetMacro(ZMag, float);
  vtkGetMacro(ZMag, float);

protected:
  vtkRTAnalyticSource();
  ~vtkRTAnalyticSource() {};

  float XFreq;
  float YFreq;
  float ZFreq;
  float XMag;
  float YMag;
  float ZMag;
  float StandardDeviation;
  int WholeExtent[6];
  float Center[3];
  float Maximum;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkRTAnalyticSource(const vtkRTAnalyticSource&);  // Not implemented.
  void operator=(const vtkRTAnalyticSource&);  // Not implemented.
};


#endif
