// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTupleInterpolator.h"
#include "vtkKochanekSpline.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSpline.h"

#include <algorithm>
#include <cstddef>
#include <iostream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTupleInterpolator);

//------------------------------------------------------------------------------
vtkTupleInterpolator::vtkTupleInterpolator()
{
  // Set up the interpolation
  this->NumberOfComponents = 0;
  this->InterpolationType = INTERPOLATION_TYPE_SPLINE;
  this->InterpolatingSpline = nullptr;

  this->Spline = nullptr;
  this->Linear = nullptr;
}

//------------------------------------------------------------------------------
vtkTupleInterpolator::~vtkTupleInterpolator()
{
  this->Initialize();
  if (this->InterpolatingSpline)
  {
    this->InterpolatingSpline->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::SetNumberOfComponents(int numComp)
{
  numComp = (numComp < 1 ? 1 : numComp);
  if (numComp != this->NumberOfComponents)
  {
    this->ReinitializeInterpolation(numComp);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkTupleInterpolator::GetNumberOfTuples()
{
  if (this->Spline)
  {
    return this->Spline[0]->GetNumberOfPoints();
  }
  else if (this->Linear)
  {
    return this->Linear[0]->GetSize();
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
double vtkTupleInterpolator::GetMinimumT()
{
  if (this->Spline)
  {
    double range[2];
    this->Spline[0]->GetParametricRange(range);
    return range[0];
  }
  else if (this->Linear)
  {
    return this->Linear[0]->GetRange()[0];
  }
  else
  {
    return 0.0;
  }
}

//------------------------------------------------------------------------------
double vtkTupleInterpolator::GetMaximumT()
{
  if (this->Spline)
  {
    double range[2];
    this->Spline[0]->GetParametricRange(range);
    return range[1];
  }
  else if (this->Linear)
  {
    return this->Linear[0]->GetRange()[1];
  }
  else
  {
    return 1.0;
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::Initialize()
{
  int i;

  // Wipe out old data
  if (this->Spline)
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Spline[i]->Delete();
    }
    delete[] this->Spline;
    this->Spline = nullptr;
  }
  if (this->Linear)
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Linear[i]->Delete();
    }
    delete[] this->Linear;
    this->Linear = nullptr;
  }

  this->NumberOfComponents = 0;
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::FillFromData(int nb, double* time, double** data, bool isSOADataArray)
{
  if (nb <= 0 || !time || !data)
  {
    return;
  }

  // ptr will contains the data relative to the
  // current tuple dimension (current components)
  // and the corresponding time data. That is why
  // it is initialized with 2 * nb values
  // The time/tuple data are interlaced
  // to be consistent with the method
  // FillFromDataPointer from the class
  // vtkPieceWiseFunction
  std::vector<double> ptr(2 * nb, 0);

  for (int j = 0; j < nb; ++j)
  {
    // Interlacing of time / tuple data
    // we fill the time data only once since
    // all the dimension share the same time entry
    ptr[2 * j] = time[j];
  }

  for (int i = 0; i < this->NumberOfComponents; i++)
  {
    for (int j = 0; j < nb; j++)
    {
      // Interlacing of time / tuple data
      if (isSOADataArray)
      {
        ptr[2 * j + 1] = data[i][j];
      }
      else
      {
        ptr[2 * j + 1] = data[j][i];
      }
    }

    if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR && this->Linear[i])
    {
      this->Linear[i]->FillFromDataPointer(nb, ptr.data());
    }
    else if (this->InterpolationType == INTERPOLATION_TYPE_SPLINE && this->Spline[i])
    {
      this->Spline[i]->FillFromDataPointer(nb, ptr.data());
    }
    else
    {
      vtkWarningMacro(<< "Interpolation initialization failed for " << this->NumberOfComponents
                      << " components.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::InitializeInterpolation()
{
  // Prepare for new data
  if (this->NumberOfComponents <= 0)
  {
    return;
  }

  int i;
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    this->Linear = new vtkPiecewiseFunction*[this->NumberOfComponents];
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Linear[i] = vtkPiecewiseFunction::New();
    }
  }

  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
  {
    this->Spline = new vtkSpline*[this->NumberOfComponents];
    if (!this->InterpolatingSpline)
    {
      this->InterpolatingSpline = vtkKochanekSpline::New();
    }
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Spline[i] = this->InterpolatingSpline->NewInstance();
      this->Spline[i]->DeepCopy(this->InterpolatingSpline);
      this->Spline[i]->RemoveAllPoints();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::SetInterpolationType(int type)
{
  type = std::clamp<int>(type, INTERPOLATION_TYPE_LINEAR, INTERPOLATION_TYPE_SPLINE);
  if (type != this->InterpolationType)
  {
    // Snapshot the samples before the interpolation functions are replaced.
    const std::vector<double> samples = this->GetTimedTuples();
    const int numComp = this->NumberOfComponents;
    this->Initialize(); // wipe out the interpolation functions
    this->InterpolationType = type;
    this->NumberOfComponents = numComp;
    this->InitializeInterpolation();
    this->RestoreTimedTuples(samples, numComp);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::SetInterpolatingSpline(vtkSpline* spline)
{
  if (this->InterpolatingSpline == spline)
  {
    return;
  }
  if (this->InterpolatingSpline)
  {
    this->InterpolatingSpline->UnRegister(this);
    this->InterpolatingSpline = nullptr;
  }
  if (spline)
  {
    spline->Register(this);
  }
  this->InterpolatingSpline = spline;
  // The per-component splines are copies of the prototype, so they have to be
  // built again. Their samples are carried over.
  if (this->InterpolationType == INTERPOLATION_TYPE_SPLINE && this->NumberOfComponents > 0)
  {
    this->ReinitializeInterpolation(this->NumberOfComponents);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::AddTuple(double t, double tuple[])
{
  int i;
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Linear[i]->AddPoint(t, tuple[i]);
    }
  }

  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Spline[i]->AddPoint(t, tuple[i]);
    }
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::RemoveTuple(double t)
{
  int i;
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Linear[i]->RemovePoint(t);
    }
  }

  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      this->Spline[i]->RemovePoint(t);
    }
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::InterpolateTuple(double t, double tuple[])
{
  if (this->NumberOfComponents <= 0)
  {
    return;
  }

  int i;
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    double* range = this->Linear[0]->GetRange();
    t = std::clamp(t, range[0], range[1]);
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      tuple[i] = this->Linear[i]->GetValue(t);
    }
  }

  else // this->InterpolationType == INTERPOLATION_TYPE_SPLINE
  {
    for (i = 0; i < this->NumberOfComponents; i++)
    {
      tuple[i] = this->Spline[i]->Evaluate(t);
    }
  }
}

//------------------------------------------------------------------------------
vtkPiecewiseFunction* vtkTupleInterpolator::GetComponentFunction(int i) const
{
  if (i < 0 || i >= this->NumberOfComponents)
  {
    return nullptr;
  }
  if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR)
  {
    return this->Linear ? this->Linear[i] : nullptr;
  }
  return (this->Spline && this->Spline[i]) ? this->Spline[i]->GetPiecewiseFunction() : nullptr;
}

//------------------------------------------------------------------------------
std::vector<double> vtkTupleInterpolator::GetTimedTuples() const
{
  std::vector<double> result;
  vtkPiecewiseFunction* first = this->GetComponentFunction(0);
  if (!first)
  {
    return result;
  }
  const int numTuples = first->GetSize();
  if (numTuples <= 0)
  {
    return result;
  }
  const std::size_t stride = static_cast<std::size_t>(this->NumberOfComponents) + 1;
  result.resize(static_cast<std::size_t>(numTuples) * stride, 0.0);

  // All components share the same parameter values, take them from the first.
  double node[4];
  for (int j = 0; j < numTuples; j++)
  {
    first->GetNodeValue(j, node);
    result[static_cast<std::size_t>(j) * stride] = node[0];
  }
  for (int i = 0; i < this->NumberOfComponents; i++)
  {
    vtkPiecewiseFunction* function = this->GetComponentFunction(i);
    if (!function)
    {
      continue;
    }
    const int size = std::min(numTuples, function->GetSize());
    for (int j = 0; j < size; j++)
    {
      function->GetNodeValue(j, node);
      result[static_cast<std::size_t>(j) * stride + 1 + i] = node[1];
    }
  }
  return result;
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::SetTimedTuples(const std::vector<double>& values)
{
  if (this->NumberOfComponents <= 0)
  {
    vtkErrorMacro(<< "Set the number of components before setting the timed tuples.");
    return;
  }
  const std::size_t stride = static_cast<std::size_t>(this->NumberOfComponents) + 1;
  if ((values.size() % stride) != 0)
  {
    vtkErrorMacro(<< "The length of the timed tuples array is not divisible by " << stride
                  << ". Expects a sequence of t0, c0_0, ..., c0_" << (this->NumberOfComponents - 1)
                  << ", t1, etc");
    return;
  }
  if (values.empty())
  {
    for (int i = 0; i < this->NumberOfComponents; i++)
    {
      if (vtkPiecewiseFunction* function = this->GetComponentFunction(i))
      {
        function->RemoveAllPoints();
      }
    }
  }
  else
  {
    this->RestoreTimedTuples(values, this->NumberOfComponents);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::RestoreTimedTuples(const std::vector<double>& values, int numComp)
{
  if (values.empty() || numComp <= 0 || this->NumberOfComponents <= 0)
  {
    return;
  }
  const std::size_t stride = static_cast<std::size_t>(numComp) + 1;
  if ((values.size() % stride) != 0)
  {
    vtkErrorMacro(<< "The length of the timed tuples array is not divisible by " << stride << '.');
    return;
  }
  const int numTuples = static_cast<int>(values.size() / stride);

  // Interlaced time/value pairs, as expected by FillFromDataPointer.
  std::vector<double> ptr(2 * static_cast<std::size_t>(numTuples), 0.0);
  for (int j = 0; j < numTuples; j++)
  {
    ptr[2 * static_cast<std::size_t>(j)] = values[static_cast<std::size_t>(j) * stride];
  }
  for (int i = 0; i < this->NumberOfComponents; i++)
  {
    for (int j = 0; j < numTuples; j++)
    {
      // Components beyond the ones present in `values` are zero-filled.
      ptr[2 * static_cast<std::size_t>(j) + 1] =
        (i < numComp) ? values[static_cast<std::size_t>(j) * stride + 1 + i] : 0.0;
    }
    if (this->InterpolationType == INTERPOLATION_TYPE_LINEAR && this->Linear && this->Linear[i])
    {
      this->Linear[i]->FillFromDataPointer(numTuples, ptr.data());
    }
    else if (this->InterpolationType == INTERPOLATION_TYPE_SPLINE && this->Spline &&
      this->Spline[i])
    {
      this->Spline[i]->FillFromDataPointer(numTuples, ptr.data());
    }
    else
    {
      vtkWarningMacro(<< "Interpolation initialization failed for " << this->NumberOfComponents
                      << " components.");
    }
  }
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::ReinitializeInterpolation(int numComp)
{
  const std::vector<double> samples = this->GetTimedTuples();
  const int oldNumComp = this->NumberOfComponents;
  this->Initialize(); // wipe out the interpolation functions
  this->NumberOfComponents = numComp;
  this->InitializeInterpolation();
  this->RestoreTimedTuples(samples, oldNumComp);
}

//------------------------------------------------------------------------------
std::vector<double> vtkTupleInterpolator::InterpolateTuple(double t)
{
  std::vector<double> result(this->NumberOfComponents);
  this->InterpolateTuple(t, result.data());
  return result;
}

//------------------------------------------------------------------------------
void vtkTupleInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "There are " << this->GetNumberOfTuples() << " tuples to be interpolated\n";

  os << indent << "Number of Components: " << this->NumberOfComponents << "\n";

  os << indent << "Interpolation Type: "
     << (this->InterpolationType == INTERPOLATION_TYPE_LINEAR ? "Linear\n" : "Spline\n");

  os << indent << "Interpolating Spline: ";
  if (this->InterpolatingSpline)
  {
    os << this->InterpolatingSpline << "\n";
  }
  else
  {
    os << "(null)\n";
  }
}
VTK_ABI_NAMESPACE_END
