// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGIsocontourPointsResponder.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDGBezierFindCriticalPoints.h"
#include "vtkDGCell.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeUInt32Array.h"
#include "vtkVector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGIsocontourPointsResponder);

namespace
{

using Rst = std::array<double, 3>;
using vtk::basis::bezier::CriticalPoint;

/// Return the point where the segment from \a lo to \a hi crosses the
/// isovalue, given that \a lo's value is below it and \a hi's is at or above
/// it (so there is at least one crossing). \a field(rst, gradient) returns the
/// field's value at \a rst and sets \a gradient to its parametric gradient.
/// The point returned carries the parametric gradient at its location.
///
/// The search is a bounded Newton-Raphson method over the segment's parameter
/// t, from 0 at \a lo to 1 at \a hi: the crossing stays bracketed, and a
/// Newton step is taken only when it lands inside the bracket and shrinks the
/// step by at least half compared to the one before last; otherwise the
/// bracket is bisected. It therefore cannot leave the segment or fail to
/// converge, and converges quadratically near a simple crossing. It stops once
/// the crossing's position is known to within 1e-12 of the segment's length
/// (or the field is exactly the isovalue). That tolerance is on t, which is
/// dimensionless, so it does not depend on the field's magnitude.
template <typename Field>
CriticalPoint FindIsovaluePoint(
  Field&& field, const CriticalPoint& lo, const CriticalPoint& hi, double isovalue)
{
  constexpr double tTolerance = 1e-12;
  constexpr int maxIterations = 100;

  // A crossing at an end is that end exactly, as in marching cells.
  if (hi.Value == isovalue)
  {
    CriticalPoint end = hi;
    field(hi.Parameter, end.Gradient);
    return end;
  }

  // Positions along the segment are given by t: 0 at lo, 1 at hi.
  const vtkVector3d start(lo.Parameter.data());
  const vtkVector3d direction = vtkVector3d(hi.Parameter.data()) - start;
  auto pointAt = [&](double t)
  {
    const vtkVector3d rst = start + t * direction;
    return Rst{ { rst[0], rst[1], rst[2] } };
  };
  // How far the field is above (positive) or below (negative) the isovalue at
  // t; the crossing is where this is zero. slope is how fast it changes as t
  // grows: the field's gradient projected onto the segment. The gradient is
  // kept, so it is at hand for the point returned (always the last t
  // evaluated).
  Rst gradient{ { 0., 0., 0. } };
  auto offsetAt = [&](double t, double& slope)
  {
    const double offset = field(pointAt(t), gradient) - isovalue;
    slope = vtkVector3d(gradient.data()).Dot(direction);
    return offset;
  };

  // The crossing always lies between tBelow (offset < 0) and tAbove
  // (offset >= 0); each step moves one of them closer to it.
  double tBelow = 0.;
  double tAbove = 1.;
  const double loOffset = lo.Value - isovalue;
  const double hiOffset = hi.Value - isovalue;
  // Start where a linear field would cross, as marching cells does.
  double t = loOffset / (loOffset - hiOffset);
  double offset = 0.;
  double step = 1.;
  double previousStep = step;
  int iteration = 0;
  for (; iteration < maxIterations; ++iteration)
  {
    double slope;
    offset = offsetAt(t, slope);
    // Stop once t has settled: the step that brought it here was tiny.
    if (offset == 0. || std::abs(step) <= tTolerance)
    {
      break;
    }
    (offset < 0. ? tBelow : tAbove) = t;
    if (tAbove - tBelow <= tTolerance)
    {
      break;
    }

    // Newton's step: follow the slope to where the offset would reach zero.
    const double newtonT = t - offset / slope;
    // Written so a NaN or infinite step (slope == 0, or a singular gradient
    // such as at a pyramid's apex) also falls back to bisection.
    const bool newtonUsable = newtonT > tBelow && newtonT < tAbove &&
      std::abs(2. * offset) <= std::abs(previousStep * slope);
    previousStep = step;
    const double nextT = newtonUsable ? newtonT : 0.5 * (tBelow + tAbove);
    step = nextT - t;
    t = nextT;
  }
  if (iteration == maxIterations)
  {
    // Out of iterations: t moved after the offset was last evaluated.
    double slope;
    offset = offsetAt(t, slope);
  }
  return { pointAt(t), isovalue + offset, /*EntityDimension*/ 0, gradient };
}

/// Project \a rst back into the parametric domain of a cell of the given
/// \a shape. For prismatic shapes this is a box clamp; for barycentric shapes
/// it projects onto the simplex; for mixed shapes it does both.
void ClampToParametricDomain(vtkDGCell::Shape shape, int dimension, Rst& rst)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Edge:
      rst[0] = std::clamp(rst[0], -1., 1.);
      break;
    case vtkDGCell::Shape::Quadrilateral:
      rst[0] = std::clamp(rst[0], -1., 1.);
      rst[1] = std::clamp(rst[1], -1., 1.);
      break;
    case vtkDGCell::Shape::Hexahedron:
      rst[0] = std::clamp(rst[0], -1., 1.);
      rst[1] = std::clamp(rst[1], -1., 1.);
      rst[2] = std::clamp(rst[2], -1., 1.);
      break;
    case vtkDGCell::Shape::Triangle:
    {
      // Domain: r >= 0, s >= 0, r + s <= 1.
      rst[0] = std::max(rst[0], 0.);
      rst[1] = std::max(rst[1], 0.);
      const double sum = rst[0] + rst[1];
      if (sum > 1.)
      {
        rst[0] /= sum;
        rst[1] /= sum;
      }
      break;
    }
    case vtkDGCell::Shape::Tetrahedron:
    {
      // Domain: r >= 0, s >= 0, t >= 0, r + s + t <= 1.
      rst[0] = std::max(rst[0], 0.);
      rst[1] = std::max(rst[1], 0.);
      rst[2] = std::max(rst[2], 0.);
      const double sum = rst[0] + rst[1] + rst[2];
      if (sum > 1.)
      {
        rst[0] /= sum;
        rst[1] /= sum;
        rst[2] /= sum;
      }
      break;
    }
    case vtkDGCell::Shape::Wedge:
    {
      // Triangle in (r, s): r >= 0, s >= 0, r + s <= 1.
      // Prismatic in t: t in [-1, 1].
      rst[0] = std::max(rst[0], 0.);
      rst[1] = std::max(rst[1], 0.);
      const double sum = rst[0] + rst[1];
      if (sum > 1.)
      {
        rst[0] /= sum;
        rst[1] /= sum;
      }
      rst[2] = std::clamp(rst[2], -1., 1.);
      break;
    }
    case vtkDGCell::Shape::Pyramid:
    {
      // Base at t=0, apex at t=1. Cross-section shrinks linearly.
      // r, s in [-(1-t), (1-t)], t in [0, 1].
      rst[2] = std::clamp(rst[2], 0., 1.);
      const double halfWidth = 1. - rst[2];
      rst[0] = std::clamp(rst[0], -halfWidth, halfWidth);
      rst[1] = std::clamp(rst[1], -halfWidth, halfWidth);
      break;
    }
    default:
      // Vertex or unknown: clamp each active axis to [-1, 1].
      for (int k = 0; k < dimension; ++k)
      {
        rst[k] = std::clamp(rst[k], -1., 1.);
      }
      break;
  }
}

