/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsInternalCommon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMPToolsInternalCommon_h
#define vtkSMPToolsInternalCommon_h

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef __VTK_WRAP__
namespace vtk
{
namespace detail
{
namespace smp
{

template <typename InputIt, typename OutputIt, typename Functor>
class TransformCall
{
  InputIt In;
  OutputIt Out;
  Functor& Transform;

public:
  TransformCall(InputIt _in, OutputIt _out, Functor& _transform)
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
      *itOut = Transform(*itIn, *itOut);
      ++itIn;
      ++itOut;
    }
  }
};

} // namespace smp
} // namespace detail
} // namespace vtk
#endif // __VTK_WRAP__
#endif // DOXYGEN_SHOULD_SKIP_THIS

#endif // VTK-HeaderTest-Exclude: vtkSMPToolsInternalCommon_h.h
