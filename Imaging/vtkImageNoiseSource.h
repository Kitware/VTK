/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNoiseSource.h
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
// .NAME vtkImageNoiseSource - Create an image filled with noise.
// .SECTION Description
// vtkImageNoiseSource just produces images filled with noise.  The only
// option now is uniform noise specified by a min and a max.  There is one
// major problem with this source. Every time it executes, it will output
// different pixel values.  This has important implications when a stream
// requests overlapping regions.  The same pixels will have different values
// on different updates.


#ifndef __vtkImageNoiseSource_h
#define __vtkImageNoiseSource_h


#include "vtkImageSource.h"


class VTK_IMAGING_EXPORT vtkImageNoiseSource : public vtkImageSource 
{
public:
  static vtkImageNoiseSource *New();
  vtkTypeRevisionMacro(vtkImageNoiseSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the minimum and maximum values for the generated noise.
  vtkSetMacro(Minimum, float);
  vtkGetMacro(Minimum, float);
  vtkSetMacro(Maximum, float);
  vtkGetMacro(Maximum, float);

  // Description:
  // Set how large of an image to generate.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);

protected:
  vtkImageNoiseSource();
  ~vtkImageNoiseSource() {};

  float Minimum;
  float Maximum;
  int WholeExtent[6];

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkImageNoiseSource(const vtkImageNoiseSource&);  // Not implemented.
  void operator=(const vtkImageNoiseSource&);  // Not implemented.
};


#endif

  
