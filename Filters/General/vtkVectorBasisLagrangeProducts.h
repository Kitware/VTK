/*
 * Vector Basis Lagrange Product Matrices
 * Generated from
 * [intrepid2](https://github.com/trilinos/Trilinos/tree/master/packages/intrepid2/src/Discretization/Basis)
 *
 */

#ifndef vtkVectorBasisLagrangeProducts_h
#define vtkVectorBasisLagrangeProducts_h

#include "vtkCellType.h"
#include "vtkFiltersGeneralModule.h"

#include <functional>
#include <vector>

class VTKFILTERSGENERAL_EXPORT vtkVectorBasisLagrangeProducts
{
public:
  vtkVectorBasisLagrangeProducts();
  ~vtkVectorBasisLagrangeProducts() = default;

  enum class SpaceType : int
  {
    HCurl = 0,
    HDiv
  };
  using VblpMatrixType = std::vector<std::vector<std::vector<double>>>;
  using VbfuncType =
    std::function<std::vector<double>(const double& x, const double& y, const double& z)>[3];

  void Initialize(const VTKCellType& cell, const double* coords, const int& npts);
  bool RequiresInitialization(const VTKCellType& cell, const double* coords, const int& npts);
  void Clear(const VTKCellType& cell);
  VblpMatrixType* GetVblp(const SpaceType& space, const VTKCellType& cell);
  VbfuncType* GetVbFunctions(const SpaceType& space, const VTKCellType& cell);

private:
  VblpMatrixType HexVblpMats[2], QuadVblpMats[2], TetVblpMats[2], TriVblpMats[2], WedgeVblpMats[2];
  VbfuncType HexVbf[2], QuadVbf[2], TetVbf[2], TriVbf[2], WedgeVbf[2];
};

#endif
// VTK-HeaderTest-Exclude: vtkVectorBasisLagrangeProducts.h
