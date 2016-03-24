/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSPHInterpolator - interpolate over point cloud using SPH kernels

// .SECTION Description
// This filter uses SPH (smooth particle hydrodynamics) kernels to
// interpolate a data source onto an input structure. For example, while the
// data source is a set of particles, the data from these particles can be
// interpolated onto an input object such as a line, plane or volume. Then
// the output (which consists of the input structure plus interpolated data)
// can then be visualized using classical visualization techniques such as
// isocontouring, slicing, heat maps and so on.
//
// To use this filter, besides setting the input and source, specify a point
// locator (which accelerates queries about points and their neighbors) and
// an interpolation kernel (a subclass of vtkSPHKernel). In addition, the
// name of the source's density and mass arrays must be provided (or a
// strategy for computing the mass must be specified).
//
// Other options to the filter include specifying which data attributes to
// interpolate from the source. By default, all data attributes contained in
// the source are interpolated. However, by adding array names to the
// exclusion list, these arrays will not be interpolated. Also, it is
// possible to use a SPH derivative formulation to interpolate from the
// source data attributes. This requires adding arrays (by name) to the
// derivative list, in which case the derivative formulation will be applied
// to create a new output array named "X_deriv" where X is the name of a
// source point attribute array.

// .SECTION Caveats
// This class has been threaded with vtkSMPTools. Using TBB or other
// non-sequential type (set in the CMake variable
// VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
//
// For widely spaced points in Pc, or when p is located outside the bounding
// region of Pc, the interpolation may behave badly and the interpolation
// process will adapt as necessary to produce output. For example, if the N
// closest points within R are requested to interpolate p, if N=0 then the
// interpolation will switch to a different strategy (which can be controlled
// as in the NullPointsStrategy).

// .SECTION Caveats
// See D.J. Price, Smoothed particle hydrodynamics and magnetohydrodynamics,
// J. Comput. Phys. 231:759-794, 2012. Especially equation 49.

// .SECTION See Also
// vtkPointInterpolator vtkSPHKernel vtkSPHQuinticKernel

#ifndef vtkSPHInterpolator_h
#define vtkSPHInterpolator_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"
#include "vtkStdString.h"        // For vtkStdString ivars
#include <vector>

class vtkAbstractPointLocator;
class vtkIdList;
class vtkDoubleArray;
class vtkSPHKernel;
class vtkCharArray;
class vtkFloatArray;


class VTKFILTERSPOINTS_EXPORT vtkSPHInterpolator : public vtkDataSetAlgorithm
{
public:
  // Description:
  // Standard methods for instantiating, obtaining type information, and
  // printing.
  static vtkSPHInterpolator *New();
  vtkTypeMacro(vtkSPHInterpolator,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the dataset Pc that will be probed by the input points P.  The
  // Input P defines the dataset structure (the points and cells) for the
  // output, while the Source Pc is probed (interpolated) to generate the
  // scalars, vectors, etc. for the output points based on the point
  // locations.
  void SetSourceData(vtkDataObject *source);
  vtkDataObject *GetSource();

  // Description:
  // Specify the dataset Pc that will be probed by the input points P.  The
  // Input P defines the structure (the points and cells) for the output,
  // while the Source Pc is probed (interpolated) to generate the scalars,
  // vectors, etc. for the output points based on the point locations.
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  // Description:
  // Specify a point locator. By default a vtkStaticPointLocator is
  // used. The locator performs efficient searches to locate near a
  // specified interpolation position.
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);

  // Description:
  // Specify an interpolation kernel. By default a vtkVoronoiKernel is used
  // (i.e., closest point). The interpolation kernel changes the basis of the
  // interpolation.
  void SetKernel(vtkSPHKernel *kernel);
  vtkGetObjectMacro(Kernel,vtkSPHKernel);

  // Description:
  // Specify the density array name. This is required in computation. If not
  // specified the code will error out.
  vtkSetMacro(DensityArrayName,vtkStdString);
  vtkGetMacro(DensityArrayName,vtkStdString);

  // Description:
  // Specify the mass array name. If not specified, then the mass at each point
  // will be computed from the density and local (spherical) volume at a each
  // point.
  vtkSetMacro(MassArrayName,vtkStdString);
  vtkGetMacro(MassArrayName,vtkStdString);

  // How to handle NULL points
  enum NullStrategy
  {
    MASK_POINTS=0,
    NULL_VALUE=1
  };

  // Description:
  // Adds an array to the list of arrays which are to be excluded from the
  // interpolation process.
  void AddExcludedArray(vtkStdString &excludedArray)
    {this->ExcludedArrays.push_back(excludedArray);}

  // Description:
  // Clears the contents of excluded array list.
  void ClearExcludedArrays()
    {this->ExcludedArrays.clear();}

  // Description:
  // Return the number of excluded arrays.
  int GetNumberOfExcludedArrays()
    {return static_cast<int>(this->ExcludedArrays.size());}

  // Description:
  // Return the name of the ith excluded array.
  const char* GetExcludedArray(int i)
    {
      if ( i < 0 || i >= static_cast<int>(this->ExcludedArrays.size()) )
        {
        return NULL;
        }
      return this->ExcludedArrays[i].c_str();
    }
  // Description:
  // Adds an array to the list of arrays whose derivative is to be taken. If
  // the name of the array is "derivArray" this will produce an output array
  // with the name "derivArray_deriv" (after filter execution).
  void AddDerivativeArray(vtkStdString &derivArray)
    {this->DerivArrays.push_back(derivArray);}