/// Return the diameter of the parametric domain of a cell of the given
/// \a shape (the distance between its two farthest points, or an upper bound).
double ParametricDiameter(vtkDGCell::Shape shape, int dimension)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Tetrahedron:
      return std::sqrt(2.);
    case vtkDGCell::Shape::Wedge:
      return std::sqrt(6.);
    case vtkDGCell::Shape::Pyramid:
      return 2. * std::sqrt(2.);
    default:
      // A box from -1 to 1 along each axis.
      return 2. * std::sqrt(static_cast<double>(dimension));
  }
}

/// Return the average, over \a particles, of the parametric distance from each
/// to its nearest neighbor.
double MeanNearestNeighborDistance(const std::vector<CriticalPoint>& particles, int dimension)
{
  const int numParticles = static_cast<int>(particles.size());
  double totalNearestDistance = 0.;
  for (int i = 0; i < numParticles; ++i)
  {
    double minDistance2 = std::numeric_limits<double>::max();
    for (int j = 0; j < numParticles; ++j)
    {
      if (i == j)
      {
        continue;
      }
      double distance2 = 0.;
      for (int k = 0; k < dimension; ++k)
      {
        const double diff = particles[i].Parameter[k] - particles[j].Parameter[k];
        distance2 += diff * diff;
      }
      minDistance2 = std::min(minDistance2, distance2);
    }
    totalNearestDistance += std::sqrt(minDistance2);
  }
  return totalNearestDistance / numParticles;
}

