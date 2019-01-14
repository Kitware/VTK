/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointInterpolator
 * @brief   interpolate over point cloud using various kernels
 *
 *
 * vtkPointInterpolator probes a point cloud Pc (the filter Source) with a
 * set of points P (the filter Input), interpolating the data values from Pc
 * onto P. Note however that the descriptive phrase "point cloud" is a
 * misnomer: Pc can be represented by any vtkDataSet type, with the points of
 * the dataset forming Pc. Similarly, the output P can also be represented by
 * any vtkDataSet type; and the topology/geometry structure of P is passed
 * through to the output along with the newly interpolated arrays.
 *
 * A key input to this filter is the specification of the interpolation
 * kernel, and the parameters which control the associated interpolation
 * process. Interpolation kernels include Voronoi, Gaussian, Shepard, and SPH
 * (smoothed particle hydrodynamics), with additional kernels to be added in
 * the future.
 *
 * An overview of the algorithm is as follows. For each p from P, Np "close"
 * points to p are found. (The meaning of what is "close" can be specified as
 * either the N closest points, or all points within a given radius Rp. This
 * depends on how the kernel is defined.) Once the Np close points are found,
 * then the interpolation kernel is applied to compute new data values
 * located on p. Note that for reasonable performance, finding the Np closest
 * points requires a point locator. The locator may be specified as input to
 * the algorithm. (By default, a vtkStaticPointLocator is used because
 * generally it is much faster to build, delete, and search with. However,
 * with highly non-uniform point distributions, octree- or kd-tree based
 * locators may perform better.)
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * For widely spaced points in Pc, or when p is located outside the bounding
 * region of Pc, the interpolation may behave badly and the interpolation
 * process will adapt as necessary to produce output. For example, if the N
 * closest points within R are requested to interpolate p, if N=0 then the
 * interpolation will switch to a different strategy (which can be controlled
 * as in the NullPointsStrategy).
 *
 * @sa
 * vtkPointInterpolator2D vtkProbeFilter vtkGaussianSplatter
 * vtkCheckerboardSplatter vtkShepardMethod vtkVoronoiKernel vtkShepardKernel
 * vtkGaussianKernel vtkSPHKernel
*/

#ifndef vtkPointInterpolator_h
#define vtkPointInterpolator_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"
#include "vtkStdString.h"        // For vtkStdString ivars
#include <vector> //For STL vector

class vtkAbstractPointLocator;
class vtkIdList;
class vtkDoubleArray;
class vtkInterpolationKernel;
class vtkCharArray;


