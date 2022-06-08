#include "vtkVectorBasisLagrangeProducts.h"

namespace
{
using VblpMatrixType = vtkVectorBasisLagrangeProducts::VblpMatrixType;
}

vtkVectorBasisLagrangeProducts::vtkVectorBasisLagrangeProducts()
{
  for (const auto& cellType : { VTK_HEXAHEDRON, VTK_QUAD, VTK_TETRA, VTK_TRIANGLE, VTK_WEDGE })
  {
    switch (cellType)
    {
      case VTK_HEXAHEDRON:
      {
        auto& hcurl_phi0 = this->HexVbf[0][0];
        auto& hcurl_phi1 = this->HexVbf[0][1];
        auto& hcurl_phi2 = this->HexVbf[0][2];
        // clang-format off
        hcurl_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            (1.0 - y)*(1.0 - z)/4.0,
            0.0,
            -(1.0 + y)*(1.0 - z)/4.0,
            0.0,
            (1.0 - y)*(1.0 + z)/4.0,
            0.0,
            -(1.0 + y)*(1.0 + z)/4.0,
            0.0,
            0.0,
            0.0,
            0.0,
            0.0
          });
        };
        hcurl_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            (1.0 + x)*(1.0 - z)/4.0,
            0.0,
            -(1.0 - x)*(1.0 - z)/4.0,
            0.0,
            (1.0 + x)*(1.0 + z)/4.0,
            0.0,
            -(1.0 - x)*(1.0 + z)/4.0,
            0.0,
            0.0,
            0.0,
            0.0
          });
        };
        hcurl_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0,
            0.0,
            0.0,
            0.0,
            0.0,
            0.0,
            (1.0 - x)*(1.0 - y)/4.0,
            (1.0 + x)*(1.0 - y)/4.0,
            (1.0 + x)*(1.0 + y)/4.0,
            (1.0 - x)*(1.0 + y)/4.0
          });
        };
        // clang-format on
        auto& hdiv_phi0 = this->HexVbf[1][0];
        auto& hdiv_phi1 = this->HexVbf[1][1];
        auto& hdiv_phi2 = this->HexVbf[1][2];
        // clang-format off
        hdiv_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            (1.0 + x)/2.0,
            0.0,
            (x - 1.0)/2.0,
            0.0,
            0.0
          });
        };
        hdiv_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            (y - 1.0)/2.0,
            0.0,
            (1.0 + y)/2.0,
            0.0,
            0.0,
            0.0
          });
        };
        hdiv_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0,
            0.0,
            (z - 1.0)/2.0,
            (1.0 + z)/2.0
          });
        };
        // clang-format on
        break;
      }
      case VTK_QUAD:
      {
        auto& hcurl_phi0 = this->QuadVbf[0][0];
        auto& hcurl_phi1 = this->QuadVbf[0][1];
        auto& hcurl_phi2 = this->QuadVbf[0][2];
        // clang-format off
        hcurl_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.5 * (1. - y),
            0.,
            -0.5 * (1. + y),
            0.
          });
        };
        hcurl_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0,
            0.5 * (1. + x),
            0,
            -0.5 * (1. - x)
          });
        };
        hcurl_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0,
            0.0
          });
        };
        // clang-format on
        auto& hdiv_phi0 = this->QuadVbf[1][0];
        auto& hdiv_phi1 = this->QuadVbf[1][1];
        auto& hdiv_phi2 = this->QuadVbf[1][2];
        // clang-format off
        hdiv_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.5*(1.0 + x),
            0.0,
            0.5*(x - 1.0)
          });
        };
        hdiv_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.5*(y - 1.0),
            0.0,
            0.5*(1.0 + y),
            0.0
          });
        };
        hdiv_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0,
            0.0
          });
        };
        // clang-format on
        break;
      }
      case VTK_TETRA:
      {
        auto& hcurl_phi0 = this->TetVbf[0][0];
        auto& hcurl_phi1 = this->TetVbf[0][1];
        auto& hcurl_phi2 = this->TetVbf[0][2];
        // clang-format off
        hcurl_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * (1.0 - y - z),
            -2.0 * y,
            -2.0 * y,
            2.0 * z,
            -2.0 * z,
            0.0
          });
        };
        hcurl_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * x,
            2.0 * x,
            2.0 * (-1.0 + x + z),
            2.0 * z,
            0.0,
            -2.0 * z
          });
        };
        hcurl_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * x,
            0.0,
            -2.0 * y,
            2.0 * (1.0 - x - y),
            2.0 * x,
            2.0 * y
          });
        };
        // clang-format on
        auto& hdiv_phi0 = this->TetVbf[1][0];
        auto& hdiv_phi1 = this->TetVbf[1][1];
        auto& hdiv_phi2 = this->TetVbf[1][2];
        // clang-format off
        hdiv_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            x,
            x,
            x - 1.0,
            x
          });
        };
        hdiv_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            y - 1.0,
            y,
            y,
            y
          });
        };
        hdiv_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            z,
            z,
            z,
            z - 1.0
          });
        };
        // clang-format on
        break;
      }
      case VTK_TRIANGLE:
      {
        auto& hcurl_phi0 = this->TriVbf[0][0];
        auto& hcurl_phi1 = this->TriVbf[0][1];
        auto& hcurl_phi2 = this->TriVbf[0][2];
        // clang-format off
        hcurl_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * (1. - y),
            -2.0 * y,
            -2.0 * y
          });
        };
        hcurl_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * x,
            2.0 * x,
            2.0 * (-1. + x)
          });
        };
        hcurl_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0
          });
        };
        // clang-format on
        auto& hdiv_phi0 = this->TriVbf[1][0];
        auto& hdiv_phi1 = this->TriVbf[1][1];
        auto& hdiv_phi2 = this->TriVbf[1][2];
        // clang-format off
        hdiv_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * x,
            2.0 * x,
            2.0 * (x - 1.0)
          });
        };
        hdiv_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * (y - 1.0),
            2.0 * y,
            2.0 * y
          });
        };
        hdiv_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0
          });
        };
        // clang-format on
        break;
      }
      case VTK_WEDGE:
      {
        auto& hcurl_phi0 = this->WedgeVbf[0][0];
        auto& hcurl_phi1 = this->WedgeVbf[0][1];
        auto& hcurl_phi2 = this->WedgeVbf[0][2];
        // clang-format off
          hcurl_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
            return std::vector<double>({
              (1.0 - z)*(1.0 - y),
              y*(z - 1.0),
              y*(z - 1.0),
              (1.0 - y)*(1.0 + z),
              -y*(1.0 + z),
              -y*(1.0 + z),
              0.0,
              0.0,
              0.0
            });
          };
          hcurl_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
            return std::vector<double>({
              x*(1.0 - z),
              x*(1.0 - z),
              (1.0 - x)*(z - 1.0),
              x*(1.0 + z),
              x*(1.0 + z),
              (x - 1.0)*(1.0 + z),
              0.0,
              0.0,
              0.0
            });
          };
          hcurl_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
            return std::vector<double>({
              0.0,
              0.0,
              0.0,
              0.0,
              0.0,
              0.0,
              (1.0 - x - y),
              x,
              y
            });
          };
        // clang-format on
        auto& hdiv_phi0 = this->WedgeVbf[1][0];
        auto& hdiv_phi1 = this->WedgeVbf[1][1];
        auto& hdiv_phi2 = this->WedgeVbf[1][2];
        // clang-format off
        hdiv_phi0 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * x,
            2.0 * x,
            2.0 * (x - 1.0),
            0.0,
            0.0
          });
        };
        hdiv_phi1 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            2.0 * (y - 1.0),
            2.0 * y,
            2.0 * y,
            0.0,
            0.0
          });
        };
        hdiv_phi2 = [](const double& x, const double& y, const double& z) -> std::vector<double> {
          return std::vector<double>({
            0.0,
            0.0,
            0.0,
            (z - 1.0) / 2.0,
            (1.0 + z) / 2.0
          });
        };
        // clang-format on
        break;
      }
      default:
        break;
    }
  }
}

