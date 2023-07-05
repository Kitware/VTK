// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSampleFunction
 * @brief   sample an implicit function over a structured point set
 *
 * vtkSampleFunction is a source object that evaluates an implicit function
 * and normals at each point in a vtkStructuredPoints. The user can specify
 * the sample dimensions and location in space to perform the sampling. To
 * create closed surfaces (in conjunction with the vtkContourFilter), capping
 * can be turned on to set a particular value on the boundaries of the sample
 * space.
 *
 * @sa
 * vtkImplicitModeller
 */

#ifndef vtkSampleFunction_h
#define vtkSampleFunction_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingHybridModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;
class vtkDataArray;

class VTKIMAGINGHYBRID_EXPORT vtkSampleFunction : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSampleFunction, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
   * Capping turned off, and normal generation on.
   */
  static vtkSampleFunction* New();

  ///@{
  /**
   * Specify the implicit function to use to generate data.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Set what type of scalar data this source should generate.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(VTK_DOUBLE); }
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(VTK_FLOAT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(VTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(VTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(VTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(VTK_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  ///@}

  /**
   * Specify the dimensions of the data on which to sample.
   */
  void SetSampleDimensions(int i, int j, int k);

  ///@{
  /**
   * Specify the dimensions of the data on which to sample.
   */
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions, int, 3);
  ///@}

  ///@{
  /**
   * Specify the region in space over which the sampling occurs. The
   * bounds is specified as (xMin,xMax, yMin,yMax, zMin,zMax).
   */
  void SetModelBounds(const double bounds[6]);
  void SetModelBounds(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  vtkGetVectorMacro(ModelBounds, double, 6);
  ///@}

  ///@{
  /**
   * Turn on/off capping. If capping is on, then the outer boundaries of the
   * structured point set are set to cap value. This can be used to ensure
   * surfaces are closed.
   */
  vtkSetMacro(Capping, vtkTypeBool);
  vtkGetMacro(Capping, vtkTypeBool);
  vtkBooleanMacro(Capping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the cap value.
   */
  vtkSetMacro(CapValue, double);
  vtkGetMacro(CapValue, double);
  ///@}

  ///@{
  /**
   * Turn on/off the computation of normals (normals are float values).
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the scalar array name for this data set. Initial value is
   * "scalars".
   */
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);
  ///@}

  ///@{
  /**
   * Set/get the normal array name for this data set. Initial value is
   * "normals".
   */
  vtkSetStringMacro(NormalArrayName);
  vtkGetStringMacro(NormalArrayName);
  ///@}

  /**
   * Return the MTime also considering the implicit function.
   */
  vtkMTimeType GetMTime() override;

protected:
  /**
   * Default constructor.
   * Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
   * Capping turned off, CapValue=VTK_DOUBLE_MAX, normal generation on,
   * OutputScalarType set to VTK_DOUBLE, ImplicitFunction set to nullptr,
   * ScalarArrayName is "" and NormalArrayName is "".
   */
  vtkSampleFunction();

  ~vtkSampleFunction() override;

  void ReportReferences(vtkGarbageCollector*) override;

  void ExecuteDataWithInformation(vtkDataObject*, vtkInformation*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void Cap(vtkDataArray* s);

  int OutputScalarType;
  int SampleDimensions[3];
  double ModelBounds[6];
  vtkTypeBool Capping;
  double CapValue;
  vtkImplicitFunction* ImplicitFunction;
  vtkTypeBool ComputeNormals;
  char* ScalarArrayName;
  char* NormalArrayName;

private:
  vtkSampleFunction(const vtkSampleFunction&) = delete;
  void operator=(const vtkSampleFunction&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
