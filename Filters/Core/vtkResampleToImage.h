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
/**
 * @class   vtkResampleToImage
 * @brief   sample dataset on a uniform grid
 *
 * vtkPResampleToImage is a filter that resamples the input dataset on
 * a uniform grid. It internally uses vtkProbeFilter to do the probing.
 * @sa
 * vtkProbeFilter
*/

#ifndef vtkResampleToImage_h
#define vtkResampleToImage_h

#include "vtkAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkNew.h" // For vtkCompositeDataProbeFilter member variable


class vtkDataObject;
class vtkImageData;
class vtkCompositeDataProbeFilter;

class VTKFILTERSCORE_EXPORT vtkResampleToImage : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkResampleToImage, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkResampleToImage *New();

  //@{
  /**
   * Set/Get if the filter should use Input bounds to sub-sample the data.
   * By default it is set to 1.
   */
  vtkSetMacro(UseInputBounds, bool);
  vtkGetMacro(UseInputBounds, bool);
  vtkBooleanMacro(UseInputBounds, bool);
  //@}

  //@{
  /**
   * Set/Get sampling bounds. If (UseInputBounds == 1) then the sampling
   * bounds won't be used.
   */
  vtkSetVector6Macro(SamplingBounds, double);
  vtkGetVector6Macro(SamplingBounds, double);
  //@}

  //@{
  /**
   * Set/Get sampling dimension along each axis. Default will be [10,10,10]
   */
  vtkSetVector3Macro(SamplingDimensions, int);
  vtkGetVector3Macro(SamplingDimensions, int);
  //@}

  /**
   * Get the output data for this algorithm.
   */
  vtkImageData* GetOutput();

protected:
  vtkResampleToImage();
  ~vtkResampleToImage() VTK_OVERRIDE;

  // Usual data generation method
  int ProcessRequest(vtkInformation*, vtkInformationVector**,
                     vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);
  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation *) VTK_OVERRIDE;

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

  /**
   * Resample input vtkDataObject to a vtkImageData with the specified bounds
   * and extent.
   */
  void PerformResampling(vtkDataObject *input, const double samplingBounds[6],
                         bool computeProbingExtent, const double inputBounds[6],
                         vtkImageData *output);

  /**
   * Mark invalid points and cells of vtkImageData as hidden
   */
  void SetBlankPointsAndCells(vtkImageData *data);

  /**
   * Helper function to compute the bounds of the given vtkDataSet or
   * vtkCompositeDataSet
   */
  static void ComputeDataBounds(vtkDataObject *data, double bounds[6]);


  bool UseInputBounds;
  double SamplingBounds[6];
  int SamplingDimensions[3];
  vtkNew<vtkCompositeDataProbeFilter> Prober;

private:
  vtkResampleToImage(const vtkResampleToImage&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResampleToImage&) VTK_DELETE_FUNCTION;
};

#endif
