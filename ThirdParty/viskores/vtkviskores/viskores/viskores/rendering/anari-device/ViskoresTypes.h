//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include <viskores/VecVariable.h>
#include <viskores/VectorAnalysis.h>

#include <limits>

namespace viskores_device
{

using float2 = viskores::Vec2f_32;
using float3 = viskores::Vec3f_32;
using float4 = viskores::Vec4f_32;

#if 0
struct float2
{
  float2(float v) {data[0]=v; data[1] = v;}
  float2(float v1, float v2) {data[0]=v1;data[1]=v2;}
  float operator[](std::size_t i) const {return data[i];}
  float& operator[](std::size_t i) {return data[i];}
  float x() const {return this->data[0];}
  float& x() {return this->data[0];}
  float y() const {return this->data[1];}
  float& y() {return this->data[1];}

  float data[2] = {0.f, 0.f};
};
struct float3
{
  float3(float v) {data[0]=v;data[1]=v;data[2]=v;}
  float3(float v1, float v2, float v3) {data[0]=v1;data[1]=v2;data[2]=v3;}
  float operator[](std::size_t i) const {return data[i];}
  float& operator[](std::size_t i) {return data[i];}
  float data[3];
};
struct float4
{
  float4(float v) {data[0]=v;data[1]=v;data[2]=v;data[3]=v;}
  float4(float v1, float v2, float v3, float v4) {data[0]=v1;data[1]=v2;data[2]=v3;data[3]=v4;}
  float4(const float3& f3, float v) {data[0]=f3[0];data[1]=f3[1];data[2]=f3[2];data[3]=v;}
  float operator[](std::size_t i) const {return data[i];}
  float& operator[](std::size_t i) {return data[i];}
  float data[4];
};
#endif

template <typename T>
struct range_t
{
  using element_t = T;

  range_t() = default;
  range_t(const T& t)
    : lower(t)
    , upper(t)
  {
  }
  range_t(const T& _lower, const T& _upper)
    : lower(_lower)
    , upper(_upper)
  {
  }

  range_t<T>& extend(const T& t)
  {
    lower = min(lower, t);
    upper = max(upper, t);
    return *this;
  }

  range_t<T>& extend(const range_t<T>& t)
  {
    lower = min(lower, t.lower);
    upper = max(upper, t.upper);
    return *this;
  }

  T lower{ T(std::numeric_limits<float>::max()) };
  T upper{ T(-std::numeric_limits<float>::max()) };
};

using box1 = range_t<float>;
using box2 = range_t<float2>;
using box3 = range_t<float3>;
using uint32_t = unsigned int;

//using float2 = float[2];
//using float3 = float[3];
//using float4 = float[4];

struct Ray
{
  // Ray //

  float3 org;
  float tnear{ 0.f };
  float3 dir;
  float time{ 0.f };
  float tfar{ std::numeric_limits<float>::max() };
  unsigned int mask{ ~0u };
  unsigned int id{ 0 };
  unsigned int flags{ 0 };

  // Hit //

  float3 Ng;
  float u;
  float v;
  unsigned int primID; //{RTC_INVALID_GEOMETRY_ID}; // primitive ID
  unsigned int geomID; //{RTC_INVALID_GEOMETRY_ID}; // geometry ID
  unsigned int instID; //{RTC_INVALID_GEOMETRY_ID}; // instance ID
};

struct Volume;
struct VolumeRay
{
  float3 org;
  float3 dir;
  box1 t{ 0.f, std::numeric_limits<float>::max() };
  Volume* volume{ nullptr };
  uint32_t instID; //{RTC_INVALID_GEOMETRY_ID};
};

} //namespace viskores_device
