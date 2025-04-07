#include <algorithm>
#include <cassert>
#include <vector>

namespace
{
struct O2MRelation
{
  std::vector<unsigned int> Connectivity;
  std::vector<unsigned int> Sizes;
  std::vector<unsigned int> Offsets;

  size_t GetNumberOfElements() const { return this->Sizes.size(); }

  unsigned int AddElement(const unsigned int* ptIds, const std::vector<unsigned int>& lids)
  {
    auto index = this->Sizes.size();
    assert(this->Sizes.size() == this->Offsets.size());

    this->Sizes.push_back(static_cast<unsigned int>(lids.size()));
    this->Offsets.push_back(static_cast<unsigned int>(this->Connectivity.size()));

    this->Connectivity.resize(this->Connectivity.size() + lids.size());
    std::transform(lids.begin(), lids.end(),
      std::next(this->Connectivity.begin(), this->Offsets.back()),
      [&ptIds](unsigned int idx) { return ptIds ? ptIds[idx] : idx; });

    return index;
  }
};

class Grid
{
public:
  Grid();
  void Initialize(const unsigned int numPoints[3], const double spacing[3]);

  size_t GetNumberOfPoints() const;
  size_t GetNumberOfCells() const;

  const double* GetPoint(size_t id) const;
  std::vector<double>& GetPoints() { return this->Points; }
  const O2MRelation& GetPolyhedralCells() const { return this->PolyhedralCells; }
  const O2MRelation& GetPolygonalFaces() const { return this->PolygonalFaces; }

private:
  std::vector<double> Points;
  O2MRelation PolyhedralCells;
  O2MRelation PolygonalFaces;

  void AppendHex(const unsigned int pointIds[8]);
};

class Attributes
{
  // A class for generating and storing point and cell fields.
  // Velocity is stored at the points and pressure is stored
  // for the cells. The current velocity profile is for a
  // shearing flow with U(y,t) = y*t, V = 0 and W = 0.
  // Pressure is constant through the domain.
public:
  Attributes();
  void Initialize(Grid* grid);
  void UpdateFields(double time);
  std::vector<double>& GetVelocityArray();
  std::vector<float>& GetPressureArray();

private:
  std::vector<double> Velocity;
  std::vector<float> Pressure;
  Grid* GridPtr;
};

Grid::Grid() = default;

void Grid::Initialize(const unsigned int numPoints[3], const double spacing[3])
{
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }

  this->Points.clear();

  // in parallel, we do a simple partitioning in the x-direction.
  int mpiSize = 1;
  int mpiRank = 0;
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

  unsigned int startXPoint = mpiRank * numPoints[0] / mpiSize;
  unsigned int endXPoint = (mpiRank + 1) * numPoints[0] / mpiSize;
  if (mpiSize != mpiRank + 1)
  {
    endXPoint++;
  }

  // create the points -- slowest in the x and fastest in the z directions
  double coord[3] = { 0, 0, 0 };
  for (unsigned int i = startXPoint; i < endXPoint; i++)
  {
    coord[0] = i * spacing[0];
    for (unsigned int j = 0; j < numPoints[1]; j++)
    {
      coord[1] = j * spacing[1];
      for (unsigned int k = 0; k < numPoints[2]; k++)
      {
        coord[2] = k * spacing[2];
        // add the coordinate to the end of the vector
        std::copy(coord, coord + 3, std::back_inserter(this->Points));
      }
    }
  }
  // create the hex cells
  unsigned int cellPoints[8];
  unsigned int numXPoints = endXPoint - startXPoint;
  for (unsigned int i = 0; i < numXPoints - 1; i++)
  {
    for (unsigned int j = 0; j < numPoints[1] - 1; j++)
    {
      for (unsigned int k = 0; k < numPoints[2] - 1; k++)
      {
        cellPoints[0] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[1] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[2] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[3] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[4] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[5] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[6] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        cellPoints[7] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;

        this->AppendHex(cellPoints);
      }
    }
  }
}

void Grid::AppendHex(const unsigned int pointIds[8])
{
  // add a hex as a polyhedral cell, i.e. a cell with 6 quads

  // add the quads; since I couldn't confirm how the face normal should point,
  // I am making them all point outward.
  std::vector<unsigned int> faces;
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 0, 3, 2, 1 })); // bottom
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 0, 1, 5, 4 }));
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 1, 2, 6, 5 }));
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 2, 3, 7, 6 }));
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 3, 0, 4, 7 }));
  faces.push_back(this->PolygonalFaces.AddElement(pointIds, { 4, 5, 6, 7 })); // top

  this->PolyhedralCells.AddElement(nullptr, faces);
}

size_t Grid::GetNumberOfPoints() const
{
  return this->Points.size() / 3;
}
size_t Grid::GetNumberOfCells() const
{
  return this->PolyhedralCells.GetNumberOfElements();
}

const double* Grid::GetPoint(size_t pointId) const
{
  if (pointId >= this->Points.size())
  {
    return nullptr;
  }
  return &(this->Points[pointId * 3]);
}

Attributes::Attributes()
{
  this->GridPtr = nullptr;
}

void Attributes::Initialize(Grid* grid)
{
  this->GridPtr = grid;
}

void Attributes::UpdateFields(double time)
{
  size_t numPoints = this->GridPtr->GetNumberOfPoints();
  this->Velocity.resize(numPoints * 3);
  for (size_t pt = 0; pt < numPoints; pt++)
  {
    const double* coord = this->GridPtr->GetPoint(pt);
    this->Velocity[pt] = coord[1] * time;
  }
  std::fill(this->Velocity.begin() + numPoints, this->Velocity.end(), 0.);
  size_t numCells = this->GridPtr->GetNumberOfCells();
  this->Pressure.resize(numCells);
  std::fill(this->Pressure.begin(), this->Pressure.end(), 1.f);
}

std::vector<double>& Attributes::GetVelocityArray()
{
  assert(!this->Velocity.empty());
  return this->Velocity;
}

std::vector<float>& Attributes::GetPressureArray()
{
  assert(!this->Pressure.empty());
  return this->Pressure;
}

}
