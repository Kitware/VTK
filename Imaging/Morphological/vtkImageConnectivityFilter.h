/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================
  Copyright (c) 2014 David Gobbi
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of David Gobbi nor the names of any contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/
/**
 * @class   vtkImageConnectivityFilter
 * @brief   Label an image by connectivity
 *
 * vtkImageConnectivityFilter will identify connected regions within an
 * image and label them.  Only points with scalar values within a
 * prescribed range are considered for inclusion, by default this range
 * includes all scalar values with a value greater than zero.  Points
 * within the prescribed scalar range are considered to be connected
 * if a path exists between the points that does not traverse any
 * points that are not within the prescribed scalar range.
 * Adjacency of points is governed by 4-connectivity for 2D images, and
 * 6-connectivity for 3D images.
 *
 * The output of this filter is a label image.  By default, each region
 * is assigned a different label, where the labels are integer values
 * starting at a value of 1.  The SetLabelMode() method can be used to
 * change the way that labels are assigned.  Labels can be assigned
 * by providing input seed points for each region to be labelled, or
 * they can be assigned by ranking the regions by size.
 *
 * If a set of seeds is provided with the SetSeedData() method,
 * then the default behavior is to only output the regions that are
 * connected to the seeds, and if the seeds have scalars, then these
 * scalars will be used to label the regions.  Seeds with a scalar
 * value equal to zero are ignored.  See the documentation for the
 * SetExtractionMode() method for details on how to control which
 * regions will labeled.
 *
 * Regions can be selected by size with the SetSizeRange() method,
 * which can be useful for identifying objects of a certain size,
 * e.g. for rejecting small regions that are likely to be noise.
 * It is also possible to label only the largest region and ignore
 * all others, with SetExtractionModeToLargestRegion().
 *
 * In addition to the labels, the following additional information
 * is provided: the number of regions identified, the size of each
 * region, a list of all label values used, and the seed for each
 * region (if seeds were used).  Optionally, this filter can also
 * compute the extent of each region if GenerateRegionExtentsOn()
 * is called.  These extents can be useful for cropping the output
 * of the filter.
 *
 * @sa
 * vtkConnectivityFilter, vtkPolyDataConnectivityFilter
*/

#ifndef vtkImageConnectivityFilter_h
#define vtkImageConnectivityFilter_h

