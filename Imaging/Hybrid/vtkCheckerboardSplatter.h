/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardSplatter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCheckerboardSplatter
 * @brief   splat points into a volume with an elliptical, Gaussian distribution
 *
 * vtkCheckerboardSplatter is a filter that injects input points into a
 * structured points (volume) dataset using a multithreaded 8-way
 * checkerboard approach. It produces a scalar field of a specified type. As
 * each point is injected, it "splats" or distributes values to nearby
 * voxels. Data is distributed using an elliptical, Gaussian distribution
 * function. The distribution function is modified using scalar values
 * (expands distribution) or normals (creates ellipsoidal distribution rather
 * than spherical). This algorithm is designed for scalability through
 * multithreading.
 *
 * In general, the Gaussian distribution function f(x) around a given
 * splat point p is given by
 *
 *     f(x) = ScaleFactor * exp( ExponentFactor*((r/Radius)**2) )
 *
 * where x is the current voxel sample point; r is the distance |x-p|
 * ExponentFactor <= 0.0, and ScaleFactor can be multiplied by the scalar
 * value of the point p that is currently being splatted.
 *
 * If point normals are present (and NormalWarping is on), then the splat
 * function becomes elliptical (as compared to the spherical one described
 * by the previous equation). The Gaussian distribution function then
 * becomes:
 *
 *     f(x) = ScaleFactor *
 *               exp( ExponentFactor*( ((rxy/E)**2 + z**2)/R**2) )
 *
 * where E is a user-defined eccentricity factor that controls the elliptical
 * shape of the splat; z is the distance of the current voxel sample point
 * along normal N; and rxy is the distance of x in the direction
 * prependicular to N.
 *
 * This class is typically used to convert point-valued distributions into
 * a volume representation. The volume is then usually iso-surfaced or
 * volume rendered to generate a visualization. It can be used to create
 * surfaces from point distributions, or to create structure (i.e.,
 * topology) when none exists.
 *
 * This class makes use of vtkSMPTools to implement a parallel, shared-memory
 * implementation. Hence performance will be significantly improved if VTK is
 * built with VTK_SMP_IMPLEMENTATION_TYPE set to something other than
 * "Sequential" (typically TBB). For example, on a standard laptop with four
 * threads it is common to see a >10x speedup as compared to the serial
 * version of vtkGaussianSplatter.
 *
 * In summary, the algorithm operates by dividing the volume into a 3D
 * checkerboard, where the squares of the checkerboard overlay voxels in the
 * volume. The checkerboard overlay is designed as a function of the splat
 * footprint, so that when splatting occurs in a group (or color) of
 * checkerboard squares, the splat operation will not cause write contention
 * as the splatting proceeds in parallel. There are eight colors in this
 * checkerboard (like an octree) and parallel splatting occurs simultaneously
 * in one of the eight colors (e.g., octants). A single splat operation
 * (across the given 3D footprint) may also be parallelized if the splat is
 * large enough.
 *
 * @warning
 * The input to this filter is of type vtkPointSet. Currently only real types
 * (e.g., float, double) are supported as input, but this could easily be
 * extended to other types. The output type is limited to real types as well.
 *
 * @warning
 * Some voxels may never receive a contribution during the splatting process.
 * The final value of these points can be specified with the "NullValue"
 * instance variable. Note that NullValue is also the initial value of the
 * output voxel values and will affect the accumulation process.
 *
 * @warning
 * While this class is very similar to vtkGaussianSplatter, it does produce
 * slightly different output in most cases (due to the way the footprint is
 * computed).
 *
 * @sa
 * vtkShepardMethod vtkGaussianSplatter
*/

#ifndef vtkCheckerboardSplatter_h
#define vtkCheckerboardSplatter_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageAlgorithm.h"

#define VTK_ACCUMULATION_MODE_MIN 0
#define VTK_ACCUMULATION_MODE_MAX 1
#define VTK_ACCUMULATION_MODE_SUM 2

class vtkDoubleArray;
class vtkCompositeDataSet;

