/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkAdaptiveResampleToImage
 * @brief samples a dataset with adaptive refinements.
 *
 * vtkAdaptiveResampleToImage resamples any dataset to a `vtkPartitionedDataSet`
 * comprising of `vtkImageData`. Each partition may have different spacing thus
 * spanning different spatial regions from the input dataset.
 *
 * vtkAdaptiveResampleToImage builds a kd-tree with at least as many leaf nodes
 * as requested using `SetNumberOfImages`. The kd tree is built by splitting the
 * points in the input dataset. The bounds of each leaf are then used to
 * determine the bounds (i.e. spacing and origin) for the image dataset for that
 * leaf which will have the dimensions requested (set using `SetSamplingDimensions`).
 *
 * `NumberOfImages` is simply a hint and the tree will have exactly as many
 * leaves as the nearest power of two not less than `NumberOfImages` (see
 * `vtkMath::NearestPowerOfTwo`). If set to 0, the number of images requested is assumed
 * to be same as the number of parallel MPI ranks.
 *
 * When running in parallel, the leaf nodes of the kd-tree are assigned to
 * individual ranks. If the leaf nodes is exactly same as the number of MPI
 * ranks, then each rank gets a leaf. If the leaf nodes is less than the MPI
 * ranks, the extra ranks will not be assigned any data and will generate an
 * empty `vtkPartitionedDataSet` in the output. If the number of leaf nodes is
 * greater than the number of ranks, then each rank my be assigned more than 1
 * block. The assignment algorithm, however, preserves the kd-tree across ranks
 * i.e. a rank will always be assigned a complete sub-tree (which may be simply
 * the leaf node). @sa `vtkDIYKdTreeUtilities::CreateAssigner`,
 * `vtkDIYKdTreeUtilities::ComputeAssignments`.
 *
 */

#ifndef vtkAdaptiveResampleToImage_h
#define vtkAdaptiveResampleToImage_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelDIY2Module.h" // For export macro

class vtkMultiProcessController;

class VTKFILTERSPARALLELDIY2_EXPORT vtkAdaptiveResampleToImage : public vtkDataObjectAlgorithm
{
public:
  static vtkAdaptiveResampleToImage* New();
  vtkTypeMacro(vtkAdaptiveResampleToImage, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Get/Set a hint to use to indicate how many different refinements to split
   * the dataset into. This is just a hint. The actual number of images used to
   * resample the input dataset is the nearest power-of-two not less than the
   * requested value (@sa vtkMath::NearestPowerOfTwo).
   */
  vtkSetClampMacro(NumberOfImages, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfImages, int);
  //@}

  //@{
  /**
   * Set/Get sampling dimensions along each axis. Each partition will be
   * resampled using these dimensions.
   */
  vtkSetVector3Macro(SamplingDimensions, int);
  vtkGetVector3Macro(SamplingDimensions, int);
  //@}
protected:
  vtkAdaptiveResampleToImage();
  ~vtkAdaptiveResampleToImage() override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkAdaptiveResampleToImage(const vtkAdaptiveResampleToImage&) = delete;
  void operator=(const vtkAdaptiveResampleToImage&) = delete;

  vtkMultiProcessController* Controller;
  int NumberOfImages;
  int SamplingDimensions[3];
};

#endif
