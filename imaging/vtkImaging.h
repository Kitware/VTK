/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImaging.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkAxisActor2D.h"
#include "vtkBMPReader.h"
#include "vtkBMPWriter.h"
#include "vtkImageAccumulate.h"
#include "vtkImageAnisotropicDiffusion2D.h"
#include "vtkImageAnisotropicDiffusion3D.h"
#include "vtkImageAppend.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageButterworthHighPass.h"
#include "vtkImageButterworthLowPass.h"
#include "vtkImageCanvasSource2D.h"
#include "vtkImageCast.h"
#include "vtkImageCityBlockDistance.h"
#include "vtkImageClip.h"
#include "vtkImageConnector.h"
#include "vtkImageConstantPad.h"
#include "vtkImageContinuousDilate3D.h"
#include "vtkImageContinuousErode3D.h"
#include "vtkImageCorrelation.h"
#include "vtkImageCursor3D.h"
#include "vtkImageDecomposeFilter.h"
#include "vtkImageDifference.h"
#include "vtkImageDilateErode3D.h"
#include "vtkImageDivergence.h"
#include "vtkImageDotProduct.h"
#include "vtkImageEllipsoidSource.h"
#include "vtkImageEuclideanToPolar.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageFFT.h"
#include "vtkImageToImageFilter.h"
#include "vtkImageFlip.h"
#include "vtkImageFourierCenter.h"
#include "vtkImageFourierFilter.h"
#include "vtkImageGaussianSmooth.h"
#include "vtkImageGaussianSource.h"
#include "vtkImageGradient.h"
#include "vtkImageGradientMagnitude.h"
#include "vtkImageHSVToRGB.h"
#include "vtkImageHybridMedian2D.h"
#include "vtkImageIdealHighPass.h"
#include "vtkImageIdealLowPass.h"
#include "vtkImageInPlaceFilter.h"
#include "vtkImageIslandRemoval2D.h"
#include "vtkImageIterateFilter.h"
#include "vtkImageLaplacian.h"
#include "vtkImageLogarithmicScale.h"
#include "vtkImageLogic.h"
#include "vtkImageLuminance.h"
#include "vtkImageMagnify.h"
#include "vtkImageMagnitude.h"
#include "vtkImageMapper.h"
#include "vtkImageMask.h"
#include "vtkImageMaskBits.h"
#include "vtkImageMathematics.h"
#include "vtkImageMedian3D.h"
#include "vtkImageMirrorPad.h"
#include "vtkImageMultipleInputFilter.h"
#include "vtkImageNoiseSource.h"
#include "vtkImageNonMaximumSuppression.h"
#include "vtkImageOpenClose3D.h"
#include "vtkImagePadFilter.h"
#include "vtkImagePermute.h"
#include "vtkImageRFFT.h"
#include "vtkImageRGBToHSV.h"
#include "vtkImageRange3D.h"
#include "vtkImageReader.h"
#include "vtkImageResample.h"
#include "vtkImageSeedConnectivity.h"
#include "vtkImageShiftScale.h"
#include "vtkImageShrink3D.h"
#include "vtkImageSinusoidSource.h"
#include "vtkImageSkeleton2D.h"
#include "vtkImageSobel2D.h"
#include "vtkImageSobel3D.h"
#include "vtkImageSpatialFilter.h"
#include "vtkImageThreshold.h"
#include "vtkImageTranslateExtent.h"
#include "vtkImageTwoInputFilter.h"
#include "vtkImageVariance3D.h"
#include "vtkImageViewer.h"
#include "vtkImageWindow.h"
#include "vtkImageWrapPad.h"
#include "vtkImageWriter.h"
#include "vtkImager.h"
#include "vtkImagerCollection.h"
#include "vtkLabeledDataMapper.h"
#include "vtkPNMReader.h"
#include "vtkPNMWriter.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkScalarBarActor.h"
#include "vtkTIFFReader.h"
#include "vtkTIFFWriter.h"
#include "vtkTextMapper.h"
#include "vtkXYPlotActor.h"