#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkIdTypeArray;
class vtkIntArray;
class vtkDataSet;
class vtkImageData;
class vtkImageStencilData;

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageConnectivityFilter :
  public vtkImageAlgorithm
{
public:
  static vtkImageConnectivityFilter *New();
  vtkTypeMacro(vtkImageConnectivityFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Enum constants for SetLabelMode().
   */
  enum LabelModeEnum {
    SeedScalar = 0,
    ConstantValue = 1,
    SizeRank = 2
  };

  /**
   * Enum constants for SetExtractionMode().
   */
  enum ExtractionModeEnum {
    SeededRegions = 0,
    AllRegions = 1,
    LargestRegion = 2
  };

  //@{
  /**
   * The input for seed locations (input port 1).
   * Each point in the supplied data set will be used as a seed, unless
   * the data set has scalars, in which case only the points with scalar
   * values that are not equal to zero will be used as seeds.
   */
  void SetSeedConnection(vtkAlgorithmOutput *port);
  vtkAlgorithmOutput *GetSeedConnection();
  void SetSeedData(vtkDataSet *data);
  //@}

  //@{
  /**
   * The input for a stencil (input port 2).
   * The output labels will be restricted to the region inside the stencil,
   * as if no input voxels existed outside the stencil.  This allows you to
   * apply this filter within an arbitrary region of interest.
   */
  void SetStencilConnection(vtkAlgorithmOutput *port);
  vtkAlgorithmOutput *GetStencilConnection();
  void SetStencilData(vtkImageStencilData *data);
  //@}

  //@{
  /**
   * Set the scalar type for the output label image.
   * This should be one of UnsignedChar, Short, UnsignedShort, or Int
   * depending on how many labels are expected.  The default is UnsignedChar,
   * which allows for 255 label values.  If the total number of regions is
   * greater than the maximum label value N, then only the largest N regions
   * will be kept and the rest will be discarded.
   */
  void SetLabelScalarTypeToUnsignedChar() {
    this->SetLabelScalarType(VTK_UNSIGNED_CHAR); }
  void SetLabelScalarTypeToShort() {
    this->SetLabelScalarType(VTK_SHORT); }
  void SetLabelScalarTypeToUnsignedShort() {
    this->SetLabelScalarType(VTK_UNSIGNED_SHORT); }
  void SetLabelScalarTypeToInt() {
    this->SetLabelScalarType(VTK_INT); }
  const char *GetLabelScalarTypeAsString();
  vtkSetMacro(LabelScalarType, int);
  vtkGetMacro(LabelScalarType, int);
  //@}

  //@{
  /**
   * Set the mode for applying labels to the output.
   * Labeling by SeedScalar uses the scalars from the seeds as labels, if
   * present, or the regions will be labeled consecutively starting at 1,
   * if the seeds have no scalars. Labeling by SizeRank means that the
   * largest region is labeled 1 and other regions are labeled consecutively
   * in order of decreasing size (if there is a tie, then the seed point ID
   * is used as a tiebreaker).  Finally, Constant means that all regions
   * will have the value of SetLabelConstantValue().  The default is to
   * label using the seed scalars, if present, or to label consecutively,
   * if no seed scalars are present.
   */
  void SetLabelModeToSeedScalar() { this->SetLabelMode(SeedScalar); }
  void SetLabelModeToConstantValue() { this->SetLabelMode(ConstantValue); }
  void SetLabelModeToSizeRank() { this->SetLabelMode(SizeRank); }
  const char *GetLabelModeAsString();
  vtkSetMacro(LabelMode, int);
  vtkGetMacro(LabelMode, int);
  //@}

  //@{
  /**
   * Set which regions to output from this filter.
   * This can be all the regions, just the seeded regions, or the largest
   * region (which will the the largest seeded region, if there are seeds).
   * The default is to output all the seeded regions, if there are seeds,
   * or to output all the regions, if there are no seeds.
   */
  void SetExtractionModeToSeededRegions(){
    this->SetExtractionMode(SeededRegions); }
  void SetExtractionModeToAllRegions() {
    this->SetExtractionMode(AllRegions); }
  void SetExtractionModeToLargestRegion() {
    this->SetExtractionMode(LargestRegion); }
  const char *GetExtractionModeAsString();
  vtkSetMacro(ExtractionMode, int);
  vtkGetMacro(ExtractionMode, int);
  //@}

  //@{
  /**
   * The label used when LabelMode is ConstantValue.
   * The default value is 255.
   */
  vtkSetMacro(LabelConstantValue, int);
  vtkGetMacro(LabelConstantValue, int);
  //@}

  /**
   * Get the number of extracted regions.
   */
  vtkIdType GetNumberOfExtractedRegions();

  /**
   * Get the label used for each extracted region.
   */
  vtkIdTypeArray *GetExtractedRegionLabels() {
    return this->ExtractedRegionLabels; }

  // Desciption:
  // Get the size of each extracted region, as a voxel count.
  vtkIdTypeArray *GetExtractedRegionSizes() {
    return this->ExtractedRegionSizes; }

  /**
   * Get the PointId of the seed for each region.
   * If no seed was used, the PointId will be -1.
   */
  vtkIdTypeArray *GetExtractedRegionSeedIds() {
    return this->ExtractedRegionSeedIds; }

  /**
   * Get the extent (a 6-tuples) for each output region.
   * This is only valid if GenerateRegionExtentsOn() was called before
   * the filter was executed.
   */
  vtkIntArray *GetExtractedRegionExtents() {
    return this->ExtractedRegionExtents; }

  //@{
  /**
   * Turn this on to request creation of the ExtractedRegionExtents array.
   */
  vtkSetMacro(GenerateRegionExtents, int);
  vtkBooleanMacro(GenerateRegionExtents, int);
  vtkGetMacro(GenerateRegionExtents, int);
  //@}

  //@{
  /**
   * Set the size range for the extracted regions.
   * Only regions that have sizes within the specified range will be present
   * in the output.  The default range is (1, VTK_ID_MAX).
   */
  vtkSetVector2Macro(SizeRange, vtkIdType);
  vtkGetVector2Macro(SizeRange, vtkIdType);
  //@}

  //@{
  /**
   * Set the scalar range used to define potential regions.
   * Only voxels with values that are within this range will be considered
   * for region membership.  This is an inclusive range, meaning that the
   * upper and lower limits are considered to be within the range.  The
   * default range goes from 0.5 to VTK_DOUBLE_MAX.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVector2Macro(ScalarRange, double);
  //@}

  //@{
  /**
   * For multi-component input images, select which component to use.
   */
  vtkSetMacro(ActiveComponent, int);
  vtkGetMacro(ActiveComponent, int);
  //@}

protected:
  vtkImageConnectivityFilter();
  ~vtkImageConnectivityFilter() VTK_OVERRIDE;

  int LabelMode;
  int ExtractionMode;

  double ScalarRange[2];
  vtkIdType SizeRange[2];
  int LabelConstantValue;
  int ActiveComponent;
  int LabelScalarType;
  int GenerateRegionExtents;

  vtkIdTypeArray *ExtractedRegionLabels;
  vtkIdTypeArray *ExtractedRegionSizes;
  vtkIdTypeArray *ExtractedRegionSeedIds;
  vtkIntArray *ExtractedRegionExtents;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestUpdateExtent(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkImageConnectivityFilter(const vtkImageConnectivityFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageConnectivityFilter&) VTK_DELETE_FUNCTION;
};

#endif
