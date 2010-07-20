/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOcclusionSpectrum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageOcclusionSpectrum - Computes the occlusion spectrum of a volume.
// .SECTION Description
// vtkImageOcclusionSpectrum computes the occlusion spectrum of an image as
// introduced in the paper.
//
//   The occlusion spectrum for volume classification and visualization
//   in IEEE Trans. Vis. Comput. Graph., Vol. 15, Nr. 6(2009), p. 1465-1472
//   by Carlos D. Correa and Kwan-Liu Ma
//   http://dx.doi.org/10.1109/TVCG.2009.189

#ifndef __vtkImageOcclusionSpectrum_h
#define __vtkImageOcclusionSpectrum_h

#include "vtkThreadedImageAlgorithm.h"

class VTK_IMAGING_EXPORT vtkImageOcclusionSpectrum
: public vtkThreadedImageAlgorithm
{
public :
  static vtkImageOcclusionSpectrum* New ();
  vtkTypeMacro(vtkImageOcclusionSpectrum, vtkThreadedImageAlgorithm);
  void PrintSelf (ostream&, vtkIndent);

protected :
 ~vtkImageOcclusionSpectrum () {};
  vtkImageOcclusionSpectrum ();

  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  // virtual int RequestData(vtkInformation*,
  //                         vtkInformationVector**,
  //                         vtkInformationVector*);
  void ThreadedRequestData(vtkInformation*,
                           vtkInformationVector**,
                           vtkInformationVector*,
                           vtkImageData*** inData,
                           vtkImageData** outData,
                           int outExt [6],
                           int threadId);

public :
  int Radius;

private :
  vtkImageOcclusionSpectrum (const vtkImageOcclusionSpectrum&);
  void operator = (const vtkImageOcclusionSpectrum&);
};

#endif