/// A deterministic source of uniform random numbers in [0, 1) (xorshift64),
/// so that the same cell always gives the same particles.
class UniformRandom
{
public:
  explicit UniformRandom(std::uint64_t seed)
    : State(seed == 0 ? 1 : seed)
  {
  }

  double Next()
  {
    this->State ^= this->State << 13;
    this->State ^= this->State >> 7;
    this->State ^= this->State << 17;
    // The top 53 bits, as a fraction of 2^53.
    return static_cast<double>(this->State >> 11) / 9007199254740992.;
  }

private:
  std::uint64_t State;
};

/// Bring \a particle back onto the isosurface after it moved: up to
/// \a maxSteps Newton steps along the gradient, each followed by a clamp to the
/// cell's parametric domain. Afterwards the particle's Value and Gradient are
/// those at its Parameter.
template <typename Field>
void ProjectOntoIsosurface(Field&& field, CriticalPoint& particle, int dimension,
  vtkDGCell::Shape shape, double isovalue, int maxSteps)
{
  ClampToParametricDomain(shape, dimension, particle.Parameter);
  for (int step = 0; step < maxSteps; ++step)
  {
    Rst gradient;
    const double value = field(particle.Parameter, gradient);
    particle.Value = value;
    particle.Gradient = gradient;
    const double residual = value - isovalue;
    if (std::abs(residual) < 1e-14)
    {
      return;
    }
    double gradientNorm2 = 0.;
    for (int k = 0; k < dimension; ++k)
    {
      gradientNorm2 += gradient[k] * gradient[k];
    }
    if (gradientNorm2 < 1e-30)
    {
      return;
    }
    for (int k = 0; k < dimension; ++k)
    {
      particle.Parameter[k] -= residual / gradientNorm2 * gradient[k];
    }
    ClampToParametricDomain(shape, dimension, particle.Parameter);
  }
  particle.Value = field(particle.Parameter, particle.Gradient);
}

/// Remove from \a direction its component along \a gradient, leaving a vector
/// tangent to the isosurface.
void MakeTangent(Rst& direction, const Rst& gradient, int dimension)
{
  double gradientNorm2 = 0.;
  double dot = 0.;
  for (int k = 0; k < dimension; ++k)
  {
    gradientNorm2 += gradient[k] * gradient[k];
    dot += direction[k] * gradient[k];
  }
  if (gradientNorm2 > 1e-30)
  {
    for (int k = 0; k < dimension; ++k)
    {
      direction[k] -= dot / gradientNorm2 * gradient[k];
    }
  }
}

