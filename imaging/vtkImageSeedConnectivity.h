/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeedConnectivity.h
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
// .NAME vtkImageSeedConnectivity - SeedConnectivity with user defined seeds.
// .SECTION Description
// vtkImageSeedConnectivity marks pixels connected to user supplied seeds.  The input
// must be unsigned char, and the output is also unsigned char.  If a seed supplied by
// the user does not have pixel value "InputTrueValue", then the image is 
// scaned +x, +y, +z until a pixel is encountered with value "InputTrueValue".
// This new pixel is used as the seed .  Any pixel with out value "InputTrueValue" 
// is consider off.  The output pixels values are 0 for any off pixel in input, 
// "OutputTrueValue" for any pixels connected to seeds, and "OutputUnconnectedValue"
// for any on pixels not connected to seeds.  The same seeds are used for all images in the
// image set (images along unfiltered axes).  Index values for
// unfiltered axes are ignored.


#ifndef __vtkImageSeedConnectivity_h
#define __vtkImageSeedConnectivity_h


#include "vtkImageConnector.h"
#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageSeedConnectivity : public vtkImageFilter
{
public:
  vtkImageSeedConnectivity();
  ~vtkImageSeedConnectivity();
  static vtkImageSeedConnectivity *New() 
    {return new vtkImageSeedConnectivity;};
  const char *GetClassName() {return "vtkImageSeedConnectivity";};
  
  void RemoveAllSeeds();
  void AddSeed(int num, int *index);
  void AddSeed(int i0, int i1, int i2);
  void AddSeed(int i0, int i1);

  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes, int);

  vtkSetMacro(InputConnectValue, int);
  vtkGetMacro(InputConnectValue, int);
  vtkSetMacro(OutputConnectedValue, int);
  vtkGetMacro(OutputConnectedValue, int);
  vtkSetMacro(OutputUnconnectedValue, int);
  vtkGetMacro(OutputUnconnectedValue, int);
  
  void InterceptCacheUpdate();

  vtkGetObjectMacro(Connector,vtkImageConnector);
  
private:
  unsigned char InputConnectValue;
  unsigned char OutputConnectedValue;
  unsigned char OutputUnconnectedValue;
  vtkImageConnectorSeed *Seeds;
  vtkImageConnector *Connector;
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};



#endif


  
