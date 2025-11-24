// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLissajousPointCloud.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <random>

VTK_ABI_NAMESPACE_BEGIN

namespace // anonymous
{

using Functor = std::function<void(vtkIdType, vtkIdType)>;

void GramSchmidt(
  const double* v1, const double* v2, const double* v3, double* u1, double* u2, double* u3)
{
  double projectionV2U1[3], projectionV3U1[3], projectionV3U2[3];

  for (int i = 0; i < 3; ++i)
  {
    u1[i] = v1[i];
  }

  vtkMath::ProjectVector(v2, u1, projectionV2U1);
  for (int i = 0; i < 3; ++i)
  {
    u2[i] = v2[i] - projectionV2U1[i];
  }

  vtkMath::ProjectVector(v3, u1, projectionV3U1);
  vtkMath::ProjectVector(v3, u2, projectionV3U2);
  for (int i = 0; i < 3; ++i)
  {
    u3[i] = v3[i] - projectionV3U1[i] - projectionV3U2[i];
  }

  vtkMath::Normalize(u1);
  vtkMath::Normalize(u2);
  vtkMath::Normalize(u3);
}

template <bool HaveNoise>
Functor generatePointFunctor(
  vtkLissajousPointCloud* self, vtkPoints* pts, vtkDoubleArray* param, vtkIntArray* region)
{
  double AA[3];
  double FF[3];
  double PP[3];
  double NN[3];
  self->GetAmplitude(AA);
  self->GetFrequency(FF);
  self->GetPhase(PP);
  self->GetNoise(NN);
  double RR = self->GetRadius();
  vtkIdType np = self->GetNumberOfPoints();

  auto func = [AA, FF, PP, NN, RR, np, pts, param, region](vtkIdType begin, vtkIdType end)
  {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_real_distribution<double> u(0., 1.);

    for (vtkIdType ii = begin; ii < end; ++ii)
    {
      double t = static_cast<double>(ii) / static_cast<double>(np) * 2. * vtkMath::Pi();
      std::array<double, 3> point;
      for (int jj = 0; jj < 3; ++jj)
      {
        point[jj] = AA[jj] * std::sin(FF[jj] * t + PP[jj]);
        if (HaveNoise)
        {
          point[jj] += u(gen) * NN[jj];
        }
      }
      if (RR <= 0.)
      {
        pts->SetPoint(ii, point.data());
        param->SetValue(ii, t);
        region->SetValue(ii, 0);
      }
      else
      {
        pts->SetPoint(3 * ii, point.data());
        param->SetValue(3 * ii, t);
        region->SetValue(3 * ii, 0);

        std::array<double, 3> tangent;
        std::array<double, 3> normal;
        std::array<double, 3> binormal;
        for (int jj = 0; jj < 3; ++jj)
        {
          // point[jj] = AA[jj] * std::sin(FF[jj] * t + PP[jj]);
          // tangent = d/dt (Asin(Ft+P) = A*F*cos(Ft+P)
          // normal = d/dt (AFcos(Ft+P) = -A*A*F*F*sin(Ft+P)
          // binormal = tangent × normal
          tangent[jj] = AA[jj] * FF[jj] * std::cos(FF[jj] * t + PP[jj]);
          normal[jj] = -AA[jj] * AA[jj] * FF[jj] * FF[jj] * std::sin(FF[jj] * t + PP[jj]);
        }
        binormal[0] = tangent[1] * normal[2] - tangent[2] * normal[1];
        binormal[1] = tangent[2] * normal[0] - tangent[0] * normal[2];
        binormal[2] = tangent[0] * normal[1] - tangent[1] * normal[0];
        std::array<double, 3> ct;
        std::array<double, 3> cn;
        std::array<double, 3> cb;
        ::GramSchmidt(
          tangent.data(), normal.data(), binormal.data(), ct.data(), cn.data(), cb.data());
        // Now compute the angle in the cn/cb plane we wish to sample.
        // We have np total points along the curve but let's assume a constant factor of ii radians
        // per point.
        // Reuse tangent and normal to hold displaced point.
        double sni = std::sin(static_cast<double>(ii));
        double csi = std::cos(static_cast<double>(ii));
        for (int jj = 0; jj < 3; ++jj)
        {
          tangent[jj] = point[jj] + 2 * RR * (sni * cn[jj] + csi * cb[jj]);
          normal[jj] = point[jj] - 2 * RR * (sni * cn[jj] + csi * cb[jj]);
        }
        pts->SetPoint(3 * ii + 1, tangent.data());
        param->SetValue(3 * ii + 1, t);
        region->SetValue(3 * ii + 1, +1);
        pts->SetPoint(3 * ii + 2, normal.data());
        param->SetValue(3 * ii + 2, t);
        region->SetValue(3 * ii + 2, +1);
      }
    }
  };
  return func;
}

void addDeterministicNoise(vtkLissajousPointCloud* self, vtkPoints* pts)
{
  if (!self || !self->GetDeterministicNoise())
  {
    return;
  }
  vtkNew<vtkMinimalStandardRandomSequence> rseq;
  rseq->Initialize(self->GetDeterministicSeed());
  std::array<double, 3> point;
  std::array<double, 3> noisy;
  double NN[3];
  self->GetNoise(NN);
  vtkIdType nn = self->GetNumberOfPoints();
  for (vtkIdType ii = 0; ii < nn; ++ii)
  {
    pts->GetPoint(ii, point.data());
    for (int jj = 0; jj < 3; ++jj)
    {
      noisy[jj] = rseq->GetValue() * NN[jj] + point[jj];
      rseq->Next();
    }
    pts->SetPoint(ii, noisy.data());
  }
}

} // anonymous namespace

