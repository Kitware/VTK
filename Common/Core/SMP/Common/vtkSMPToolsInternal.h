// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPToolsInternal_h
#define vtkSMPToolsInternal_h

#include <iterator> // For std::advance

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace vtk
{
namespace detail
{
namespace smp
{
VTK_ABI_NAMESPACE_BEGIN

template <typename InputIt, typename OutputIt, typename Functor>
class UnaryTransformCall
{
protected:
  InputIt In;
  OutputIt Out;
  Functor& Transform;

public:
  UnaryTransformCall(InputIt _in, OutputIt _out, Functor& _transform)
    : In(_in)
    , Out(_out)
    , Transform(_transform)
  {
  }

  void Execute(vtkIdType begin, vtkIdType end)
  {
    InputIt itIn(In);
    OutputIt itOut(Out);
    std::advance(itIn, begin);
    std::advance(itOut, begin);
    for (vtkIdType it = begin; it < end; it++)
    {
      *itOut = Transform(*itIn);
      ++itIn;
      ++itOut;
    }
  }
};

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
class BinaryTransformCall : public UnaryTransformCall<InputIt1, OutputIt, Functor>
{
  InputIt2 In2;

public:
  BinaryTransformCall(InputIt1 _in1, InputIt2 _in2, OutputIt _out, Functor& _transform)
    : UnaryTransformCall<InputIt1, OutputIt, Functor>(_in1, _out, _transform)
    , In2(_in2)
  {
  }

  void Execute(vtkIdType begin, vtkIdType end)
  {
    InputIt1 itIn1(this->In);
    InputIt2 itIn2(In2);
    OutputIt itOut(this->Out);
    std::advance(itIn1, begin);
    std::advance(itIn2, begin);
    std::advance(itOut, begin);
    for (vtkIdType it = begin; it < end; it++)
    {
      *itOut = this->Transform(*itIn1, *itIn2);
      ++itIn1;
      ++itIn2;
      ++itOut;
    }
  }
};

template <typename T>
struct FillFunctor
{
  const T& Value;

public:
  FillFunctor(const T& _value)
    : Value(_value)
  {
  }

  T operator()(T vtkNotUsed(inValue)) { return Value; }
};

VTK_ABI_NAMESPACE_END

} // namespace smp
} // namespace detail
} // namespace vtk
#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif
/* VTK-HeaderTest-Exclude: vtkSMPToolsInternal.h */