  // Description:
  // Clears the contents of derivative array list.
  void ClearDerivativeArrays()
    {this->DerivArrays.clear();}

  // Description:
  // Return the number of derivative arrays.
  int GetNumberOfDerivativeArrays()
    {return static_cast<int>(this->DerivArrays.size());}

  // Description:
  // Return the name of the ith derivative array.
  const char* GetDerivativeArray(int i)
    {
      if ( i < 0 || i >= static_cast<int>(this->DerivArrays.size()) )
        {
        return NULL;
        }
      return this->DerivArrays[i].c_str();
    }

  // Description:
  // Specify a strategy to use when encountering a "null" point during the
  // interpolation process. Null points occur when the local neighborhood (of
  // nearby points to interpolate from) is empty. If the strategy is set to
  // MaskPoints, then an output array is created that marks points as being
  // valid (=1) or null (invalid =0) (and the NullValue is set as well). If
  // the strategy is set to NullValue, then the output data value(s) are set
  // to the NullPoint value (specified in the output point data).
  vtkSetMacro(NullPointsStrategy,int);
  vtkGetMacro(NullPointsStrategy,int);
  void SetNullPointsStrategyToMaskPoints()
    { this->SetNullPointsStrategy(MASK_POINTS); }
  void SetNullPointsStrategyToNullValue()
    { this->SetNullPointsStrategy(NULL_VALUE); }

  // Description:
  // Indicate whether to compute the summation of Shepard coefficients. In the
  // interior of a SPH point cloud, the Shephard summation value should be ~1.0.
  // Towards the boundary, the Shepard summation is <1.0. If ComputeShepardSum
  // is specified, then the output will contain an array of summed Shepard weights
  // for each output point. On by default.
  vtkSetMacro(ComputeShepardSum, bool);
  vtkBooleanMacro(ComputeShepardSum, bool);
  vtkGetMacro(ComputeShepardSum, bool);

  // Description:
  // If ComputeShepardSum is on, then an array is generated with name
  // ShepardSumArrayName for each input point. This vtkFloatArray is placed
  // into the output of the filter, and NullPoints have value =0.0. The
  // default name is "Shepard Summation".
  vtkSetMacro(ShepardSumArrayName, vtkStdString);
  vtkGetMacro(ShepardSumArrayName, vtkStdString);

  // Description:
  // If the NullPointsStrategy == MASK_POINTS, then an array is generated for
  // each input point. This vtkCharArray is placed into the output of the filter,
  // with a non-zero value for a valid point, and zero otherwise. The name of
  // this masking array is specified here.
  vtkSetMacro(ValidPointsMaskArrayName, vtkStdString);
  vtkGetMacro(ValidPointsMaskArrayName, vtkStdString);

  // Description:
  // Specify the null point value. When a null point is encountered then all
  // components of each null tuple are set to this value. By default the
  // null value is set to zero.
  vtkSetMacro(NullValue,double);
  vtkGetMacro(NullValue,double);

  // Description:
  // Indicate whether to shallow copy the input point data arrays to the
  // output. On by default.
  vtkSetMacro(PassPointArrays, bool);
  vtkBooleanMacro(PassPointArrays, bool);
  vtkGetMacro(PassPointArrays, bool);

  // Description:
  // Indicate whether to shallow copy the input cell data arrays to the
  // output. On by default.
  vtkSetMacro(PassCellArrays, bool);
  vtkBooleanMacro(PassCellArrays, bool);
  vtkGetMacro(PassCellArrays, bool);

  // Description:
  // Indicate whether to pass the field-data arrays from the input to the
  // output. On by default.
  vtkSetMacro(PassFieldArrays, bool);
  vtkBooleanMacro(PassFieldArrays, bool);
  vtkGetMacro(PassFieldArrays, bool);

protected:
  vtkSPHInterpolator();
  ~vtkSPHInterpolator();

  vtkAbstractPointLocator *Locator;
  vtkSPHKernel *Kernel;

  vtkStdString DensityArrayName;
  vtkStdString MassArrayName;

  std::vector<vtkStdString> ExcludedArrays;
  std::vector<vtkStdString> DerivArrays;

  int NullPointsStrategy;
  double NullValue;
  vtkStdString ValidPointsMaskArrayName;
  vtkCharArray *ValidPointsMask;

  bool ComputeShepardSum;
  vtkStdString ShepardSumArrayName;
  vtkFloatArray *ShepardSumArray;

  bool PassCellArrays;
  bool PassPointArrays;
  bool PassFieldArrays;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);

  // Description:
  // Virtual for specialized subclass(es)
  virtual void Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output);

  // Description:
  // Call at end of RequestData() to pass attribute data respecting the
  // PassCellArrays, PassPointArrays, PassFieldArrays flags.
  virtual void PassAttributeData(
    vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

  // Description:
  // Internal method to extract image metadata
  void ExtractImageDescription(vtkImageData *input, int dims[3],
                               double origin[3], double spacing[3]);

private:
  vtkSPHInterpolator(const vtkSPHInterpolator&);  // Not implemented.
  void operator=(const vtkSPHInterpolator&);  // Not implemented.

};

#endif
