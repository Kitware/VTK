/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRTAnalyticSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkImageAlgorithm.h"

class VTK_PARALLEL_EXPORT vtkRTAnalyticSource : public vtkImageAlgorithm
{
public:
  static vtkRTAnalyticSource *New();
  vtkTypeRevisionMacro(vtkRTAnalyticSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);
  vtkGetVector6Macro(WholeExtent, int);
  
  // Description:
  // Set/Get the center of function.
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);

  // Description:
  // Set/Get the Maximum value of the function.
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);

  // Description:
  // Set/Get the standard deviation of the function.
  vtkSetMacro(StandardDeviation, double);
  vtkGetMacro(StandardDeviation, double);

  // Description:
  // Set the natural frequencies in x,y and z
  vtkSetMacro(XFreq, double);
  vtkGetMacro(XFreq, double);
  vtkSetMacro(YFreq, double);
  vtkGetMacro(YFreq, double);
  vtkSetMacro(ZFreq, double);
  vtkGetMacro(ZFreq, double);

  vtkSetMacro(XMag, double);
  vtkGetMacro(XMag, double);
  vtkSetMacro(YMag, double);
  vtkGetMacro(YMag, double);
  vtkSetMacro(ZMag, double);
  vtkGetMacro(ZMag, double);

  vtkSetMacro(SubsampleRate, int);
  vtkGetMacro(SubsampleRate, int);

protected:
  vtkRTAnalyticSource();
  ~vtkRTAnalyticSource() {};

  double XFreq;
  double YFreq;
  double ZFreq;
  double XMag;
  double YMag;
  double ZMag;
  double StandardDeviation;
  int WholeExtent[6];
  double Center[3];
  double Maximum;
  int SubsampleRate;

  virtual int RequestInformation (vtkInformation *, 
                                  vtkInformationVector **, 
                                  vtkInformationVector *);
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkRTAnalyticSource(const vtkRTAnalyticSource&);  // Not implemented.
  void operator=(const vtkRTAnalyticSource&);  // Not implemented.
};


#endif
