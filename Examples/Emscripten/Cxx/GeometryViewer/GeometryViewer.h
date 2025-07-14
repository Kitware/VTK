// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <memory>
#include <string>

class GeometryViewer
{
public:
  GeometryViewer();
  ~GeometryViewer();

  void LoadDataFileFromMemory(
    const std::string& filename, std::uintptr_t buffer, std::size_t nbytes);
  void LoadDataFile(const std::string& filename);
  void WriteDataFileToVirtualFS(
    const std::string& filename, std::uintptr_t buffer, std::size_t nbytes);

  void Initialize();
  void Render();
  void ResetView();
  void SetSize(int width, int height);
  void RemoveAllActors();

  void Start();
  void Halt();
  void Resume();

  void SetBackgroundColor1(int r, int g, int b);
  void SetBackgroundColor2(int r, int g, int b);
  void SetMouseWheelMotionFactor(float sensitivity);
  void SetUseOrthographicProjection(bool value);
  void SetDitherGradient(bool value);
  void SetHighlightOnHover(bool value, bool snapToPoint = false);

  void Azimuth(float value);

  void SetRepresentation(int representation);
  void SetVertexVisibility(bool visible);
  void SetRenderPointsAsSpheres(bool value);
  void SetPointSize(float value);
  void SetEdgeVisibility(bool visible);
  void SetRenderLinesAsTubes(bool value);
  void SetLineWidth(float value);

  void SetColorByArray(const std::string& arrayName);
  void SetInterpolateScalarsBeforeMapping(bool value);
  void SetColor(int r, int g, int b);
  void SetColorMapPreset(const std::string& presetName);
  void SetVertexColor(int r, int g, int b);
  void SetEdgeColor(int r, int g, int b);
  void SetOpacity(float value);
  void SetEdgeOpacity(float value);

  std::string GetPointDataArrays();
  std::string GetCellDataArrays();
  std::string GetColorMapPresets();

  void SaveScreenshotAsPNG();

private:
  class Internal;
  std::unique_ptr<Internal> P;
};
