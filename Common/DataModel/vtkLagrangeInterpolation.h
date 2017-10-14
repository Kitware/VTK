#ifndef vtkLagrangeInterpolation_h
#define vtkLagrangeInterpolation_h

#include "vtkObject.h"
#include "vtkSmartPointer.h" // For API.
#include "vtkCommonDataModelModule.h" // For export macro.

#include <vector> // For scratch storage.

// Define this to include support for a "complete" (21- vs 18-point) wedge.
#define VTK_21_POINT_WEDGE true

class vtkPoints;
class vtkVector2i;
class vtkVector3d;

class VTKCOMMONDATAMODEL_EXPORT vtkLagrangeInterpolation : public vtkObject
{
public:
  static vtkLagrangeInterpolation* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkLagrangeInterpolation,vtkObject);

  enum Constants {
    MaxDegree = 10 // The maximum degree that VTK will support.
  };

  static void EvaluateShapeFunctions(int order, double pcoord, double* shape);
  static void EvaluateShapeAndGradient(int order, double pcoord, double* shape, double* grad);

  static int Tensor1ShapeFunctions(const int order[1], const double* pcoords, double* shape);
  static int Tensor1ShapeDerivatives(const int order[1], const double* pcoords, double* derivs);

  static int Tensor2ShapeFunctions(const int order[2], const double* pcoords, double* shape);
  static int Tensor2ShapeDerivatives(const int order[2], const double* pcoords, double* derivs);

  static int Tensor3ShapeFunctions(const int order[3], const double* pcoords, double* shape);
  static int Tensor3ShapeDerivatives(const int order[3], const double* pcoords, double* derivs);

  void Tensor3EvaluateDerivative(
    const int order[4],
    const double* pcoords,
    double* fieldVals,
    int fieldDim,
    double* fieldDerivs);

  static void WedgeShapeFunctions(const int order[4], const double* pcoords, double* shape);
  static void WedgeShapeDerivatives(const int order[4], const double* pcoords, double* derivs);

  void WedgeEvaluate(
    const int order[4],
    const double* pcoords,
    double* fieldVals,
    int fieldDim,
    double* fieldAtPCoords);

  void WedgeEvaluateDerivative(
    const int order[4],
    const double* pcoords,
    double* fieldVals,
    int fieldDim,
    double* fieldDerivs);

  static vtkVector3d GetParametricHexCoordinates(int vertexId);
  static vtkVector2i GetPointIndicesBoundingHexEdge(int edgeId);
  static int GetVaryingParameterOfHexEdge(int edgeId);
  static vtkVector2i GetFixedParametersOfHexEdge(int edgeId);

  static const int* GetPointIndicesBoundingHexFace(int faceId);
  static const int* GetEdgeIndicesBoundingHexFace(int faceId);
  static vtkVector2i GetVaryingParametersOfHexFace(int faceId);
  static int GetFixedParameterOfHexFace(int faceId);

  static vtkVector3d GetParametricWedgeCoordinates(int vertexId);
  static vtkVector2i GetPointIndicesBoundingWedgeEdge(int edgeId);
  static int GetVaryingParameterOfWedgeEdge(int edgeId);
  static vtkVector2i GetFixedParametersOfWedgeEdge(int edgeId);

  static const int* GetPointIndicesBoundingWedgeFace(int faceId);
  static const int* GetEdgeIndicesBoundingWedgeFace(int faceId);
  static vtkVector2i GetVaryingParametersOfWedgeFace(int faceId);
  static int GetFixedParameterOfWedgeFace(int faceId);

  static void AppendCurveCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[1]);
  static void AppendQuadrilateralCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[2]);
  static void AppendHexahedronCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[3]);
  static void AppendWedgeCollocationPoints(vtkSmartPointer<vtkPoints>& pts, const int order[3]);

  template<int N>
  static int NumberOfIntervals(const int order[N]);
protected:
  vtkLagrangeInterpolation();
  ~vtkLagrangeInterpolation() override;

  void PrepareForOrder(const int o[4]);

  std::vector<double> ShapeSpace;
  std::vector<double> DerivSpace;

private:
  vtkLagrangeInterpolation(const vtkLagrangeInterpolation&) = delete;
  void operator=(const vtkLagrangeInterpolation&) = delete;
};

template<int N>
int vtkLagrangeInterpolation::NumberOfIntervals(const int order[N])
{
  int ni = 1;
  for (int n = 0; n < N; ++n)
    {
    ni *= order[n];
    }
  return ni;
}

#endif // vtkLagrangeInterpolation_h