class VTKFILTERSPOINTS_EXPORT vtkPointInterpolator : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing.
   */
  static vtkPointInterpolator *New();
  vtkTypeMacro(vtkPointInterpolator,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the dataset Pc that will be probed by the input points P.  The
   * Input P defines the dataset structure (the points and cells) for the
   * output, while the Source Pc is probed (interpolated) to generate the
   * scalars, vectors, etc. for the output points based on the point
   * locations.
   */
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();
  //@}

  /**
   * Specify the dataset Pc that will be probed by the input points P.  The
   * Input P defines the structure (the points and cells) for the output,
   * while the Source Pc is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate near a
   * specified interpolation position.
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

  //@{
  /**
   * Specify an interpolation kernel. By default a vtkLinearKernel is used
   * (i.e., linear combination of closest points). The interpolation kernel
   * changes the basis of the interpolation.
   */
  void SetKernel(vtkInterpolationKernel *kernel);
  vtkGetObjectMacro(Kernel,vtkInterpolationKernel);
  //@}

  enum Strategy
  {
    MASK_POINTS=0,
    NULL_VALUE=1,
    CLOSEST_POINT=2
  };

  //@{
  /**
   * Specify a strategy to use when encountering a "null" point during the
   * interpolation process. Null points occur when the local neighborhood (of
   * nearby points to interpolate from) is empty. If the strategy is set to
   * MaskPoints, then an output array is created that marks points as being
   * valid (=1) or null (invalid =0) (and the NullValue is set as well). If
   * the strategy is set to NullValue (this is the default), then the output
   * data value(s) are set to the NullPoint value (specified in the output
   * point data). Finally, the strategy ClosestPoint is to simply use the
   * closest point to perform the interpolation.
   */
  vtkSetMacro(NullPointsStrategy,int);
  vtkGetMacro(NullPointsStrategy,int);
  void SetNullPointsStrategyToMaskPoints()
    { this->SetNullPointsStrategy(MASK_POINTS); }
  void SetNullPointsStrategyToNullValue()
    { this->SetNullPointsStrategy(NULL_VALUE); }
  void SetNullPointsStrategyToClosestPoint()
    { this->SetNullPointsStrategy(CLOSEST_POINT); }
  //@}

  //@{
  /**
   * If the NullPointsStrategy == MASK_POINTS, then an array is generated for
   * each input point. This vtkCharArray is placed into the output of the filter,
   * with a non-zero value for a valid point, and zero otherwise. The name of
   * this masking array is specified here.
   */
  vtkSetMacro(ValidPointsMaskArrayName, vtkStdString);
  vtkGetMacro(ValidPointsMaskArrayName, vtkStdString);
  //@}

  //@{
  /**
   * Specify the null point value. When a null point is encountered then all
   * components of each null tuple are set to this value. By default the
   * null value is set to zero.
   */
  vtkSetMacro(NullValue,double);
  vtkGetMacro(NullValue,double);
  //@}

  //@{
  /**
   * Adds an array to the list of arrays which are to be excluded from the
   * interpolation process.
   */
  void AddExcludedArray(const vtkStdString &excludedArray)
  {
    this->ExcludedArrays.push_back(excludedArray);
    this->Modified();
  }
  //@}

  //@{
  /**
   * Clears the contents of excluded array list.
   */
  void ClearExcludedArrays()
  {
    this->ExcludedArrays.clear();
    this->Modified();
  }
  //@}

  /**
   * Return the number of excluded arrays.
   */
  int GetNumberOfExcludedArrays()
    {return static_cast<int>(this->ExcludedArrays.size());}

  //@{
  /**
   * Return the name of the ith excluded array.
   */
  const char* GetExcludedArray(int i)
  {
      if ( i < 0 || i >= static_cast<int>(this->ExcludedArrays.size()) )
      {
        return nullptr;
      }
      return this->ExcludedArrays[i].c_str();
  }
  //@}

  //@{
  /**
   * If enabled, then input arrays that are non-real types (i.e., not float
   * or double) are promoted to float type on output. This is because the
   * interpolation process may not be well behaved when integral types are
   * combined using interpolation weights.
   */
  vtkSetMacro(PromoteOutputArrays, bool);
  vtkBooleanMacro(PromoteOutputArrays, bool);
  vtkGetMacro(PromoteOutputArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input point data arrays to the
   * output.  On by default.
   */
  vtkSetMacro(PassPointArrays, bool);
  vtkBooleanMacro(PassPointArrays, bool);
  vtkGetMacro(PassPointArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input cell data arrays to the
   * output.  On by default.
   */
  vtkSetMacro(PassCellArrays, bool);
  vtkBooleanMacro(PassCellArrays, bool);
  vtkGetMacro(PassCellArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to pass the field-data arrays from the input to the
   * output. On by default.
   */
  vtkSetMacro(PassFieldArrays, bool);
  vtkBooleanMacro(PassFieldArrays, bool);
  vtkGetMacro(PassFieldArrays, bool);
  //@}

  /**
   * Get the MTime of this object also considering the locator and kernel.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkPointInterpolator();
  ~vtkPointInterpolator() override;

  vtkAbstractPointLocator *Locator;
  vtkInterpolationKernel *Kernel;

  int NullPointsStrategy;
  double NullValue;
  vtkStdString ValidPointsMaskArrayName;
  vtkCharArray *ValidPointsMask;

  std::vector<vtkStdString> ExcludedArrays;

  bool PromoteOutputArrays;

  bool PassCellArrays;
  bool PassPointArrays;
  bool PassFieldArrays;

  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;

  /**
   * Virtual for specialized subclass(es)
   */
  virtual void Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output);

  /**
   * Call at end of RequestData() to pass attribute data respecting the
   * PassCellArrays, PassPointArrays, PassFieldArrays flags.
   */
  virtual void PassAttributeData(
    vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

  /**
   * Internal method to extract image metadata
   */
  void ExtractImageDescription(vtkImageData *input, int dims[3],
                               double origin[3], double spacing[3]);

private:
  vtkPointInterpolator(const vtkPointInterpolator&) = delete;
  void operator=(const vtkPointInterpolator&) = delete;

};

#endif
