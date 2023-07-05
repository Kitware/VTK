// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTupleInterpolator.h"
#include "vtkKochanekSpline.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSpline.h"

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
    this->Initialize(); // wipe out data
    this->NumberOfComponents = numComp;
    this->InitializeInterpolation();
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
      vtkWarningMacro(<< "Interpolation initializaton failed for " << this->NumberOfComponents
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
  type = (type < INTERPOLATION_TYPE_LINEAR
      ? INTERPOLATION_TYPE_LINEAR
      : (type > INTERPOLATION_TYPE_SPLINE ? INTERPOLATION_TYPE_SPLINE : type));
  if (type != this->InterpolationType)
  {
    this->Initialize(); // wipe out data
    this->InterpolationType = type;
    this->InitializeInterpolation();
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
    t = (t < range[0] ? range[0] : (t > range[1] ? range[1] : t));
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
