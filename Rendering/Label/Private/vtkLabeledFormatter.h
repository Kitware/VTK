// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Private implementation header — not installed.

#ifndef vtkLabeledFormatter_h
#define vtkLabeledFormatter_h

#include "../vtkLabeledDataMapper.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkStringFormatter.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

/**
 * Resolved label input for one dataset/graph, produced by
 * vtkLabeledDataMapper::ResolveLabeledFormatterInput and consumed by
 * vtkLabeledFormatterInterface::Dispatch.
 */
struct vtkLabeledFormatterInput
{
  bool Valid = true;
  vtkIntArray* TypeArr = nullptr;
  int NumCurLabels = 0;
  vtkDataArray* NumericData = nullptr;
  vtkStringArray* StringData = nullptr;
  int NumComp = 0;
  int ActiveComp = 0;
  bool PointIdLabels = false;
  std::string FormatString;
};

/**
 * @brief Base class for label formatters.
 *
 * Subclasses implement SetFormattedString() to store or render each label.
 * Call Dispatch() to select the correct formatting path given a resolved
 * vtkLabeledFormatterInterfaceInput.
 */
struct vtkLabeledFormatterInterface
{
  vtkLabeledDataMapper* Self;
  vtkIntArray* TypeArr;
  int NumCurLabels;

  vtkLabeledFormatterInterface(vtkLabeledDataMapper* self, vtkIntArray* typeArr, int numCurLabels)
    : Self(self)
    , TypeArr(typeArr)
    , NumCurLabels(numCurLabels)
  {
  }

  virtual ~vtkLabeledFormatterInterface() = default;

  virtual void SetFormattedString(int i, const char* text) = 0;

  void operator()(const std::string& FormatString)
  {
    char formatedString[1024];
    for (int i = 0; i < this->NumCurLabels; i++)
    {
      VTK_FORMAT_IF_ERROR_RETURN(
        auto result = vtk::format_to_n(formatedString, sizeof(formatedString), FormatString, i);
        *result.out = '\0', );
      this->SetFormattedString(i, formatedString);
    }
  }

  struct NumericComponent
  {
  };
  template <class TArray>
  void operator()(TArray* array, int activeComp, const std::string& FormatString, NumericComponent)
  {
    char formatedString[1024];
    auto a = vtk::DataArrayTupleRange(array);
    using ValueType = vtk::GetAPIType<TArray>;
    for (int i = 0; i < this->NumCurLabels; i++)
    {
      VTK_FORMAT_IF_ERROR_RETURN(
        auto result = vtk::format_to_n(formatedString, sizeof(formatedString), FormatString,
          static_cast<ValueType>(a[i][activeComp]));
        *result.out = '\0', );
      this->SetFormattedString(i, formatedString);
    }
  }

  struct NumericVector
  {
  };
  template <class TArray>
  void operator()(TArray* array, int numComp, const std::string& FormatString, NumericVector)
  {
    char formatedString[1024];
    std::string ResultString;
    auto a = vtk::DataArrayTupleRange(array);
    using ValueType = vtk::GetAPIType<TArray>;
    for (int i = 0; i < this->NumCurLabels; i++)
    {
      ResultString = "(";
      for (int j = 0; j < numComp; ++j)
      {
        VTK_FORMAT_IF_ERROR_RETURN(
          auto result = vtk::format_to_n(
            formatedString, sizeof(formatedString), FormatString, static_cast<ValueType>(a[i][j]));
          *result.out = '\0', );
        ResultString += formatedString;
        if (j < (numComp - 1))
        {
          ResultString += this->Self->GetComponentSeparator();
        }
        else
        {
          ResultString += ')';
        }
      }
      this->SetFormattedString(i, ResultString.c_str());
    }
  }

  void operator()(vtkStringArray* array, const std::string& FormatString)
  {
    char formatedString[1024];
    for (int i = 0; i < this->NumCurLabels; i++)
    {
      const char* labelFormat = this->Self->GetLabelFormat();
      if (!labelFormat || std::string_view(labelFormat).empty())
      {
        this->SetFormattedString(i, array->GetValue(i).c_str());
      }
      else
      {
        VTK_FORMAT_IF_ERROR_RETURN(
          auto result = vtk::format_to_n(formatedString, sizeof(formatedString), FormatString,
            static_cast<std::string&>(array->GetValue(i)));
          *result.out = '\0', );
        this->SetFormattedString(i, formatedString);
      }
    }
  }

  void Dispatch(const vtkLabeledFormatterInput& input)
  {
    if (input.PointIdLabels)
    {
      (*this)(input.FormatString);
    }
    else if (input.NumericData)
    {
      if (input.NumComp == 1)
      {
        if (!vtkArrayDispatch::Dispatch::Execute(
              input.NumericData, *this, input.ActiveComp, input.FormatString, NumericComponent{}))
        {
          (*this)(input.NumericData, input.ActiveComp, input.FormatString, NumericComponent{});
        }
      }
      else
      {
        if (!vtkArrayDispatch::Dispatch::Execute(
              input.NumericData, *this, input.NumComp, input.FormatString, NumericVector{}))
        {
          (*this)(input.NumericData, input.NumComp, input.FormatString, NumericVector{});
        }
      }
    }
    else
    {
      (*this)(input.StringData, input.FormatString);
    }
  }
};

VTK_ABI_NAMESPACE_END
#endif