/// Witkin & Heckbert (1994) adaptive repulsion, fission and death, which
/// redistributes \a particles more uniformly along the isosurface inside one
/// cell ("Using Particles to Sample and Control Implicit Surfaces", sections
/// 4.3 and 4.4). The particles, all of them on the isosurface already, move in
/// the cell's parametric space.
///
/// Every particle i has its own repulsion radius sigma_i, and repels each
/// particle j with the energy E_ij = alpha exp(-|p_i - p_j|^2 / (2 sigma_i^2)).
/// Each iteration:
/// 1. A particle's desired velocity is the gradient of its energy, the sum of
///    E_ij + E_ji over the others (equation 9), projected onto the tangent
///    plane of the isosurface (equation 5).
/// 2. Its radius changes to drive the energy D_i = sum of E_ij, that its own
///    radius controls, toward the level Ehat of a hexagonal packing (equations
///    10, 12 and 13): it grows where the particle is alone and shrinks where it
///    is crowded.
/// 3. Positions and radii move a time step along these velocities (Euler), and
///    each particle is brought back onto the isosurface by Newton steps.
/// 4. A particle near equilibrium splits in two when its radius is huge, or
///    when it is energized and its radius is above the desired one; and it
///    dies, at random, when its radius is well below the desired one.
///
/// The desired radius is half the initial average spacing of the particles, so
/// that the sampling ends up as dense as the particles started on average, but
/// evenly. The constants are those the paper recommends.
template <typename Field>
void RelaxParticlesOnIsosurface(Field&& field, std::vector<CriticalPoint>& particles, int dimension,
  vtkDGCell::Shape shape, double isovalue, int maxIterations)
{
  if (particles.size() < 2 || maxIterations <= 0)
  {
    return;
  }
  const double meanSpacing = MeanNearestNeighborDistance(particles, dimension);
  if (meanSpacing < 1e-15)
  {
    return; // All particles are coincident; nothing to relax.
  }

  constexpr double alpha = 6.;             // Repulsion amplitude.
  constexpr double timeStep = 0.03;        // Euler time step.
  constexpr double energyGain = 15.;       // Feedback on the energy (rho).
  constexpr double regularizer = 10.;      // Keeps a lone particle's radius growth finite (beta).
  constexpr double equilibriumFactor = 4.; // Near equilibrium: speed < this * radius (gamma).
  constexpr double fissionEnergyFraction = 0.2; // Fission needs this fraction of Ehat (nu).
  constexpr double deathRadiusFraction = 0.7;   // Death needs a radius below this fraction of the
                                                // desired one (delta).
  // The energy of a particle among the six of a hexagonal packing, at twice
  // the radius from it: 6 alpha exp(-2).
  const double desiredEnergy = 6. * alpha * std::exp(-2.);
  // Two neighbors in hexagonal packing are two radii apart.
  const double desiredRadius = 0.5 * meanSpacing;
  const double maxRadius =
    std::max(0.5 * ParametricDiameter(shape, dimension), 1.5 * desiredRadius);
  const double minRadius = 1e-3 * desiredRadius;
  // Cap the particle count to keep the cost of each cell bounded.
  constexpr std::size_t maxParticles = 500;

  UniformRandom random(0x9E3779B97F4A7C15ull + particles.size());
  std::vector<double> radii(particles.size(), desiredRadius);
  std::vector<Rst> velocities;
  std::vector<double> speeds;
  std::vector<double> energies;
  std::vector<double> radiusRates;

  for (int iteration = 0; iteration < maxIterations; ++iteration)
  {
    const int numParticles = static_cast<int>(particles.size());
    velocities.assign(numParticles, Rst{ { 0., 0., 0. } });
    speeds.assign(numParticles, 0.);
    energies.assign(numParticles, 0.);
    radiusRates.assign(numParticles, 0.);

    // The desired velocity and radius change of every particle.
    double maxSpeed = 0.;
    for (int i = 0; i < numParticles; ++i)
    {
      const double radius2 = radii[i] * radii[i];
      Rst velocity = { { 0., 0., 0. } };
      double energy = 0.;      // D_i
      double energySlope = 0.; // The derivative of D_i with respect to sigma_i.
      for (int j = 0; j < numParticles; ++j)
      {
        if (i == j)
        {
          continue;
        }
        Rst diff;
        double distance2 = 0.;
        for (int k = 0; k < dimension; ++k)
        {
          diff[k] = particles[i].Parameter[k] - particles[j].Parameter[k];
          distance2 += diff[k] * diff[k];
        }
        const double otherRadius2 = radii[j] * radii[j];
        // Beyond this (more than four radii) neither Gaussian is felt.
        if (distance2 > 18. * std::max(radius2, otherRadius2))
        {
          continue;
        }
        const double energyOfI = alpha * std::exp(-distance2 / (2. * radius2));      // E_ij
        const double energyOfJ = alpha * std::exp(-distance2 / (2. * otherRadius2)); // E_ji
        // Equation 9: sigma_i^2 (r_ij / sigma_i^2 E_ij + r_ij / sigma_j^2 E_ji).
        const double weight = energyOfI + radius2 / otherRadius2 * energyOfJ;
        for (int k = 0; k < dimension; ++k)
        {
          velocity[k] += weight * diff[k];
        }
        energy += energyOfI;
        // Equation 13.
        energySlope += distance2 * energyOfI / (radius2 * radii[i]);
      }
      MakeTangent(velocity, particles[i].Gradient, dimension);
      double speed2 = 0.;
      for (int k = 0; k < dimension; ++k)
      {
        speed2 += velocity[k] * velocity[k];
      }
      velocities[i] = velocity;
      speeds[i] = std::sqrt(speed2);
      energies[i] = energy;
      // Equations 10 and 12.
      radiusRates[i] = -energyGain * (energy - desiredEnergy) / (energySlope + regularizer);
      maxSpeed = std::max(maxSpeed, speeds[i]);
    }

    // Move the particles and change their radii.
    for (int i = 0; i < numParticles; ++i)
    {
      for (int k = 0; k < dimension; ++k)
      {
        particles[i].Parameter[k] += timeStep * velocities[i][k];
      }
      ProjectOntoIsosurface(field, particles[i], dimension, shape, isovalue, 3);
      radii[i] = std::clamp(radii[i] + timeStep * radiusRates[i], minRadius, 2. * maxRadius);
    }

    // Fission and death, not after the last move, which would leave the new
    // particles unrelaxed.
    if (iteration + 1 == maxIterations)
    {
      break;
    }
    std::vector<CriticalPoint> nextParticles;
    std::vector<double> nextRadii;
    nextParticles.reserve(particles.size());
    nextRadii.reserve(particles.size());
    std::size_t numDeaths = 0;
    for (int i = 0; i < numParticles; ++i)
    {
      const double radius = radii[i];
      const bool nearEquilibrium = speeds[i] < equilibriumFactor * radius;
      bool splits = false;
      bool dies = false;
      if (nearEquilibrium)
      {
        splits = radius > maxRadius ||
          (energies[i] > fissionEnergyFraction * desiredEnergy && radius > desiredRadius);
        // The death test is random, so that an overcrowded region does not
        // lose all its particles at once.
        dies = !splits && radius < deathRadiusFraction * desiredRadius &&
          random.Next() > radius / (deathRadiusFraction * desiredRadius) &&
          static_cast<std::size_t>(numParticles) - numDeaths > 2;
      }
      if (dies)
      {
        ++numDeaths;
        continue;
      }
      const bool room = nextParticles.size() + (numParticles - i) < maxParticles;
      if (splits && room)
      {
        // Two particles, each with radius sigma / sqrt(2), moved apart along a
        // random direction tangent to the isosurface.
        Rst direction;
        for (int k = 0; k < dimension; ++k)
        {
          direction[k] = 2. * random.Next() - 1.;
        }
        MakeTangent(direction, particles[i].Gradient, dimension);
        double length2 = 0.;
        for (int k = 0; k < dimension; ++k)
        {
          length2 += direction[k] * direction[k];
        }
        if (length2 > 1e-30)
        {
          const double childRadius = radius / std::sqrt(2.);
          const double offset = 0.3 * radius / std::sqrt(length2);
          CriticalPoint first = particles[i];
          CriticalPoint second = particles[i];
          for (int k = 0; k < dimension; ++k)
          {
            first.Parameter[k] -= offset * direction[k];
            second.Parameter[k] += offset * direction[k];
          }
          ProjectOntoIsosurface(field, first, dimension, shape, isovalue, 5);
          ProjectOntoIsosurface(field, second, dimension, shape, isovalue, 5);
          nextParticles.push_back(first);
          nextRadii.push_back(childRadius);
          nextParticles.push_back(second);
          nextRadii.push_back(childRadius);
          continue;
        }
      }
      nextParticles.push_back(particles[i]);
      nextRadii.push_back(radius);
    }
    const bool changed = nextParticles.size() != particles.size() || numDeaths > 0;
    particles = std::move(nextParticles);
    radii = std::move(nextRadii);

    // Stop when nothing moves any more and no particle was born or died.
    if (!changed && maxSpeed < 1e-9 * desiredRadius)
    {
      break;
    }
  }

  // Ensure every particle has up-to-date Value and Gradient at its final Parameter.
  for (auto& particle : particles)
  {
    particle.Value = field(particle.Parameter, particle.Gradient);
    particle.EntityDimension = dimension;
  }
}

} // anonymous namespace

