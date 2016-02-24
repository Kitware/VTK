/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkResampleToImage - sample dataset on a uniform grid
// .SECTION Description
// vtkPResampleToImage is a filter that resamples the input dataset on
// a uniform grid. It internally uses vtkProbeFilter to do the probing.
// .SECTION See Also
// vtkProbeFilter

#ifndef vtkResampleToImage_h
#define vtkResampleToImage_h

#include "vtkAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

class vtkDataSet;
class vtkImageData;

class VTKFILTERSCORE_EXPORT vtkResampleToImage : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkResampleToImage, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkResampleToImage *New();

  // Description:
  // Set/Get if the filter should use Input bounds to sub-sample the data.
  // By default it is set to 1.
  vtkSetMacro(UseInputBounds, bool);
  vtkGetMacro(UseInputBounds, bool);
  vtkBooleanMacro(UseInputBounds, bool);

  // Description:
  // Set/Get sampling bounds. If (UseInputBounds == 1) then the sampling
  // bounds won't be used.
  vtkSetVector6Macro(SamplingBounds, double);
  vtkGetVector6Macro(SamplingBounds, double);

  // Description:
  // Set/Get sampling dimension along each axis. Default will be [10,10,10]
  vtkSetVector3Macro(SamplingDimensions, int);
  vtkGetVector3Macro(SamplingDimensions, int);

  // Description:
  // Get the output data for this algorithm.
  vtkImageData* GetOutput();

protected:
  vtkResampleToImage();
  ~vtkResampleToImage();

  // Usual data generation method
  virtual int ProcessRequest(vtkInformation*, vtkInformationVector**,
                             vtkInformationVector*);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);
  virtual int FillOutputPortInformation(int, vtkInformation *);

  static void SetBlankPointsAndCells(vtkImageData *data,
                                     const char *maskArrayName);

  bool UseInputBounds;
  double SamplingBounds[6];
  int SamplingDimensions[3];

private:
  vtkResampleToImage(const vtkResampleToImage&);  // Not implemented.
  void operator=(const vtkResampleToImage&);  // Not implemented.
};

#endif
