/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi and Sebastien Barre who developed this class.

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
// .NAME vtkImageBlend - blend images together using alpha or opacity
// .SECTION Description
// vtkImageBlend takes L, LA, RGB, or RGBA images as input and blends them 
// according to the alpha values and/or the opacity setting for each input.
// Different blending modes are available:
//
// \em Normal (default) : 
// The blending rules are very similar to those for VTK texture maps.
// The alpha value of the first input, if present, is copied to the alpha 
// value of the output.  The output always has the same number of components
// and the same extent as the first input.
//
// \code
// output <- input[0]
// foreach input i {
//   foreach pixel px {
//     r <- input[i](px)(alpha) * opacity[i]
//     f <- (255 - r)
//     output(px) <- output(px) * f + input(px) * r
//   }
// }
// \endcode
//
// \em Compound : 
// Images are compounded together and each component is scaled by the sum of
// the alpha/opacity values. Use the CompoundThreshold method to set 
// specify a threshold in compound mode. Pixels with opacity*alpha less
// or equal than this threshold are ignored.
// The alpha value of the first input, if present, is NOT copied to the alpha 
// value of the output.  The output always has the same number of components
// and the same extent as the first input.
//
// \code
// output <- 0
// foreach pixel px {
//   sum <- 0
//   foreach input i {
//     r <- input[i](px)(alpha) * opacity(i)
//     sum <- sum + r
//     if r > threshold {
//       output(px) <- output(px) + input(px) * r
//     }
//   }
//   output(px) <- output(px) / sum
// }
// \endcode

#ifndef __vtkImageBlend_h
#define __vtkImageBlend_h


#include "vtkImageMultipleInputFilter.h"
#include "vtkImageStencilData.h"

#define VTK_IMAGE_BLEND_MODE_NORMAL    0
#define VTK_IMAGE_BLEND_MODE_COMPOUND 1

class VTK_IMAGING_EXPORT vtkImageBlend : public vtkImageMultipleInputFilter
{
public:
  static vtkImageBlend *New();
  vtkTypeMacro(vtkImageBlend,vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the opacity of an input image: the alpha values of the image are
  // multiplied by the opacity.  The opacity of image idx=0 is ignored.
  void SetOpacity(int idx, double opacity);
  double GetOpacity(int idx);

  // Description:
  // Set a stencil to apply when blending the data.
  vtkSetObjectMacro(Stencil, vtkImageStencilData);
  vtkGetObjectMacro(Stencil, vtkImageStencilData);

  // Description:
  // Set the blend mode
  vtkSetClampMacro(BlendMode,int,
                   VTK_IMAGE_BLEND_MODE_NORMAL, 
                   VTK_IMAGE_BLEND_MODE_COMPOUND );
  vtkGetMacro(BlendMode,int);
  void SetBlendModeToNormal() 
	{this->SetBlendMode(VTK_IMAGE_BLEND_MODE_NORMAL);};
  void SetBlendModeToCompound() 
	{this->SetBlendMode(VTK_IMAGE_BLEND_MODE_COMPOUND);};
  const char *GetBlendModeAsString(void);

  // Description:
  // Specify a threshold in compound mode. Pixels with opacity*alpha less
  // or equal the threshold are ignored.
  vtkSetMacro(CompoundThreshold,float);
  vtkGetMacro(CompoundThreshold,float);

protected:
  vtkImageBlend();
  ~vtkImageBlend();

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
				int whichInput);

  void ExecuteInformation() {
    this->vtkImageMultipleInputFilter::ExecuteInformation(); };

  void ExecuteInformation(vtkImageData **, vtkImageData *);

  void ThreadedExecute(vtkImageData **inDatas, 
                       vtkImageData *outData,
		       int extent[6], 
                       int id);

  void ExecuteData(vtkDataObject *output);
  
  vtkImageStencilData *Stencil;
  double *Opacity;
  int OpacityArrayLength;
  int BlendMode;
  float CompoundThreshold;
  int DataWasPassed;  
private:
  vtkImageBlend(const vtkImageBlend&);  // Not implemented.
  void operator=(const vtkImageBlend&);  // Not implemented.
};

// Description:
// Get the blending mode as a descriptive string
inline const char *vtkImageBlend::GetBlendModeAsString()
{
  switch (this->BlendMode)
    {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      return "Normal";
    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      return "Compound";
    default:
      return "Unknown Blend Mode";
    }
}


#endif




