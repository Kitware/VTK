// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPiecewiseFunction.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <limits>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPiecewiseFunction);

// The Node structure
class vtkPiecewiseFunctionNode
{
public:
  double X;
  double Y;
  double Sharpness;
  double Midpoint;
};

// A comparison method for sorting nodes in increasing order
class vtkPiecewiseFunctionCompareNodes
{
public:
  bool operator()(const vtkPiecewiseFunctionNode* node1, const vtkPiecewiseFunctionNode* node2)
  {
    return node1->X < node2->X;
  }
};

// A find method for finding nodes inside a specified range
class vtkPiecewiseFunctionFindNodeInRange
{
public:
  double X1;
  double X2;
  bool operator()(const vtkPiecewiseFunctionNode* node)
  {
    return (node->X >= this->X1 && node->X <= this->X2);
  }
};

// A find method for finding nodes outside a specified range
class vtkPiecewiseFunctionFindNodeOutOfRange
{
public:
  double X1;
  double X2;
  bool operator()(const vtkPiecewiseFunctionNode* node)
  {
    return (node->X < this->X1 || node->X > this->X2);
  }
};

// The internal structure for containing the STL objects
class vtkPiecewiseFunctionInternals
{
public:
  std::vector<vtkPiecewiseFunctionNode*> Nodes;
  vtkPiecewiseFunctionCompareNodes CompareNodes;
  vtkPiecewiseFunctionFindNodeInRange FindNodeInRange;
  vtkPiecewiseFunctionFindNodeOutOfRange FindNodeOutOfRange;
  vtkPiecewiseFunction::SearchMethod AutomaticSearchMethod = vtkPiecewiseFunction::BINARY_SEARCH;
  vtkPiecewiseFunction::SearchMethod CustomSearchMethod = vtkPiecewiseFunction::BINARY_SEARCH;
  bool UseCustomSearchMethod = false;
  std::vector<vtkPiecewiseFunctionNode*>::iterator UpperBound(vtkPiecewiseFunctionNode* node);
  std::vector<vtkPiecewiseFunctionNode*>::iterator InterpolationSearch(
    vtkPiecewiseFunctionNode* node);
};

//------------------------------------------------------------------------------
vtkPiecewiseFunction::vtkPiecewiseFunction()
{
  this->Clamping = 1;
  this->Range[0] = 0;
  this->Range[1] = 0;

  this->Function = nullptr;

  this->AllowDuplicateScalars = 0;
  this->UseLogScale = false;

  this->Internal = new vtkPiecewiseFunctionInternals;
}