void vtkVectorBasisLagrangeProducts::Initialize(
  const VTKCellType& cell, const double* coords, const int& npts)
{
  auto hcurl_mats = this->GetVblp(SpaceType::HCurl, cell);
  auto hdiv_mats = this->GetVblp(SpaceType::HDiv, cell);

  const auto hcurl_vbfs = this->GetVbFunctions(SpaceType::HCurl, cell);
  const auto hdiv_vbfs = this->GetVbFunctions(SpaceType::HDiv, cell);

  if (hcurl_mats == nullptr || hdiv_mats == nullptr || hcurl_vbfs == nullptr ||
    hdiv_vbfs == nullptr)
  {
    return;
  }

  hcurl_mats->clear();
  hdiv_mats->clear();
  hcurl_mats->resize(3);
  hdiv_mats->resize(3);

  for (int k = 0; k < 3; ++k)
  {
    auto& hcurl_mat = (*hcurl_mats)[k];
    auto& hdiv_mat = (*hdiv_mats)[k];
    auto& hcurl_vbf = (*hcurl_vbfs)[k];
    auto& hdiv_vbf = (*hdiv_vbfs)[k];
    for (int j = 0; j < npts; ++j)
    {
      const double& x = coords[j * 3];
      const double& y = coords[j * 3 + 1];
      const double& z = coords[j * 3 + 2];
      hcurl_mat.emplace_back(hcurl_vbf(x, y, z));
      hdiv_mat.emplace_back(hdiv_vbf(x, y, z));
    }
  }
}

