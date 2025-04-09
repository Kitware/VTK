// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadratureSchemeDefinition
 *
 * An Elemental data type that holds a definition of a
 * numerical quadrature scheme. The definition contains
 * the requisite information to interpolate to the so called
 * quadrature points of the specific scheme. namely:
 *
 * <pre>
 * 1)
 * A matrix of shape function weights(shape functions evaluated
 * at parametric coordinates of the quadrature points).
 *
 * 2)
 * The number of quadrature points and cell nodes. These parameters
 * size the matrix, and allow for convenient evaluation by users
 * of the definition.
 * </pre>
 */

#ifndef vtkQuadratureSchemeDefinition_h
#define vtkQuadratureSchemeDefinition_h

#include "vtkCellType.h"              // For VTK_EMPTY_CELL
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationQuadratureSchemeDefinitionVectorKey;
class vtkInformationStringKey;
class vtkXMLDataElement;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadratureSchemeDefinition : public vtkObject
{
public:
  // vtk stuff
  vtkTypeMacro(vtkQuadratureSchemeDefinition, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkInformationQuadratureSchemeDefinitionVectorKey* DICTIONARY();
  static vtkInformationStringKey* QUADRATURE_OFFSET_ARRAY_NAME();

  /**
   * New object in an unusable state. You'll have to call
   * "Initialize" to get the definition in to a usable state.
   */
  static vtkQuadratureSchemeDefinition* New();

  /**
   * Deep copy.
   */
  int DeepCopy(const vtkQuadratureSchemeDefinition* other);

  /**
   * Put the object into an XML representation. The element
   * passed in is assumed to be empty.
   */
  int SaveState(vtkXMLDataElement* root);

  /**
   * Restore the object from an XML representation.
   */
  int RestoreState(vtkXMLDataElement* root);

  /**
   * Initialize the object allocating resources as needed.
   */
  void Initialize(
    int cellType, int numberOfNodes, int numberOfQuadraturePoints, double* shapeFunctionWeights);

  /**
   * Initialize the object allocating resources as needed.
   */
  void Initialize(int cellType, int numberOfNodes, int numberOfQuadraturePoints,
    double* shapeFunctionWeights, double* quadratureWeights);

  /**
   * Initialize the object allocating resources as needed.
   */
  void Initialize(int cellType, int numberOfNodes, int numberOfQuadraturePoints,
    const double* shapeFunctionWeights, const double* quadratureWeights, int dim,
    const double* shapeFunctionDerivativeWeights);

  /**
   * Access the VTK cell type id.
   */
  int GetCellType() const { return this->CellType; }

  /**
   * Access to an alternative key.
   */
  int GetQuadratureKey() const { return this->QuadratureKey; }

  /**
   * Get the number of nodes associated with the interpolation.
   */
  int GetNumberOfNodes() const { return this->NumberOfNodes; }

  /**
   * Get the number of quadrature points associated with the scheme.
   */
  int GetNumberOfQuadraturePoints() const { return this->NumberOfQuadraturePoints; }

  /**
   * Get the dimension of the reference element.
   */
  vtkGetMacro(Dimension, int);

  /**
   * Get the array of shape function weights. Shape function weights are
   * the shape functions evaluated at the quadrature points. There are
   * "NumberOfNodes" weights for each quadrature point.
   */
  const double* GetShapeFunctionWeights() const { return this->ShapeFunctionWeights; }

  ///@{
  /**
   * Get the array of shape function weights associated with a
   * single quadrature point.
   */
  const double* GetShapeFunctionWeights(int quadraturePointId) const
  {
    int idx = quadraturePointId * this->NumberOfNodes;
    return this->ShapeFunctionWeights + idx;
  }

  /**
   * Get the array of shape function derivative weights associated with a
   * single quadrature point.
   */
  const double* GetShapeFunctionDerivativeWeights(int quadraturePointId) const
  {
    int idx = quadraturePointId * this->NumberOfNodes * this->Dimension;
    return this->ShapeFunctionDerivativeWeights + idx;
  }
  ///@}

  /**
   * Access to the quadrature weights.
   */
  const double* GetQuadratureWeights() const { return this->QuadratureWeights; }

protected:
  vtkQuadratureSchemeDefinition();
  ~vtkQuadratureSchemeDefinition() override;

private:
  /**
   * Allocate/De-allocate resources that will be used by the definition.
   * This must be called after Set*. Caller's responsibility.
   */
  void ReleaseResources();

  /**
   * Allocate resources according to the objects
   * current internal state.
   */
  int SecureResources();

  /**
   * Initialize the shape function weights definition.
   * Must call SecureResources prior.
   */
  void SetShapeFunctionWeights(const double* weights);

  /**
   * Initialize the shape function weights definition.
   * Must call SecureResources prior.
   */
  void SetQuadratureWeights(const double* weights);

  /**
   * Initialize the shape function derivative weights definition.
   * Must call SecureResources prior.
   */
  void SetShapeFunctionDerivativeWeights(const double* weights);

  //
  vtkQuadratureSchemeDefinition(const vtkQuadratureSchemeDefinition&) = delete;
  void operator=(const vtkQuadratureSchemeDefinition&) = delete;
  friend ostream& operator<<(ostream& s, const vtkQuadratureSchemeDefinition& d);
  friend istream& operator>>(istream& s, vtkQuadratureSchemeDefinition& d);
  //
  int CellType = VTK_EMPTY_CELL;
  int QuadratureKey = -1;
  int NumberOfNodes = 0;
  int NumberOfQuadraturePoints = 0;
  int Dimension = 0;
  double* ShapeFunctionWeights = nullptr;
  double* QuadratureWeights = nullptr;
  double* ShapeFunctionDerivativeWeights = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
