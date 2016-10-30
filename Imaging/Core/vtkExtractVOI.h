/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractVOI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractVOI
 * @brief   select piece (e.g., volume of interest) and/or subsample structured points dataset
 *
 *
 * vtkExtractVOI is a filter that selects a portion of an input structured
 * points dataset, or subsamples an input dataset. (The selected portion of
 * interested is referred to as the Volume Of Interest, or VOI.) The output of
 * this filter is a structured points dataset. The filter treats input data
 * of any topological dimension (i.e., point, line, image, or volume) and can
 * generate output data of any topological dimension.
 *
 * To use this filter set the VOI ivar which are i-j-k min/max indices that
 * specify a rectangular region in the data. (Note that these are 0-offset.)
 * You can also specify a sampling rate to subsample the data.
 *
 * Typical applications of this filter are to extract a slice from a volume
 * for image processing, subsampling large volumes to reduce data size, or
 * extracting regions of a volume with interesting data.
 *
 * @sa
 * vtkGeometryFilter vtkExtractGeometry vtkExtractGrid
*/

#ifndef vtkExtractVOI_h
#define vtkExtractVOI_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImageAlgorithm.h"

// Forward Declarations
class vtkExtractStructuredGridHelper;

class VTKIMAGINGCORE_EXPORT vtkExtractVOI : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkExtractVOI,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object to extract all of the input data.
   */
  static vtkExtractVOI *New();

  //@{
  /**
   * Specify i-j-k (min,max) pairs to extract. The resulting structured points
   * dataset can be of any topological dimension (i.e., point, line, image,
   * or volume).
   */
  vtkSetVector6Macro(VOI,int);
  vtkGetVectorMacro(VOI,int,6);
  //@}

  //@{
  /**
   * Set the sampling rate in the i, j, and k directions. If the rate is >
   * 1, then the resulting VOI will be subsampled representation of the
   * input.  For example, if the SampleRate=(2,2,2), every other point will
   * be selected, resulting in a volume 1/8th the original size.
   */
  vtkSetVector3Macro(SampleRate, int);
  vtkGetVectorMacro(SampleRate, int, 3);
  //@}

  //@{
  /**
   * Control whether to enforce that the "boundary" of the grid is output in
   * the subsampling process. (This ivar only has effect when the SampleRate
   * in any direction is not equal to 1.) When this ivar IncludeBoundary is
   * on, the subsampling will always include the boundary of the grid even
   * though the sample rate is not an even multiple of the grid
   * dimensions. (By default IncludeBoundary is off.)
   */
  vtkSetMacro(IncludeBoundary,int);
  vtkGetMacro(IncludeBoundary,int);
  vtkBooleanMacro(IncludeBoundary,int);
  //@}

protected:
  vtkExtractVOI();
  ~vtkExtractVOI();

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  /**
   * Implementation for RequestData using a specified VOI. This is because the
   * parallel filter needs to muck around with the VOI to get spacing and
   * partitioning to play nice. The VOI is calculated from the output
   * data object's extents in this implementation.
   */
  bool RequestDataImpl(vtkInformationVector **inputVector,
                       vtkInformationVector *outputVector);

  int VOI[6];
  int SampleRate[3];
  int IncludeBoundary;

  vtkExtractStructuredGridHelper* Internal;
private:
  vtkExtractVOI(const vtkExtractVOI&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractVOI&) VTK_DELETE_FUNCTION;
};

#endif