bool vtkDGIsocontourPointsResponder::Query(vtkCellGridIsocontourPoints::Query* query,
  vtkCellMetadata* cellType, vtkCellGridResponders* vtkNotUsed(caches))
{
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }
  switch (query->GetPass())
  {
    case vtkCellGridIsocontourPoints::Query::PassType::CountOutputs:
      return this->FindPoints(query, dgCell);
    case vtkCellGridIsocontourPoints::Query::PassType::AllocateOutputs:
      query->Allocate();
      return true;
    case vtkCellGridIsocontourPoints::Query::PassType::GenerateOutputs:
      return this->GeneratePoints(query, dgCell);
    case vtkCellGridIsocontourPoints::Query::PassType::EvaluateAttributes:
      return this->InterpolateAttributes(query, dgCell);
    default:
      break;
  }
  return true;
}

bool vtkDGIsocontourPointsResponder::FindPoints(
  vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType)
{
  auto* attribute = query->GetCellAttribute();
  if (!attribute)
  {
    vtkErrorMacro("No cell-attribute to contour.");
    return false;
  }

  vtkStringToken cellTypeName(cellType->GetClassName());
  auto info = attribute->GetCellTypeInfo(cellTypeName);

  // Cell types whose attribute is not Bezier are left alone: a grid may hold
  // several cell types and only some may be in a basis whose coefficients
  // bound the field.
  if (info.FunctionSpace != "Bezier"_token)
  {
    return true;
  }

  // A vertex has no segments to search, and other cells may be excluded by
  // their dimension.
  const int dimension = cellType->GetDimension();
  if (!query->IncludesDimension(dimension))
  {
    return true;
  }

  auto basisOp = cellType->GetOperatorEntry("Basis"_token, info);
  auto gradOp = cellType->GetOperatorEntry("BasisGradient"_token, info);
  if (!basisOp || !gradOp)
  {
    vtkErrorMacro("No Bezier basis operator for this cell type.");
    return false;
  }
  std::vector<int> order = cellType->GetBasisOrder(info);
  const int numFunctions = basisOp.NumberOfFunctions;

  auto* values = info.GetArrayForRoleAs<vtkDataArray>("values"_token);
  if (!values)
  {
    vtkErrorMacro("The attribute has no array in the \"values\" role.");
    return false;
  }
  const bool shared = info.DOFSharing.IsValid();
  auto* connectivity = info.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
  if (shared && (!connectivity || connectivity->GetNumberOfComponents() != numFunctions))
  {
    vtkErrorMacro("The attribute shares degrees of freedom but has no matching connectivity.");
    return false;
  }
  const int numComponents =
    shared ? values->GetNumberOfComponents() : values->GetNumberOfComponents() / numFunctions;
  if (numComponents != 1)
  {
    vtkErrorMacro("Only scalar attributes are supported so far; this one has " << numComponents
                                                                               << " components.");
    return false;
  }
  const vtkIdType numCells =
    shared ? connectivity->GetNumberOfTuples() : values->GetNumberOfTuples();

  const vtkDGCell::Shape shape = cellType->GetShape();
  const int numCorners = cellType->GetNumberOfCorners();
  const double isovalue = query->GetIsovalue();
  const int relaxationIterations = query->GetParticleRelaxationIterations();

  std::vector<std::vector<CriticalPoint>> isocontourPointsByCell(numCells);
  // Every cell's search reads its own coefficients and writes only its own
  // slot of isocontourPointsByCell, so cells may be searched concurrently; basisOp and
  // gradOp are only ever Evaluate()'d.
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      std::vector<vtkTypeUInt64> dofIds(shared ? numFunctions : 0);
      std::vector<double> coefficients(numFunctions);
      std::vector<CriticalPoint> below;
      std::vector<CriticalPoint> above;
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        if (!shared)
        {
          values->GetTuple(cellId, coefficients.data());
        }
        else
        {
          connectivity->GetUnsignedTuple(cellId, dofIds.data());
          for (int ff = 0; ff < numFunctions; ++ff)
          {
            coefficients[ff] = values->GetComponent(static_cast<vtkIdType>(dofIds[ff]), 0);
          }
        }

        // The field lies within the convex hull of its Bezier coefficients,
        // so a cell they do not bracket cannot hold the isocontour.
        const auto hull = std::minmax_element(coefficients.begin(), coefficients.end());
        if (isovalue < *hull.first || isovalue > *hull.second)
        {
          continue;
        }

        auto evaluateField = [&](const Rst& rst, Rst& gradient) -> double
        {
          return vtk::basis::bezier::EvaluateField(
            basisOp, gradOp, dimension, coefficients, rst, gradient);
        };

        auto& found = isocontourPointsByCell[cellId];
        below.clear();
        above.clear();
        // As in marching cells, a value equal to the isovalue counts as
        // above it. Every cell sharing a value then makes the same choice,
        // and a segment ending on the isovalue puts its point on that end.
        auto classify = [&](const CriticalPoint& point) -> void
        { (point.Value < isovalue ? below : above).push_back(point); };

        for (int corner = 0; corner < numCorners; ++corner)
        {
          const Rst& rst = cellType->GetCornerParameter(corner);
          Rst gradient;
          classify({ rst, evaluateField(rst, gradient), /*EntityDimension*/ 0 });
        }
        const auto criticalPoints =
          vtk::basis::bezier::FindCriticalPoints(shape, basisOp, gradOp, order, coefficients);
        for (const CriticalPoint& criticalPoint : criticalPoints)
        {
          classify(criticalPoint);
        }

        // Every segment from a point below the isovalue to one above it
        // crosses the isocontour; the cell's parameter space is convex, so
        // the segment stays inside the cell.
        for (const auto& lo : below)
        {
          for (const auto& hi : above)
          {
            CriticalPoint point = FindIsovaluePoint(evaluateField, lo, hi, isovalue);
            // Which face or edge the point lies on, if any, is not tracked.
            point.EntityDimension = dimension;
            found.push_back(point);
          }
        }

        if (relaxationIterations > 0 && !found.empty())
        {
          RelaxParticlesOnIsosurface(
            evaluateField, found, dimension, shape, isovalue, relaxationIterations);
        }
      }
    });

  query->SetIsocontourPoints(cellTypeName, std::move(isocontourPointsByCell));
  return true;
}