bool vtkVectorBasisLagrangeProducts::RequiresInitialization(
  const VTKCellType& cell, const double* coords, const int& npts)
{
  auto hcurl_mats = this->GetVblp(SpaceType::HCurl, cell);
  auto hdiv_mats = this->GetVblp(SpaceType::HDiv, cell);
  if (hcurl_mats == nullptr || hdiv_mats == nullptr)
  {
    return false;
  }

  if (hcurl_mats->size() == 3 && hdiv_mats->size() == 3)
  {
    bool reqInit = false;
    for (std::size_t i = 0; i < 3; ++i)
    {
      reqInit |= (*hcurl_mats)[i].size() != npts;
      reqInit |= (*hdiv_mats)[i].size() != npts;
    }
    return reqInit;
  }
  else
  {
    return true;
  }
}

void vtkVectorBasisLagrangeProducts::Clear(const VTKCellType& cell)
{
  auto hcurl_mats = this->GetVblp(SpaceType::HCurl, cell);
  auto hdiv_mats = this->GetVblp(SpaceType::HDiv, cell);
  if (hcurl_mats == nullptr || hdiv_mats == nullptr)
  {
    return;
  }
  hcurl_mats->clear();
  hdiv_mats->clear();
}

::VblpMatrixType* vtkVectorBasisLagrangeProducts::GetVblp(
  const SpaceType& space, const VTKCellType& cell)
{
  ::VblpMatrixType* mats = nullptr;
  switch (cell)
  {
    case VTK_HEXAHEDRON:
      mats = &(this->HexVblpMats[static_cast<int>(space)]);
      break;
    case VTK_QUAD:
      mats = &(this->QuadVblpMats[static_cast<int>(space)]);
      break;
    case VTK_TETRA:
      mats = &(this->TetVblpMats[static_cast<int>(space)]);
      break;
    case VTK_TRIANGLE:
      mats = &(this->TriVblpMats[static_cast<int>(space)]);
      break;
    case VTK_WEDGE:
      mats = &(this->WedgeVblpMats[static_cast<int>(space)]);
      break;
    default:
      break;
  }
  return mats;
}

vtkVectorBasisLagrangeProducts::VbfuncType* vtkVectorBasisLagrangeProducts::GetVbFunctions(
  const SpaceType& space, const VTKCellType& cell)
{
  const int space_int = static_cast<int>(space);
  switch (cell)
  {
    case VTK_HEXAHEDRON:
      return &this->HexVbf[space_int];
    case VTK_QUAD:
      return &this->QuadVbf[space_int];
    case VTK_TETRA:
      return &this->TetVbf[space_int];
    case VTK_TRIANGLE:
      return &this->TriVbf[space_int];
    case VTK_WEDGE:
      return &this->WedgeVbf[space_int];
    default:
      break;
  }
  return nullptr;
}