class VTKIMAGINGHYBRID_EXPORT vtkCheckerboardSplatter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkCheckerboardSplatter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with dimensions=(50,50,50); automatic computation of
   * bounds; a Footprint of 2; a Radius of 0; an exponent factor of -5; and normal and
   * scalar warping enabled; and Capping enabled.
   */
  static vtkCheckerboardSplatter *New();

  //@{
  /**
   * Set / get the dimensions of the sampling structured point set. Higher
   * values produce better results but may be much slower.
   */
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);
  //@}

  //@{
  /**
   * Set / get the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which
   * the sampling is performed. If any of the (min,max) bounds values are
   * min >= max, then the bounds will be computed automatically from the input
   * data. Otherwise, the user-specified bounds will be used.
   */
  vtkSetVector6Macro(ModelBounds,double);
  vtkGetVectorMacro(ModelBounds,double,6);
  //@}

  //@{
  /**
   * Control the footprint size of the splat in terms of propagation across a
   * voxel neighborhood. The Footprint value simply indicates the number of
   * neigboring voxels in the i-j-k directions to extend the splat. A value
   * of zero means that only the voxel containing the splat point is
   * affected. A value of one means the immediate neighbors touching the
   * affected voxel are affected as well. Larger numbers increase the splat
   * footprint and significantly increase processing time. Note that the
   * footprint is always 3D rectangular.
   */
  vtkSetClampMacro(Footprint,int,0,VTK_INT_MAX);
  vtkGetMacro(Footprint,int);
  //@}

  //@{
  /**
   * Set / get the radius variable that controls the Gaussian exponential
   * function (see equation above). If set to zero, it is automatically set
   * to the radius of the circumsphere bounding a single voxel. (By default,
   * the Radius is set to zero and is automatically computed.)
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Multiply Gaussian splat distribution by this value. If ScalarWarping
   * is on, then the Scalar value will be multiplied by the ScaleFactor
   * times the Gaussian function.
   */
  vtkSetClampMacro(ScaleFactor,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(ScaleFactor,double);
  //@}

  //@{
  /**
   * Set / get the sharpness of decay of the splats. This is the exponent
   * constant in the Gaussian equation described above. Normally this is a
   * negative value.
   */
  vtkSetMacro(ExponentFactor,double);
  vtkGetMacro(ExponentFactor,double);
  //@}

  //@{
  /**
   * Turn on/off the scaling of splats by scalar value.
   */
  vtkSetMacro(ScalarWarping,vtkTypeBool);
  vtkGetMacro(ScalarWarping,vtkTypeBool);
  vtkBooleanMacro(ScalarWarping,vtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the generation of elliptical splats. If normal warping is
   * on, then the input normals affect the distribution of the splat. This
   * boolean is used in combination with the Eccentricity ivar.
   */
  vtkSetMacro(NormalWarping,vtkTypeBool);
  vtkGetMacro(NormalWarping,vtkTypeBool);
  vtkBooleanMacro(NormalWarping,vtkTypeBool);
  //@}

  //@{
  /**
   * Control the shape of elliptical splatting. Eccentricity is the ratio
   * of the major axis (aligned along normal) to the minor (axes) aligned
   * along other two axes. So Eccentricity > 1 creates needles with the
   * long axis in the direction of the normal; Eccentricity<1 creates
   * pancakes perpendicular to the normal vector.
   */
  vtkSetClampMacro(Eccentricity,double,0.001,VTK_DOUBLE_MAX);
  vtkGetMacro(Eccentricity,double);
  //@}

  //@{
  /**
   * Specify the scalar accumulation mode. This mode expresses how scalar
   * values are combined when splats overlap one another. The Max mode acts
   * like a set union operation and is the most commonly used; the Min mode
   * acts like a set intersection, and the sum is just weird (and can
   * potentially cause accumulation overflow in extreme cases). Note that the
   * NullValue must be set consistent with the accumulation operation.
   */
  vtkSetClampMacro(AccumulationMode,int,
                   VTK_ACCUMULATION_MODE_MIN,VTK_ACCUMULATION_MODE_SUM);
  vtkGetMacro(AccumulationMode,int);
  void SetAccumulationModeToMin()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MIN);}
  void SetAccumulationModeToMax()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MAX);}
  void SetAccumulationModeToSum()
    {this->SetAccumulationMode(VTK_ACCUMULATION_MODE_SUM);}
  const char *GetAccumulationModeAsString();
  //@}

  //@{
  /**
   * Set what type of scalar data this source should generate. Only double
   * and float types are supported currently due to precision requirements
   * during accumulation. By default, float scalars are produced.
   */
  vtkSetMacro(OutputScalarType,int);
  vtkGetMacro(OutputScalarType,int);
  void SetOutputScalarTypeToDouble()
    {this->SetOutputScalarType(VTK_DOUBLE);}
  void SetOutputScalarTypeToFloat()
    {this->SetOutputScalarType(VTK_FLOAT);}
  //@}

  //@{
  /**
   * Turn on/off the capping of the outer boundary of the volume
   * to a specified cap value. This can be used to close surfaces
   * (after iso-surfacing) and create other effects.
   */
  vtkSetMacro(Capping,vtkTypeBool);
  vtkGetMacro(Capping,vtkTypeBool);
  vtkBooleanMacro(Capping,vtkTypeBool);
  //@}

  //@{
  /**
   * Specify the cap value to use. (This instance variable only has effect
   * if the ivar Capping is on.)
   */
  vtkSetMacro(CapValue,double);
  vtkGetMacro(CapValue,double);
  //@}

  //@{
  /**
   * Set the Null value for output points not receiving a contribution from
   * the input points. (This is the initial value of the voxel samples, by
   * default it is set to zero.) Note that the value should be consistent
   * with the output dataset type. The NullValue also provides the initial
   * value on which the accumulations process operates.
   */
  vtkSetMacro(NullValue,double);
  vtkGetMacro(NullValue,double);
  //@}

  //@{
  /**
   * Set/Get the maximum dimension of the checkerboard (i.e., the number of
   * squares in any of the i, j, or k directions). This number also impacts
   * the granularity of the parallel threading (since each checker square is
   * processed separaely). Because of the internal addressing, the maximum
   * dimension is limited to 255 (maximum value of an unsigned char).
   */
  vtkSetClampMacro(MaximumDimension,int,0,255);
  vtkGetMacro(MaximumDimension,int);
  //@}

  //@{
  /**
   * Set/get the crossover point expressed in footprint size where the
   * splatting operation is parallelized (through vtkSMPTools). By default
   * the parallel crossover point is for splat footprints of size two or
   * greater (i.e., at footprint=2 then splat is 5x5x5 and parallel splatting
   * occurs). This is really meant for experimental purposes.
   */
  vtkSetClampMacro(ParallelSplatCrossover,int,0,255);
  vtkGetMacro(ParallelSplatCrossover,int);
  //@}

  /**
   * Compute the size of the sample bounding box automatically from the
   * input data. This is an internal helper function.
   */
  void ComputeModelBounds(vtkDataSet *input, vtkImageData *output,
                          vtkInformation *outInfo);

