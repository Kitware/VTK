/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedDistance.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnsignedDistance
 * @brief   compute unsigned (i.e., non-negative) distances from an input point cloud
 *
 * vtkUnsignedDistance is a filter that computes non-negative (i.e., unsigned)
 * distances over a volume from an input point cloud. This filter is distinct
 * from vtkSignedDistance in that it does not require point normals. However,
 * isocontouring a zero-valued distance function (e.g., trying to fit a
 * surface will produce unsatisfactory results). Rather this filter, when
 * combined with an isocontouring filter such as vtkFlyingEdges3D, can
 * produce an offset, bounding surface surrounding the input point cloud.
 *
 * To use this filter, specify the input vtkPolyData (which represents the
 * point cloud); define the sampling volume; specify a radius (which limits
 * the radius of influence of each point); and set an optional point locator
 * (to accelerate proximity operations, a vtkStaticPointLocator is used by
 * default). Note that large radius values may have significant impact on
 * performance. The volume is defined by specifying dimensions in the x-y-z
 * directions, as well as a domain bounds. By default the model bounds are
 * defined from the input points, but the user can also manually specify
 * them. Finally, because the radius data member limits the influence of the
 * distance calulation, some voxels may receive no contribution. These voxel
 * values are set to the CapValue.
 *
 * This filter has one other unusual capability: it is possible to append
 * data in a sequence of operations to generate a single output. This is
 * useful when you have multiple point clouds (e.g., possibly from multiple
 * acqusition scans) and want to incrementally accumulate all the data.
 * However, the user must be careful to either specify the Bounds or
 * order the input such that the bounds of the first input completely
 * contains all other input data.  This is because the geometry and topology
 * of the output sampling volume cannot be changed after the initial Append
 * operation.
 *
 * @warning
 * Note that multiple, non-connected surfaces may be produced. For example,
 * if the point cloud is from the surface of the sphere, it is possible to
 * generate two surfaces (with isocontouring): one inside the sphere, one
 * outside the sphere. It is sometimes possible to select the surface you
 * want from the output of the contouring filter by using
 * vtkPolyDataConnectivityFilter.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkSignedDistance vtkExtractSurface vtkImplicitModeller
*/

#ifndef vtkUnsignedDistance_h
#define vtkUnsignedDistance_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class vtkPolyData;
class vtkAbstractPointLocator;


class VTKFILTERSPOINTS_EXPORT vtkUnsignedDistance : public vtkImageAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating the class, providing type information,
   * and printing.
   */
  static vtkUnsignedDistance *New();
  vtkTypeMacro(vtkUnsignedDistance,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the i-j-k dimensions on which to computer the distance function.
   */
  vtkGetVectorMacro(Dimensions,int,3);
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);
  //@}

  //@{
  /**
   * Set / get the region in space in which to perform the sampling. If
   * not specified, it will be computed automatically.
   */
  vtkSetVector6Macro(Bounds,double);
  vtkGetVectorMacro(Bounds,double,6);
  //@}

  //@{
  /**
   * Control how the model bounds are computed. If the ivar AdjustBounds
   * is set, then the bounds specified (or computed automatically) is modified
   * by the fraction given by AdjustDistance. This means that the model
   * bounds is expanded in each of the x-y-z directions.
   */
  vtkSetMacro(AdjustBounds,int);
  vtkGetMacro(AdjustBounds,int);
  vtkBooleanMacro(AdjustBounds,int);
  //@}

  //@{
  /**
   * Specify the amount to grow the model bounds (if the ivar AdjustBounds
   * is set). The value is a fraction of the maximum length of the sides
   * of the box specified by the model bounds.
   */
  vtkSetClampMacro(AdjustDistance,double,-1.0,1.0);
  vtkGetMacro(AdjustDistance,double);
  //@}

  //@{
  /**
   * Set / get the radius of influence of each point. Smaller values
   * generally improve performance markedly.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * surrounding a voxel (within the specified radius).
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

  //@{
  /**
   * The outer boundary of the volume can be assigned a particular value
   * after distances are computed. This can be used to close or "cap" all
   * surfaces during isocontouring.
   */
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);
  //@}

  //@{
  /**
   * Specify the capping value to use. The CapValue is also used as an
   * initial distance value at each point in the dataset. By default, the
   * CapValue is VTK_FLOAT_MAX;
   */
  vtkSetMacro(CapValue,double);
  vtkGetMacro(CapValue,double);
  //@}

  //@{
  /**
   * Set the desired output scalar type. Currently only real types are
   * supported. By default, VTK_FLOAT scalars are created.
   */
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToFloat(){this->SetOutputScalarType(VTK_FLOAT);};
  void SetOutputScalarTypeToDouble(){this->SetOutputScalarType(VTK_DOUBLE);};
  //@}

  /**
   * Initialize the filter for appending data. You must invoke the
   * StartAppend() method before doing successive Appends(). It's also a
   * good idea to manually specify the model bounds; otherwise the input
   * bounds for the data will be used.
   */
  void StartAppend();

  /**
   * Append a data set to the existing output. To use this function,
   * you'll have to invoke the StartAppend() method before doing
   * successive appends. It's also a good idea to specify the model
   * bounds; otherwise the input model bounds is used. When you've
   * finished appending, use the EndAppend() method.
   */
  void Append(vtkPolyData *input);

  /**
   * Method completes the append process.
   */
  void EndAppend();

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkUnsignedDistance();
  ~vtkUnsignedDistance();

  int Dimensions[3];
  double Bounds[6];
  int AdjustBounds;
  double AdjustDistance;
  double Radius;
  vtkAbstractPointLocator *Locator;
  int Capping;
  double CapValue;
  int OutputScalarType;

  // Flag tracks whether process needs initialization
  int Initialized;

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;
  virtual int RequestData (vtkInformation *,
                           vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  virtual int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkUnsignedDistance(const vtkUnsignedDistance&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnsignedDistance&) VTK_DELETE_FUNCTION;

};

#endif