bool vtkDGIsocontourPointsResponder::GeneratePoints(
  vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType)
{
  vtkStringToken cellTypeName(cellType->GetClassName());
  const auto& isocontourPointsByCell = query->GetIsocontourPoints(cellTypeName);
  if (isocontourPointsByCell.empty())
  {
    // Nothing was cached for this cell type in the CountOutputs pass.
    return true;
  }

  auto* grid = cellType->GetCellGrid();
  auto* shapeAttribute = grid ? grid->GetShapeAttribute() : nullptr;
  if (!shapeAttribute)
  {
    vtkErrorMacro("The cell grid has no shape attribute to place isocontour points with.");
    return false;
  }
  vtkNew<vtkDGInterpolateCalculator> shapeCalcProto;
  auto rawShapeCalc = shapeCalcProto->PrepareForGrid(cellType, shapeAttribute);
  auto* shapeCalc = vtkDGInterpolateCalculator::SafeDownCast(rawShapeCalc);
  if (!shapeCalc)
  {
    vtkErrorMacro("Could not prepare a calculator for the grid's shape attribute.");
    return false;
  }

  auto* output = query->GetOutput();
  auto* pd = output->GetPointData();
  auto* outputCellTypes = vtkTypeUInt32Array::SafeDownCast(pd->GetArray("CellType"));
  auto* outputCellIds = vtkIdTypeArray::SafeDownCast(pd->GetArray("CellId"));
  auto* outputRst = vtkDoubleArray::SafeDownCast(pd->GetArray("RST"));
  auto* outputValues = vtkDoubleArray::SafeDownCast(pd->GetArray("Value"));
  auto* outputGradients = vtkDoubleArray::SafeDownCast(pd->GetArray("Gradient"));

  const vtkIdType cellTypeOffset = query->GetCellTypeOffset(cellTypeName);
  const vtkIdType numCells = static_cast<vtkIdType>(isocontourPointsByCell.size());
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        const auto& isocontourPoints = isocontourPointsByCell[cellId];
        if (isocontourPoints.empty())
        {
          continue;
        }

        vtkIdType writeIndex = query->GetCellOffset(cellTypeOffset, cellId);
        for (const auto& point : isocontourPoints)
        {
          outputCellTypes->SetValue(writeIndex, cellTypeName.GetId());
          outputCellIds->SetValue(writeIndex, cellId);
          outputRst->SetTypedTuple(writeIndex, point.Parameter.data());
          outputValues->SetValue(writeIndex, point.Value);
          if (outputGradients)
          {
            // Parametric for now; turned into world coordinates below.
            outputGradients->SetTypedTuple(writeIndex, point.Gradient.data());
          }
          ++writeIndex;
        }
      }
    });

  // One batched Evaluate() over this cell type's whole slice.
  const vtkIdType start = query->GetCellOffset(cellTypeOffset, 0);
  const vtkIdType count = query->GetCellOffset(cellTypeOffset, numCells) - start;
  if (count > 0)
  {
    vtkNew<vtkIdTypeArray> cellIdsWindow;
    cellIdsWindow->SetArray(outputCellIds->GetPointer(start), count, /*save*/ 1);
    vtkNew<vtkDoubleArray> rstWindow;
    rstWindow->SetNumberOfComponents(3);
    rstWindow->SetArray(outputRst->GetPointer(3 * start), 3 * count, /*save*/ 1);
    vtkNew<vtkDoubleArray> pointsWindow;
    pointsWindow->SetNumberOfComponents(3);
    auto* outputPointsData = vtkDoubleArray::SafeDownCast(output->GetPoints()->GetData());
    pointsWindow->SetArray(outputPointsData->GetPointer(3 * start), 3 * count, /*save*/ 1);
    shapeCalc->Evaluate(cellIdsWindow, rstWindow, pointsWindow, /*useMultithreading*/ true);

    if (!outputGradients)
    {
      return true;
    }
    // The world-space gradient g of the field solves J^T g = p, where p is its
    // parametric gradient so far and J the shape's Jacobian (J[i][j] = dx_i /
    // dr_j, row-major, as the shape's derivative is output). This is only done
    // for volumetric cells; other cells, and points where J is singular (such
    // as a pyramid's apex), get a zero gradient.
    vtkNew<vtkDoubleArray> jacobians;
    jacobians->SetNumberOfComponents(9);
    jacobians->SetNumberOfTuples(count);
    shapeCalc->EvaluateDerivative(cellIdsWindow, rstWindow, jacobians, /*useMultithreading*/ true);
    const bool volumetric = cellType->GetDimension() == 3;
    vtkSMPTools::For(start, start + count,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType ii = begin; ii < end; ++ii)
        {
          double jacobian[3][3];
          jacobians->GetTypedTuple(ii - start, &jacobian[0][0]);
          double parametricGradient[3];
          outputGradients->GetTypedTuple(ii, parametricGradient);
          double worldGradient[3] = { 0., 0., 0. };
          if (volumetric)
          {
            vtkMath::LinearSolveTransposed3x3(jacobian, parametricGradient, worldGradient);
          }
          outputGradients->SetTypedTuple(ii, worldGradient);
        }
      });
  }
  return true;
}

