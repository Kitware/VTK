/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeedConnectivity.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageSeedConnectivity - SeedConnectivity with user defined seeds.
// .SECTION Description
// vtkImageSeedConnectivity marks pixels connected to user supplied seeds.
// The input must be unsigned char, and the output is also unsigned char.  If
// a seed supplied by the user does not have pixel value "InputTrueValue",
// then the image is scanned +x, +y, +z until a pixel is encountered with
// value "InputTrueValue".  This new pixel is used as the seed .  Any pixel
// with out value "InputTrueValue" is consider off.  The output pixels values
// are 0 for any off pixel in input, "OutputTrueValue" for any pixels
// connected to seeds, and "OutputUnconnectedValue" for any on pixels not
// connected to seeds.  The same seeds are used for all images in the image
// set.

#ifndef __vtkImageSeedConnectivity_h
#define __vtkImageSeedConnectivity_h


#include "vtkImageConnector.h"
#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageSeedConnectivity : public vtkImageToImageFilter
{
public:
  static vtkImageSeedConnectivity *New();
  vtkTypeMacro(vtkImageSeedConnectivity,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Methods for manipulating the seed pixels.
  void RemoveAllSeeds();
  void AddSeed(int num, int *index);
  void AddSeed(int i0, int i1, int i2);
  void AddSeed(int i0, int i1);

  // Description:
  // Set/Get what value is considered as connecting pixels.
  vtkSetMacro(InputConnectValue, int);
  vtkGetMacro(InputConnectValue, int);

  // Description:
  // Set/Get the value to set connected pixels to.
  vtkSetMacro(OutputConnectedValue, int);
  vtkGetMacro(OutputConnectedValue, int);

  // Description:
  // Set/Get the value to set unconnected pixels to.
  vtkSetMacro(OutputUnconnectedValue, int);
  vtkGetMacro(OutputUnconnectedValue, int);
  
  // Description:
  // Get the vtkImageCOnnector used by this filter.
  vtkGetObjectMacro(Connector,vtkImageConnector);

  // Description:
  // Set the number of axes to use in connectivity.
  vtkSetMacro(Dimensionality,int);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageSeedConnectivity();
  ~vtkImageSeedConnectivity();

  unsigned char InputConnectValue;
  unsigned char OutputConnectedValue;
  unsigned char OutputUnconnectedValue;
  vtkImageConnectorSeed *Seeds;
  vtkImageConnector *Connector;
  int Dimensionality;
  
  void ComputeInputUpdateExtents(vtkDataObject *out);

  void ExecuteData(vtkDataObject *out); 
private:
  vtkImageSeedConnectivity(const vtkImageSeedConnectivity&);  // Not implemented.
  void operator=(const vtkImageSeedConnectivity&);  // Not implemented.
};



#endif


  