protected:
  vtkCheckerboardSplatter();
  ~vtkCheckerboardSplatter() override {}

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) override;
  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) override;

  int OutputScalarType; //the type of output scalars
  int SampleDimensions[3]; // dimensions of volume to splat into
  double Radius; // Radius factor in the Gaussian exponential function
  int Footprint; // maximum distance splat propagates (in voxels 0->Dim)
  double ExponentFactor; // scale exponent of gaussian function
  double ModelBounds[6]; // bounding box of splatting dimensions
  double Origin[3], Spacing[3]; // output geometry
  vtkTypeBool NormalWarping; // on/off warping of splat via normal
  double Eccentricity;// elliptic distortion due to normals
  vtkTypeBool ScalarWarping; // on/off warping of splat via scalar
  double ScaleFactor; // splat size influenced by scale factor
  vtkTypeBool Capping; // Cap side of volume to close surfaces
  double CapValue; // value to use for capping
  int AccumulationMode; // how to combine scalar values
  double NullValue; // initial value of voxels
  unsigned char MaximumDimension; // max resolution of checkerboard
  int ParallelSplatCrossover; //the point at which parallel splatting occurs

private:
  vtkCheckerboardSplatter(const vtkCheckerboardSplatter&) = delete;
  void operator=(const vtkCheckerboardSplatter&) = delete;
};

#endif