bool vtkDGIsocontourPointsResponder::InterpolateAttributes(
  vtkCellGridIsocontourPoints::Query* query, vtkDGCell* cellType)
{
  vtkStringToken cellTypeName(cellType->GetClassName());
  const auto& isocontourPointsByCell = query->GetIsocontourPoints(cellTypeName);
  if (isocontourPointsByCell.empty())
  {
    // Nothing was cached for this cell type in the CountOutputs pass.
    return true;
  }
  auto* grid = cellType->GetCellGrid();
  auto* shapeAttribute = grid ? grid->GetShapeAttribute() : nullptr;
  if (!shapeAttribute)
  {
    vtkErrorMacro("The cell grid has no shape attribute to place isocontour points with.");
    return false;
  }

  // This cell type's slice of the output, whose cells and parametric
  // coordinates the GenerateOutputs pass wrote.
  auto* pd = query->GetOutput()->GetPointData();
  auto* outputCellIds = vtkIdTypeArray::SafeDownCast(pd->GetArray("CellId"));
  auto* outputRst = vtkDoubleArray::SafeDownCast(pd->GetArray("RST"));
  const vtkIdType cellTypeOffset = query->GetCellTypeOffset(cellTypeName);
  const vtkIdType numCells = static_cast<vtkIdType>(isocontourPointsByCell.size());
  const vtkIdType start = query->GetCellOffset(cellTypeOffset, 0);
  const vtkIdType count = query->GetCellOffset(cellTypeOffset, numCells) - start;
  if (count == 0)
  {
    return true;
  }
  vtkNew<vtkIdTypeArray> cellIdsWindow;
  cellIdsWindow->SetArray(outputCellIds->GetPointer(start), count, /*save*/ 1);
  vtkNew<vtkDoubleArray> rstWindow;
  rstWindow->SetNumberOfComponents(3);
  rstWindow->SetArray(outputRst->GetPointer(3 * start), 3 * count, /*save*/ 1);
  // Every other attribute of the grid that has an array in the output, at the
  // same cells and parametric coordinates. An attribute this cell type does not
  // have is NaN.
  for (const auto& attribute : grid->GetCellAttributeList())
  {
    auto* outputAttribute =
      vtkDoubleArray::SafeDownCast(pd->GetArray(attribute->GetName().Data().c_str()));
    const int numComponents = attribute->GetNumberOfComponents();
    if (attribute == shapeAttribute || !outputAttribute ||
      outputAttribute->GetNumberOfComponents() != numComponents)
    {
      continue;
    }
    vtkNew<vtkDoubleArray> attributeWindow;
    attributeWindow->SetNumberOfComponents(numComponents);
    attributeWindow->SetArray(
      outputAttribute->GetPointer(numComponents * start), numComponents * count, /*save*/ 1);
    vtkNew<vtkDGInterpolateCalculator> calcProto;
    vtkSmartPointer<vtkCellAttributeCalculator> rawCalc;
    if (attribute->GetCellTypeInfo(cellTypeName).FunctionSpace.IsValid())
    {
      rawCalc = calcProto->PrepareForGrid(cellType, attribute);
    }
    auto* calc = vtkDGInterpolateCalculator::SafeDownCast(rawCalc);
    if (calc)
    {
      calc->Evaluate(cellIdsWindow, rstWindow, attributeWindow, /*useMultithreading*/ true);
    }
    else
    {
      attributeWindow->Fill(std::numeric_limits<double>::quiet_NaN());
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