//------------------------------------------------------------------------------
vtkPiecewiseFunction::~vtkPiecewiseFunction()
{
  delete[] this->Function;

  for (unsigned int i = 0; i < this->Internal->Nodes.size(); i++)
  {
    delete this->Internal->Nodes[i];
  }
  this->Internal->Nodes.clear();
  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::DeepCopy(vtkDataObject* o)
{
  vtkPiecewiseFunction* f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != nullptr)
  {
    this->Clamping = f->Clamping;
    int i;
    this->RemoveAllPoints();
    for (i = 0; i < f->GetSize(); i++)
    {
      double val[4];
      int isInRange = f->GetNodeValue(i, val);
      assert(isInRange == 1);
      (void)isInRange;
      this->AddPoint(val[0], val[1], val[2], val[3]);
    }
    this->Modified();
  }

  // Do the superclass
  this->Superclass::DeepCopy(o);
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::ShallowCopy(vtkDataObject* o)
{
  vtkPiecewiseFunction* f = vtkPiecewiseFunction::SafeDownCast(o);

  if (f != nullptr)
  {
    this->Clamping = f->Clamping;
    int i;
    this->RemoveAllPoints();
    for (i = 0; i < f->GetSize(); i++)
    {
      double val[4];
      int isInRange = f->GetNodeValue(i, val);
      assert(isInRange == 1);
      (void)isInRange;
      this->AddPoint(val[0], val[1], val[2], val[3]);
    }
    this->Modified();
  }

  // Do the superclass
  this->vtkDataObject::ShallowCopy(o);
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::Initialize()
{
  this->RemoveAllPoints();
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::GetSize()
{
  return static_cast<int>(this->Internal->Nodes.size());
}

//------------------------------------------------------------------------------
const char* vtkPiecewiseFunction::GetType()
{
  unsigned int i;
  double value;
  double prev_value = 0.0;
  int function_type;

  function_type = 0;

  if (!this->Internal->Nodes.empty())
  {
    prev_value = this->Internal->Nodes[0]->Y;
  }

  for (i = 1; i < this->Internal->Nodes.size(); i++)
  {
    value = this->Internal->Nodes[i]->Y;

    // Do not change the function type if equal
    if (value != prev_value)
    {
      if (value > prev_value)
      {
        switch (function_type)
        {
          case 0:
          case 1:
            function_type = 1; // NonDecreasing
            break;
          case 2:
            function_type = 3; // Varied
            break;
        }
      }
      else // value < prev_value
      {
        switch (function_type)
        {
          case 0:
          case 2:
            function_type = 2; // NonIncreasing
            break;
          case 1:
            function_type = 3; // Varied
            break;
        }
      }
    }

    prev_value = value;

    // Exit loop if we find a Varied function
    if (function_type == 3)
    {
      break;
    }
  }

  switch (function_type)
  {
    case 0:
      return "Constant";
    case 1:
      return "NonDecreasing";
    case 2:
      return "NonIncreasing";
    case 3:
      return "Varied";
  }

  return "Unknown";
}

//------------------------------------------------------------------------------
double* vtkPiecewiseFunction::GetDataPointer()
{
  // Since we no longer store the data in an array, we must
  // copy out of the vector into an array. No modified check -
  // could be added if performance is a problem
  int size = static_cast<int>(this->Internal->Nodes.size());

  delete[] this->Function;
  this->Function = nullptr;

  if (size > 0)
  {
    this->Function = new double[size * 2];
    for (int i = 0; i < size; i++)
    {
      this->Function[2 * i] = this->Internal->Nodes[i]->X;
      this->Function[2 * i + 1] = this->Internal->Nodes[i]->Y;
    }
  }

  return this->Function;
}

//------------------------------------------------------------------------------
double vtkPiecewiseFunction::GetFirstNonZeroValue()
{
  // Check if no points specified
  if (this->Internal->Nodes.empty())
  {
    return 0;
  }

  unsigned int i;
  int all_zero = 1;
  double x = 0.0;
  for (i = 0; i < this->Internal->Nodes.size(); i++)
  {
    if (this->Internal->Nodes[i]->Y != 0.0)
    {
      all_zero = 0;
      break;
    }
  }

  // If every specified point has a zero value then return
  // a large value
  if (all_zero)
  {
    x = VTK_DOUBLE_MAX;
  }
  else // A point was found with a non-zero value
  {
    if (i > 0)
    // Return the value of the point that precedes this one
    {
      x = this->Internal->Nodes[i - 1]->X;
    }
    else
    // If this is the first point in the function, return its
    // value is clamping is off, otherwise VTK_DOUBLE_MIN if
    // clamping is on.
    {
      if (this->Clamping)
      {
        x = VTK_DOUBLE_MIN;
      }
      else
      {
        x = this->Internal->Nodes[0]->X;
      }
    }
  }

  return x;
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::GetNodeValue(int index, double val[4])
{
  int size = static_cast<int>(this->Internal->Nodes.size());

  if (index < 0 || index >= size)
  {
    vtkErrorMacro("Index out of range!");
    return -1;
  }

  val[0] = this->Internal->Nodes[index]->X;
  val[1] = this->Internal->Nodes[index]->Y;
  val[2] = this->Internal->Nodes[index]->Midpoint;
  val[3] = this->Internal->Nodes[index]->Sharpness;

  return 1;
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::SetNodeValue(int index, double val[4])
{
  int size = static_cast<int>(this->Internal->Nodes.size());

  if (index < 0 || index >= size)
  {
    vtkErrorMacro("Index out of range!");
    return -1;
  }

  double oldX = this->Internal->Nodes[index]->X;
  this->Internal->Nodes[index]->X = val[0];
  this->Internal->Nodes[index]->Y = val[1];
  this->Internal->Nodes[index]->Midpoint = val[2];
  this->Internal->Nodes[index]->Sharpness = val[3];

  if (oldX != val[0])
  {
    // The point has been moved, the order of points or the range might have
    // been modified.
    this->SortAndUpdateRange();
    // No need to call Modified() here because SortAndUpdateRange() has done it
    // already.
  }
  else
  {
    this->Modified();
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::AddPoint(double x, double y)
{
  return this->AddPoint(x, y, 0.5, 0.0);
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::AddPoint(double x, double y, double midpoint, double sharpness)
{
  // Error check
  if (midpoint < 0.0 || midpoint > 1.0)
  {
    vtkErrorMacro("Midpoint outside range [0.0, 1.0]");
    return -1;
  }

  if (sharpness < 0.0 || sharpness > 1.0)
  {
    vtkErrorMacro("Sharpness outside range [0.0, 1.0]");
    return -1;
  }

  // remove any node already at this X location
  if (!this->AllowDuplicateScalars)
  {
    this->RemovePoint(x);
  }

  // Create the new node
  vtkPiecewiseFunctionNode* node = new vtkPiecewiseFunctionNode;
  node->X = x;
  node->Y = y;
  node->Sharpness = sharpness;
  node->Midpoint = midpoint;

  // Add it, then sort to get everything in order
  this->Internal->Nodes.push_back(node);
  this->SortAndUpdateRange();

  // Now find this node so we can return the index
  unsigned int i;
  for (i = 0; i < this->Internal->Nodes.size(); i++)
  {
    if (this->Internal->Nodes[i]->X == x && this->Internal->Nodes[i]->Y == y)
    {
      break;
    }
  }

  int retVal;

  // If we didn't find it, something went horribly wrong so
  // return -1
  if (i < this->Internal->Nodes.size())
  {
    retVal = i;
  }
  else
  {
    retVal = -1;
  }

  return retVal;
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::SortAndUpdateRange(bool updateSearchMethod)
{
  // Use stable_sort to avoid shuffling of DuplicateScalars
  std::stable_sort(
    this->Internal->Nodes.begin(), this->Internal->Nodes.end(), this->Internal->CompareNodes);
  bool modifiedInvoked = this->UpdateRange();
  // If range is updated, Modified() has been called, don't call it again.
  if (!modifiedInvoked)
  {
    this->Modified();
  }

  if (updateSearchMethod)
  {
    this->UpdateSearchMethod();
  }
}

//------------------------------------------------------------------------------
bool vtkPiecewiseFunction::UpdateRange()
{
  double oldRange[2];
  oldRange[0] = this->Range[0];
  oldRange[1] = this->Range[1];

  int size = static_cast<int>(this->Internal->Nodes.size());
  if (size)
  {
    this->Range[0] = this->Internal->Nodes[0]->X;
    this->Range[1] = this->Internal->Nodes[size - 1]->X;
  }
  else
  {
    this->Range[0] = 0;
    this->Range[1] = 0;
  }
  // If the rage is the same, then no need to call Modified()
  if (oldRange[0] == this->Range[0] && oldRange[1] == this->Range[1])
  {
    return false;
  }

  this->Modified();
  return true;
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::RemovePoint(double x)
{
  // First find the node since we need to know its
  // index as our return value
  size_t i;
  for (i = 0; i < this->Internal->Nodes.size(); i++)
  {
    if (this->Internal->Nodes[i]->X == x)
    {
      break;
    }
  }

  // If the node doesn't exist, we return -1
  if (i == this->Internal->Nodes.size())
  {
    return -1;
  }

  this->RemovePointByIndex(i);
  return static_cast<int>(i);
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::RemovePoint(double x, double y)
{
  size_t i;
  for (i = 0; i < this->Internal->Nodes.size(); i++)
  {
    if (this->Internal->Nodes[i]->X == x && this->Internal->Nodes[i]->Y == y)
    {
      break;
    }
  }

  // If the node doesn't exist, we return -1
  if (i == this->Internal->Nodes.size())
  {
    return -1;
  }

  this->RemovePointByIndex(i);
  return static_cast<int>(i);
}

//------------------------------------------------------------------------------
bool vtkPiecewiseFunction::RemovePointByIndex(size_t id)
{
  if (id > this->Internal->Nodes.size())
  {
    return false;
  }

  delete this->Internal->Nodes[id];
  this->Internal->Nodes.erase(this->Internal->Nodes.begin() + id);

  // if the first or last point has been removed, then we update the range
  // No need to sort here as the order of points hasn't changed.
  bool modifiedInvoked = false;
  if (id == 0 || id == this->Internal->Nodes.size())
  {
    modifiedInvoked = this->UpdateRange();
  }
  if (!modifiedInvoked)
  {
    this->Modified();
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::RemoveAllPoints()
{
  for (unsigned int i = 0; i < this->Internal->Nodes.size(); i++)
  {
    delete this->Internal->Nodes[i];
  }
  this->Internal->Nodes.clear();

  this->SortAndUpdateRange(false);
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::AddSegment(double x1, double y1, double x2, double y2)
{
  int done;

  // First, find all points in this range and remove them
  done = 0;
  while (!done)
  {
    done = 1;

    this->Internal->FindNodeInRange.X1 = x1;
    this->Internal->FindNodeInRange.X2 = x2;

    std::vector<vtkPiecewiseFunctionNode*>::iterator iter = std::find_if(
      this->Internal->Nodes.begin(), this->Internal->Nodes.end(), this->Internal->FindNodeInRange);

    if (iter != this->Internal->Nodes.end())
    {
      delete *iter;
      this->Internal->Nodes.erase(iter);
      this->Modified();
      done = 0;
    }
  }

  // Now add the points
  this->AddPoint(x1, y1, 0.5, 0.0);
  this->AddPoint(x2, y2, 0.5, 0.0);
}

//------------------------------------------------------------------------------
double vtkPiecewiseFunction::GetValue(double x)
{
  double table[1];
  this->GetTable(x, x, 1, table);
  return table[0];
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::AdjustRange(double range[2])
{
  if (!range)
  {
    return 0;
  }

  double* function_range = this->GetRange();

  // Make sure we have points at each end of the range

  if (function_range[0] < range[0])
  {
    this->AddPoint(range[0], this->GetValue(range[0]));
  }
  else
  {
    this->AddPoint(range[0], this->GetValue(function_range[0]));
  }

  if (function_range[1] > range[1])
  {
    this->AddPoint(range[1], this->GetValue(range[1]));
  }
  else
  {
    this->AddPoint(range[1], this->GetValue(function_range[1]));
  }

  // Remove all points out-of-range
  int done;

  done = 0;
  while (!done)
  {
    done = 1;

    this->Internal->FindNodeOutOfRange.X1 = range[0];
    this->Internal->FindNodeOutOfRange.X2 = range[1];

    std::vector<vtkPiecewiseFunctionNode*>::iterator iter =
      std::find_if(this->Internal->Nodes.begin(), this->Internal->Nodes.end(),
        this->Internal->FindNodeOutOfRange);

    if (iter != this->Internal->Nodes.end())
    {
      delete *iter;
      this->Internal->Nodes.erase(iter);
      this->Modified();
      done = 0;
    }
  }

  this->SortAndUpdateRange();
  return 1;
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::EstimateMinNumberOfSamples(double const& x1, double const& x2)
{
  double const d = this->FindMinimumXDistance();
  int idealWidth = static_cast<int>(ceil((x2 - x1) / d));

  return idealWidth;
}

//------------------------------------------------------------------------------
double vtkPiecewiseFunction::FindMinimumXDistance()
{
  std::vector<vtkPiecewiseFunctionNode*> const& nodes = this->Internal->Nodes;
  size_t const size = nodes.size();
  if (size < 2)
    return -1.0;

  double distance = std::numeric_limits<double>::max();
  for (size_t i = 0; i < size - 1; i++)
  {
    double const currentDist = nodes[i + 1]->X - nodes[i]->X;
    if (currentDist < distance)
    {
      distance = currentDist;
    }
  }

  return distance;
}

//------------------------------------------------------------------------------
std::vector<vtkPiecewiseFunctionNode*>::iterator vtkPiecewiseFunctionInternals::InterpolationSearch(
  vtkPiecewiseFunctionNode* node)
{
  if (this->Nodes.empty())
  {
    return this->Nodes.end();
  }

  std::vector<vtkPiecewiseFunctionNode*>::iterator begin = this->Nodes.begin();
  std::vector<vtkPiecewiseFunctionNode*>::iterator end = this->Nodes.end();
  std::vector<vtkPiecewiseFunctionNode*>::iterator mid = this->Nodes.begin();
  std::vector<vtkPiecewiseFunctionNode*>::iterator lastNode = end - 1;

  if (node->X > (*lastNode)->X)
  {
    return this->Nodes.end();
  }

  // Determines if the last step was to the right
  // or to the left side of the dichotomy. It is used
  // to return the upper bound
  bool side = true;

  // Loop while the value of the node is between the begin and the end
  // and the bounds are not crossed
  while ((*begin)->X <= node->X && node->X <= (*lastNode)->X && begin != end)
  {
    // Interpolate between the endpoints to have an idea where the Node should lie
    double fraction = (node->X - (*begin)->X) / ((*lastNode)->X - (*begin)->X);

    // Compute the possible index according to the linear fraction computed
    mid = begin +
      std::iterator_traits<std::vector<vtkPiecewiseFunctionNode*>::iterator>::difference_type(
        fraction * (std::distance(begin, end) - 1));

    if ((*mid)->X < node->X)
    {
      begin = mid + 1;
      side = false;
    }
    else if (node->X > (*lastNode)->X)
    {
      end = mid;
      side = true;
    }
    else
    {
      return mid;
    }
  }

  return (side == false) ? mid + 1 : mid;
}

//------------------------------------------------------------------------------
std::vector<vtkPiecewiseFunctionNode*>::iterator vtkPiecewiseFunctionInternals::UpperBound(
  vtkPiecewiseFunctionNode* node)
{
  vtkPiecewiseFunctionCompareNodes comparator;
  vtkPiecewiseFunction::SearchMethod searchMethod = this->AutomaticSearchMethod;

  if (this->UseCustomSearchMethod)
  {
    searchMethod = this->CustomSearchMethod;
  }

  if (searchMethod == vtkPiecewiseFunction::BINARY_SEARCH)
  {
    return std::upper_bound(this->Nodes.begin(), this->Nodes.end(), node, comparator);
  }
  else if (searchMethod == vtkPiecewiseFunction::INTERPOLATION_SEARCH)
  {
    return this->InterpolationSearch(node);
  }
  else
  {
    vtkErrorWithObjectMacro(
      nullptr, "The search method should only be binary search or interpolation search.");
    return this->Nodes.begin();
  }
}

// Returns a table of function values evaluated at regular intervals
void vtkPiecewiseFunction::GetTable(
  double start, double end, int size, double* table, int stride, int logIncrements, double epsilon)
{
  int numNodes = static_cast<int>(this->Internal->Nodes.size());
  double* tptr = nullptr;
  double xLoc = 0.0;
  double xStart = start;
  double xEnd = end;

  // Need to keep track of the last value so that
  // we can fill in table locations past this with
  // this value if Clamping is On.
  double lastValue = 0.0;
  if (numNodes != 0)
  {
    lastValue = this->Internal->Nodes[numNodes - 1]->Y;
  }

  if (logIncrements)
  {
    xStart = std::log10(xStart);
    xEnd = std::log10(xEnd);
  }

  // For each table entry
  for (int i = 0; i < size; i++)
  {
    // Find our location in the table
    tptr = table + stride * i;

    // Find our X location. If we are taking only 1 sample, make
    // it halfway between start and end (usually start and end will
    // be the same in this case)
    if (size > 1)
    {
      xLoc = xStart + (static_cast<double>(i) / static_cast<double>(size - 1)) * (xEnd - xStart);
    }
    else
    {
      xLoc = 0.5 * (xStart + xEnd);
    }

    // Convert back into data space if xStart and xEnd are defined in log space:
    if (logIncrements)
    {
      xLoc = std::pow(10., xLoc);
    }

    vtkPiecewiseFunctionNode node;
    node.X = xLoc;
    std::vector<vtkPiecewiseFunctionNode*>::iterator lowBound;
    std::vector<vtkPiecewiseFunctionNode*>::iterator upBound;
    upBound = this->Internal->UpperBound(&node);

    // Are we at the end? If so, just use the last value
    if (upBound == this->Internal->Nodes.end())
    {
      *tptr = this->Clamping ? lastValue : 0.0;
    }
    // Are we before the first node? If so, duplicate this nodes values
    else if (upBound == this->Internal->Nodes.begin())
    {
      *tptr = this->Clamping ? this->Internal->Nodes[0]->Y : 0.0;
    }
    // If we are at a valid point index, fill in
    // the value at this node, and the one before (the
    // two that surround our current sample location)
    else
    {
      double x1, x2, y1, y2, midpoint, sharpness;

      lowBound = upBound - 1;
      x1 = (*lowBound)->X;
      x2 = (*upBound)->X;
      y1 = (*lowBound)->Y;
      y2 = (*upBound)->Y;

      // We only need the previous midpoint and sharpness
      // since these control this region
      midpoint = (*lowBound)->Midpoint;
      sharpness = (*lowBound)->Sharpness;

      // Move midpoint away from extreme ends of range to avoid
      // degenerate math
      if (midpoint < epsilon)
      {
        midpoint = epsilon;
      }

      if (midpoint > 1 - epsilon)
      {
        midpoint = 1 - epsilon;
      }

      // Our first attempt at a normalized location [0, 1] -
      // we will be modifying this based on midpoint and
      // sharpness to get the curve shape we want and to have
      // it pass through (y1+y2)/2 at the midpoint.
      double scale;
      if (this->UseLogScale)
      {
        // Don't modify x1/x2 -- these are not reset on each iteration.
        double xLog = std::log10(xLoc);
        double x1Log = std::log10(x1);
        double x2Log = std::log10(x2);
        scale = (xLog - x1Log) / (x2Log - x1Log);
      }
      else
      {
        scale = (xLoc - x1) / (x2 - x1);
      }

      // Readjust based on the midpoint - linear adjustment
      if (scale < midpoint)
      {
        scale = 0.5 * scale / midpoint;
      }
      else
      {
        scale = 0.5 + 0.5 * (scale - midpoint) / (1.0 - midpoint);
      }

      // override for sharpness > 0.99
      // In this case we just want piecewise constant
      if (sharpness > 0.99)
      {
        // Use the first value if we are below the midpoint
        // Use the second value at or above the midpoint
        *tptr = scale < 0.5 ? y1 : y2;
      }

      // Override for sharpness < 0.01
      // In this case we want piecewise linear
      if (sharpness < 0.01)
      {
        // Simple linear interpolation
        *tptr = (1 - scale) * y1 + scale * y2;
        continue;
      }

      // We have a sharpness between [0.01, 0.99] - we will
      // used a modified hermite curve interpolation where we
      // derive the slope based on the sharpness, and we compress
      // the curve non-linearly based on the sharpness

      // First, we will adjust our position based on sharpness in
      // order to make the curve sharper (closer to piecewise constant)
      if (scale < 0.5)
      {
        scale = 0.5 * std::pow(scale * 2, 1.0 + 10 * sharpness);
      }
      else if (scale > 0.5)
      {
        scale = 1.0 - 0.5 * std::pow((1.0 - scale) * 2, 1 + 10 * sharpness);
      }

      // Compute some coefficients we will need for the hermite curve
      // scale squared
      double ss = scale * scale;
      // scale cubed
      double sss = ss * scale;

      double h1 = 2 * sss - 3 * ss + 1;
      double h2 = -2 * sss + 3 * ss;
      double h3 = sss - 2 * ss + scale;
      double h4 = sss - ss;

      double slope;
      double t;

      // Use one slope for both end points
      slope = y2 - y1;
      t = (1.0 - sharpness) * slope;

      // Compute the value
      *tptr = h1 * y1 + h2 * y2 + h3 * t + h4 * t;

      // Final error check to make sure we don't go outside
      // the Y range
      double min = (y1 < y2) ? (y1) : (y2);
      double max = (y1 > y2) ? (y1) : (y2);

      *tptr = (*tptr < min) ? (min) : (*tptr);
      *tptr = (*tptr > max) ? (max) : (*tptr);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::GetTable(
  double xStart, double xEnd, int size, float* table, int stride, int logIncrements, double epsilon)
{
  double* tmpTable = new double[size];

  this->GetTable(xStart, xEnd, size, tmpTable, 1, logIncrements, epsilon);

  double* tmpPtr = tmpTable;
  float* tPtr = table;

  for (int i = 0; i < size; i++)
  {
    *tPtr = static_cast<float>(*tmpPtr);
    tPtr += stride;
    tmpPtr++;
  }

  delete[] tmpTable;
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::BuildFunctionFromTable(
  double xStart, double xEnd, int size, double* table, int stride)
{
  double inc = 0.0;
  double* tptr = table;

  this->RemoveAllPoints();

  if (size > 1)
  {
    inc = (xEnd - xStart) / static_cast<double>(size - 1);
  }

  int i;
  for (i = 0; i < size; i++)
  {
    vtkPiecewiseFunctionNode* node = new vtkPiecewiseFunctionNode;
    node->X = xStart + inc * i;
    node->Y = *tptr;
    node->Sharpness = 0.0;
    node->Midpoint = 0.5;

    this->Internal->Nodes.push_back(node);
    tptr += stride;
  }

  this->SortAndUpdateRange();
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::FillFromDataPointer(int nb, double* ptr)
{
  if (nb <= 0 || !ptr)
  {
    return;
  }

  this->RemoveAllPoints();

  double* inPtr = ptr;

  int i;
  for (i = 0; i < nb; i++)
  {
    vtkPiecewiseFunctionNode* node = new vtkPiecewiseFunctionNode;
    node->X = inPtr[0];
    node->Y = inPtr[1];
    node->Sharpness = 0.0;
    node->Midpoint = 0.5;

    this->Internal->Nodes.push_back(node);
    inPtr += 2;
  }

  this->SortAndUpdateRange();
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::UpdateSearchMethod(double epsilon, double thresh)
{
  double averageDiff = 0;
  double stdDiff = 0;
  double currDiff = 0;
  const size_t nodeCount = this->Internal->Nodes.size();

  if (nodeCount < 3)
  {
    this->Internal->AutomaticSearchMethod = BINARY_SEARCH;
    return;
  }

  // compute the mean of the sampling rate
  averageDiff = (this->Internal->Nodes[nodeCount - 1]->X - this->Internal->Nodes[0]->X) /
    static_cast<double>(nodeCount);

  // It should not happens since the piecewise function
  // can't have multiple node at the same X
  if (std::abs(averageDiff) < epsilon)
  {
    this->Internal->AutomaticSearchMethod = BINARY_SEARCH;
    return;
  }

  // compute the standard deviation of the sampling rate
  for (size_t k = 0; k < this->Internal->Nodes.size() - 1; ++k)
  {
    currDiff = this->Internal->Nodes[k + 1]->X - this->Internal->Nodes[k]->X;
    stdDiff += std::pow(currDiff - averageDiff, 2);
  }

  stdDiff /= std::max(static_cast<double>(this->Internal->Nodes.size() - 1), 1.0);
  stdDiff = std::sqrt(stdDiff);

  double C = std::abs(stdDiff / averageDiff);

  if (C < thresh)
  {
    this->Internal->AutomaticSearchMethod = INTERPOLATION_SEARCH;
  }
  else
  {
    this->Internal->AutomaticSearchMethod = BINARY_SEARCH;
  }
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::GetAutomaticSearchMethod()
{
  return static_cast<int>(this->Internal->AutomaticSearchMethod);
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::SetUseCustomSearchMethod(bool useCustomSearchMethod)
{
  this->Internal->UseCustomSearchMethod = useCustomSearchMethod;
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::SetCustomSearchMethod(int type)
{
  if (type < 0 || type >= static_cast<int>(MAX_ENUM))
  {
    vtkGenericWarningMacro("enum out of scope, binary search will be applied");

    // set to binary search because it is the most general searchMethod
    this->Internal->CustomSearchMethod = BINARY_SEARCH;
  }

  this->Internal->CustomSearchMethod = static_cast<SearchMethod>(type);
}

//------------------------------------------------------------------------------
int vtkPiecewiseFunction::GetCustomSearchMethod()
{
  return static_cast<int>(this->Internal->CustomSearchMethod);
}

//------------------------------------------------------------------------------
vtkPiecewiseFunction* vtkPiecewiseFunction::GetData(vtkInformation* info)
{
  return info ? vtkPiecewiseFunction::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkPiecewiseFunction* vtkPiecewiseFunction::GetData(vtkInformationVector* v, int i)
{
  return vtkPiecewiseFunction::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
void vtkPiecewiseFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  unsigned int i;

  os << indent << "Clamping: " << this->Clamping << endl;
  os << indent << "Range: [" << this->Range[0] << "," << this->Range[1] << "]" << endl;
  os << indent << "Function Points: " << this->Internal->Nodes.size() << endl;
  for (i = 0; i < this->Internal->Nodes.size(); i++)
  {
    os << indent << "  " << i << " X: " << this->Internal->Nodes[i]->X
       << " Y: " << this->Internal->Nodes[i]->Y
       << " Sharpness: " << this->Internal->Nodes[i]->Sharpness
       << " Midpoint: " << this->Internal->Nodes[i]->Midpoint << endl;
  }
  os << indent << "AllowDuplicateScalars: " << this->AllowDuplicateScalars << endl;
  os << indent << "UseLogScale: " << this->UseLogScale << endl;
}
VTK_ABI_NAMESPACE_END
