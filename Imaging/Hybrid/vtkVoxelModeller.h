/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelModeller.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVoxelModeller
 * @brief   convert an arbitrary dataset to a voxel representation
 *
 * vtkVoxelModeller is a filter that converts an arbitrary data set to a
 * structured point (i.e., voxel) representation. It is very similar to
 * vtkImplicitModeller, except that it doesn't record distance; instead it
 * records occupancy. By default it supports a compact output of 0/1
 * VTK_BIT. Other vtk scalar types can be specified. The Foreground and
 * Background values of the output can also be specified.
 * NOTE: Not all vtk filters/readers/writers support the VTK_BIT
 * scalar type. You may want to use VTK_CHAR as an alternative.
 * @sa
 * vtkImplicitModeller
*/

#ifndef vtkVoxelModeller_h
#define vtkVoxelModeller_h

#include "vtkImagingHybridModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGHYBRID_EXPORT vtkVoxelModeller : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkVoxelModeller,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct an instance of vtkVoxelModeller with its sample dimensions
   * set to (50,50,50), and so that the model bounds are
   * automatically computed from its input. The maximum distance is set to
   * examine the whole grid. This could be made much faster, and probably
   * will be in the future.
   */
  static vtkVoxelModeller *New();

  /**
   * Compute the ModelBounds based on the input geometry.
   */
  double ComputeModelBounds(double origin[3], double ar[3]);

  //@{
  /**
   * Set the i-j-k dimensions on which to sample the distance function.
   * Default is (50, 50, 50)
   */
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);
  //@}

  //@{
  /**
   * Specify distance away from surface of input geometry to sample. Smaller
   * values make large increases in performance. Default is 1.0.
   */
  vtkSetClampMacro(MaximumDistance,double,0.0,1.0);
  vtkGetMacro(MaximumDistance,double);
  //@}

  //@{
  /**
   * Specify the position in space to perform the voxelization.
   * Default is (0, 0, 0, 0, 0, 0)
   */
  void SetModelBounds(const double bounds[6]);
  void SetModelBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
  vtkGetVectorMacro(ModelBounds,double,6);
  //@}

  //@{
  /**
   * Control the scalar type of the output image. The default is
   * VTK_BIT.
   * NOTE: Not all filters/readers/writers support the VTK_BIT
   * scalar type. You may want to use VTK_CHAR as an alternative.
   */
  vtkSetMacro(ScalarType,int);
  void SetScalarTypeToFloat(){this->SetScalarType(VTK_FLOAT);};
  void SetScalarTypeToDouble(){this->SetScalarType(VTK_DOUBLE);};
  void SetScalarTypeToInt(){this->SetScalarType(VTK_INT);};
  void SetScalarTypeToUnsignedInt()
    {this->SetScalarType(VTK_UNSIGNED_INT);};
  void SetScalarTypeToLong(){this->SetScalarType(VTK_LONG);};
  void SetScalarTypeToUnsignedLong()
    {this->SetScalarType(VTK_UNSIGNED_LONG);};
  void SetScalarTypeToShort(){this->SetScalarType(VTK_SHORT);};
  void SetScalarTypeToUnsignedShort()
    {this->SetScalarType(VTK_UNSIGNED_SHORT);};
  void SetScalarTypeToUnsignedChar()
    {this->SetScalarType(VTK_UNSIGNED_CHAR);};
  void SetScalarTypeToChar()
    {this->SetScalarType(VTK_CHAR);};
  void SetScalarTypeToBit()
    {this->SetScalarType(VTK_BIT);};
  vtkGetMacro(ScalarType,int);
  //@}

  //@{
  /**
   * Set the Foreground/Background values of the output. The
   * Foreground value is set when a voxel is occupied. The Background
   * value is set when a voxel is not occupied.
   * The default ForegroundValue is 1. The default BackgroundValue is
   * 0.
   */
  vtkSetMacro(ForegroundValue, double);
  vtkGetMacro(ForegroundValue, double);
  vtkSetMacro(BackgroundValue, double);
  vtkGetMacro(BackgroundValue, double);
  //@}

protected:
  vtkVoxelModeller();
  ~vtkVoxelModeller() VTK_OVERRIDE {}

  int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) VTK_OVERRIDE;

  // see vtkAlgorithm for details
  int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int SampleDimensions[3];
  double MaximumDistance;
  double ModelBounds[6];
  double ForegroundValue;
  double BackgroundValue;
  int ScalarType;

private:
  vtkVoxelModeller(const vtkVoxelModeller&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVoxelModeller&) VTK_DELETE_FUNCTION;
};

#endif