//================= Begin VTK class proper =====================================
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkLissajousPointCloud);

//------------------------------------------------------------------------------
vtkLissajousPointCloud::vtkLissajousPointCloud()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkLissajousPointCloud::~vtkLissajousPointCloud() = default;

//------------------------------------------------------------------------------
void vtkLissajousPointCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number Of Points: " << this->NumberOfPoints << endl;
  os << indent << "Amplitude: " << this->Amplitude[0] << " " << this->Amplitude[1] << " "
     << this->Amplitude[2] << endl;
  os << indent << "Frequency: " << this->Frequency[0] << " " << this->Frequency[1] << " "
     << this->Frequency[2] << endl;
  os << indent << "Phase: " << this->Phase[0] << " " << this->Phase[1] << " " << this->Phase[2]
     << endl;
  os << indent << "Noise: " << this->Noise[0] << " " << this->Noise[1] << " " << this->Noise[2]
     << endl;
  os << indent << "Deterministic Noise: " << (this->DeterministicNoise ? "On" : "Off") << endl;
  os << indent << "Deterministic Seed: " << this->DeterministicSeed << endl;
  os << indent << "Generate Parameter Scalar: " << (this->GenerateParameterScalar ? "On" : "Off")
     << endl;
  os << indent << "Generate Region Scalar: " << (this->GenerateRegionScalar ? "On" : "Off") << endl;
  os << indent << "Add Background Points: " << (this->AddBackgroundPoints ? "On" : "Off") << endl;
  os << indent << "Background Label: " << (this->BackgroundLabel ? "On" : "Off") << endl;
}

//------------------------------------------------------------------------------
int vtkLissajousPointCloud::RequestData(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  (void)request;
  (void)inInfo;
  auto* output = vtkPolyData::GetData(outInfo, 0);

  vtkNew<vtkPoints> pts;
  vtkNew<vtkCellArray> verts;
  vtkNew<vtkDoubleArray> param;
  param->SetName("t");
  vtkNew<vtkIntArray> region;
  region->SetName("region");

  // Allocate points.
  vtkIdType pp = this->NumberOfPoints;
  vtkIdType qq = pp;
  vtkIdType rr = qq;
  if (this->Radius > 0.)
  {
    pp = 3 * pp;
    qq = (this->AddBackgroundPoints ? pp + 9 : pp);
  }
  pts->SetNumberOfPoints(qq);
  param->SetNumberOfTuples(qq);
  region->SetNumberOfTuples(qq);

  bool haveNoise = false;
  for (auto ii = 0; ii < 3; ++ii)
  {
    if (this->Noise[ii] != 0.0)
    {
      haveNoise = true;
      break;
    }
  }

  if (!haveNoise || this->DeterministicNoise)
  {
    vtkSMPTools::For(0, rr, generatePointFunctor<false>(this, pts, param, region));
    if (this->DeterministicNoise)
    {
      addDeterministicNoise(this, pts);
    }
  }
  else
  {
    vtkSMPTools::For(0, rr, generatePointFunctor<true>(this, pts, param, region));
  }

  // If requested, add 9 final points based on the bounding box (the eight corners
  // and center point of the bounding box).
  if (this->AddBackgroundPoints && this->Radius > 0.)
  {
    int label = this->BackgroundLabel;
    // These points have no noise added to them and a fixed region
    // classification.
    double ax = this->Amplitude[0] + 2. * this->Radius;
    double ay = this->Amplitude[1] + 2. * this->Radius;
    double az = this->Amplitude[2] + 2. * this->Radius;
    pts->SetPoint(pp + 0, 0., 0., 0.);
    param->SetValue(pp + 0, -1);
    region->SetValue(pp + 0, label);
    pts->SetPoint(pp + 1, -ax, -ay, -az);
    param->SetValue(pp + 1, -1);
    region->SetValue(pp + 1, label);
    pts->SetPoint(pp + 2, ax, -ay, -az);
    param->SetValue(pp + 2, -1);
    region->SetValue(pp + 2, label);
    pts->SetPoint(pp + 3, ax, ay, -az);
    param->SetValue(pp + 3, -1);
    region->SetValue(pp + 3, label);
    pts->SetPoint(pp + 4, -ax, ay, -az);
    param->SetValue(pp + 4, -1);
    region->SetValue(pp + 4, label);
    pts->SetPoint(pp + 5, -ax, -ay, az);
    param->SetValue(pp + 5, -1);
    region->SetValue(pp + 5, label);
    pts->SetPoint(pp + 6, ax, -ay, az);
    param->SetValue(pp + 6, -1);
    region->SetValue(pp + 6, label);
    pts->SetPoint(pp + 7, ax, ay, az);
    param->SetValue(pp + 7, -1);
    region->SetValue(pp + 7, label);
    pts->SetPoint(pp + 8, -ax, ay, az);
    param->SetValue(pp + 8, -1);
    region->SetValue(pp + 8, label);
  }

  verts->AllocateExact(qq, 1);
  for (vtkIdType ii = 0; ii < qq; ++ii)
  {
    verts->InsertNextCell(1, &ii);
  }

  output->SetPoints(pts);
  output->SetVerts(verts);
  if (this->GenerateParameterScalar)
  {
    output->GetPointData()->SetScalars(param);
  }
  if (this->GenerateRegionScalar)
  {
    output->GetPointData()->AddArray(region);
    output->GetCellData()->SetScalars(region);
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
