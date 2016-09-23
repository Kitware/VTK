/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestRIB.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkRIBProperty.h"
#include "vtkRIBLight.h"
#include "vtkRIBExporter.h"
#include "vtkProperty.h"

#include "vtkTestUtilities.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStripper.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"

#include "vtkTestErrorObserver.h"

#include <sstream>

#define CHECK_ERROR_MSG(msg) \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
  { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << "\"" << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  errorObserver->Clear()

#define CHECK_WARNING_MSG(msg) \
  { \
  std::string expectedMsg(msg); \
  if (!warningObserver->GetWarning()) \
  { \
    std::cout << "Failed to catch any warning. Expected the warning message to contain \"" << expectedMsg << "\"" << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(warningObserver->GetWarningMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Warning message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  warningObserver->Clear()

#define TEST_SET_GET_VALUE( variable, command ) \
  if( variable != command )   \
  {   \
    std::cout << "Error in " << #command << std::endl; \
    std::cout << "  In " __FILE__ ", line " << __LINE__ << std::endl;   \
    std::cout << "Expected |" << variable << "|" << std::endl;           \
    std::cout << "but got  |" << command << "|" << std::endl;            \
    status += 1; \
  }

static int TestRIBProperty();
static int TestRIBLight();
static int TestRIBExporter();

int UnitTestRIB (int, char *[])
{
  int status = 0;
  status += TestRIBProperty();
  status += TestRIBLight();
  status += TestRIBExporter();

  if (status != 0)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

int TestRIBProperty()
{
  int status = 0;
  {
  std::cout << "RIBProperty empty Print..";
  std::ostringstream propPrint;
  vtkSmartPointer<vtkRIBProperty> prop =
    vtkSmartPointer<vtkRIBProperty>::New();
  prop->Print(propPrint);
  std::cout << ".PASSED" << std::endl;
  }
  {
  std::cout << "RIBProperty Set/Get..";
  vtkSmartPointer<vtkRIBProperty> prop =
    vtkSmartPointer<vtkRIBProperty>::New();
  prop->SetSurfaceShader("surfaceshader");
  TEST_SET_GET_VALUE(std::string("surfaceshader"),
                     std::string(prop->GetSurfaceShader()));
  prop->SetDisplacementShader("displacementshader");
  TEST_SET_GET_VALUE(std::string("displacementshader"),
                     std::string(prop->GetDisplacementShader()));
  prop->SetVariable("floatVar", "float");
  TEST_SET_GET_VALUE(std::string("Declare \"floatVar\" \"float\"\n"),
                     std::string(prop->GetDeclarations()));
  prop->AddVariable("colorVar", "color");
  TEST_SET_GET_VALUE(std::string(
                       "Declare \"floatVar\" \"float\"\n"
                       "Declare \"colorVar\" \"color\"\n"),
                     std::string(prop->GetDeclarations()));
  prop->SetVariable("vectorVar", "vector");
  TEST_SET_GET_VALUE(std::string("Declare \"vectorVar\" \"vector\"\n"),
                     std::string(prop->GetDeclarations()));
  prop->SetSurfaceShaderParameter("floatVar", "5.678");
  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [5.678]"),
                     std::string(prop->GetSurfaceShaderParameters()));
  prop->SetSurfaceShaderParameter("floatVar", "1.234");
  prop->AddSurfaceShaderParameter("colorVar", "1 .5 .1");
  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [1.234] \"colorVar\" [1 .5 .1]"),
                     std::string(prop->GetSurfaceShaderParameters()));
  prop->SetDisplacementShaderParameter("floatVar", "5.678");
  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [5.678]"),
                     std::string(prop->GetDisplacementShaderParameters()));
  prop->SetDisplacementShaderParameter("floatVar", "1.234");
  prop->AddDisplacementShaderParameter("colorVar", "1 .5 .1");
  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [1.234] \"colorVar\" [1 .5 .1]"),
                     std::string(prop->GetDisplacementShaderParameters()));
  prop->SurfaceShaderUsesDefaultParametersOn();
  TEST_SET_GET_VALUE(1,
                     prop->GetSurfaceShaderUsesDefaultParameters());
  prop->SurfaceShaderUsesDefaultParametersOff();
  TEST_SET_GET_VALUE(0,
                     prop->GetSurfaceShaderUsesDefaultParameters());

  std::cout << ".PASSED" << std::endl;
  std::cout << "RIBProperty Print..";
  std::ostringstream propPrint;
  prop->Print(propPrint);
  std::cout << ".PASSED" << std::endl;
  }
  {
  std::cout << "RIBProperty Deprecated Set/Get..";
  vtkSmartPointer<vtkRIBProperty> prop =
    vtkSmartPointer<vtkRIBProperty>::New();
  vtkSmartPointer<vtkTest::ErrorObserver>  warningObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  prop->AddObserver(vtkCommand::WarningEvent, warningObserver);
  prop->SetParameter("floatVar", "5.678");
  CHECK_WARNING_MSG("SetParameter is deprecated");

  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [5.678]"),
                     std::string(prop->GetParameters()));
  CHECK_WARNING_MSG("GetParameters is deprecated");

  prop->AddParameter("colorVar", "1 .5 .1");
  CHECK_WARNING_MSG("AddParameter is deprecated");
  TEST_SET_GET_VALUE(std::string(
                       " \"floatVar\" [5.678] \"colorVar\" [1 .5 .1]"),
                     std::string(prop->GetSurfaceShaderParameters()));
  std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestRIBLight()
{
  int status = 0;
  {
  std::cout << "RIBLight empty Print..";
  std::ostringstream lightPrint;
  vtkSmartPointer<vtkRIBLight> light =
    vtkSmartPointer<vtkRIBLight>::New();
  light->Print(lightPrint);
  light->ShadowsOff();
  TEST_SET_GET_VALUE(0,
                     light->GetShadows());
  light->ShadowsOn();
  TEST_SET_GET_VALUE(1,
                     light->GetShadows());

  std::cout << ".PASSED" << std::endl;
  }
  return status;
}

int TestRIBExporter()
{
  int status = 0;
  {
  std::cout << "RIBExporter empty Print..";
  std::ostringstream exporterPrint;
  vtkSmartPointer<vtkRIBExporter> exporter =
    vtkSmartPointer<vtkRIBExporter>::New();
  exporter->Print(exporterPrint);
  std::cout << ".PASSED" << std::endl;
  }
  {
  std::cout << "RIBExporter Set/Get..";
  vtkSmartPointer<vtkRIBExporter> exporter =
    vtkSmartPointer<vtkRIBExporter>::New();

  exporter->SetFilePrefix("foo");
  TEST_SET_GET_VALUE(std::string("foo"),
                     std::string(exporter->GetFilePrefix()));

  exporter->SetTexturePrefix("bar");
  TEST_SET_GET_VALUE(std::string("bar"),
                     std::string(exporter->GetTexturePrefix()));

  exporter->BackgroundOn();
  TEST_SET_GET_VALUE(1,
                     exporter->GetBackground());
  exporter->BackgroundOff();
  TEST_SET_GET_VALUE(0,
                     exporter->GetBackground());
  exporter->ExportArraysOn();
  TEST_SET_GET_VALUE(1,
                     exporter->GetExportArrays());
  exporter->ExportArraysOff();
  TEST_SET_GET_VALUE(0,
                     exporter->GetExportArrays());
  int sizeExpected[2] = {100,200};
  int sizeGot[2];
  exporter->SetSize(sizeExpected);
  exporter->GetSize(sizeGot);
  if (sizeExpected[0] != sizeGot[0] ||
      sizeExpected[1] != sizeGot[1])
  {
    std::cout << "Error in " << "GetSize" << std::endl;
    std::cout << "  In " __FILE__ ", line " << __LINE__ << std::endl;
    std::cout << "Expected |"
              << sizeExpected[0] << ", " << sizeExpected[1]
              << "|" << std::endl;
    std::cout << "but got  |"
              << sizeGot[0] << ", " << sizeGot[1]
              << "|" << std::endl;
    status += 1;
  }

  int samplesExpected[2] = {2, 3};
  int samplesGot[2];
  exporter->SetPixelSamples(samplesExpected);
  exporter->GetPixelSamples(samplesGot);
  if (samplesExpected[0] != samplesGot[0] ||
      samplesExpected[1] != samplesGot[1])
  {
    std::cout << "Error in " << "GetPixelSamples" << std::endl;
    std::cout << "  In " __FILE__ ", line " << __LINE__ << std::endl;
    std::cout << "Expected |"
              << samplesExpected[0] << ", " << samplesExpected[1]
              << "|" << std::endl;
    std::cout << "but got  |"
              << samplesGot[0] << ", " << samplesGot[1]
              << "|" << std::endl;
    status += 1;
  }

  std::cout << ".PASSED" << std::endl;
  }
  {
  std::cout << "RIBExporter Default Shader Update..";
  vtkSmartPointer<vtkSphereSource> sphereModel =
    vtkSmartPointer<vtkSphereSource>::New ();
  vtkSmartPointer<vtkStripper> stripModel =
    vtkSmartPointer<vtkStripper>::New();
  vtkSmartPointer<vtkRIBExporter> exporter =
    vtkSmartPointer<vtkRIBExporter>::New();
  vtkSmartPointer<vtkRIBProperty> prop =
    vtkSmartPointer<vtkRIBProperty>::New();
  vtkSmartPointer<vtkRIBProperty> prop2 =
    vtkSmartPointer<vtkRIBProperty>::New();

  vtkSmartPointer<vtkPolyDataMapper>  sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New ();
  sphereMapper->SetInputConnection(sphereModel->GetOutputPort());
  vtkSmartPointer<vtkActor> sphere =
    vtkSmartPointer<vtkActor>::New ();
  sphere->SetMapper(sphereMapper);

  stripModel->SetInputConnection(sphereModel->GetOutputPort());
  vtkSmartPointer<vtkPolyDataMapper>  stripMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New ();
  stripMapper->SetInputConnection(stripModel->GetOutputPort());
  vtkSmartPointer<vtkActor> strip =
    vtkSmartPointer<vtkActor>::New ();
  strip->SetMapper(stripMapper);
  strip->SetProperty(prop2);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New ();
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New ();
  ren1->AddActor(sphere);
  ren1->AddActor(strip);
  renWin->AddRenderer(ren1);
  renWin->Render();

  exporter->SetRenderWindow(renWin);
  exporter->SetFilePrefix("UnitTestRIBDefault");
  exporter->Update();
  std::cout << ".PASSED" << std::endl;

  std::cout << "RIBExporter SetSurfaceShader Update..";
  prop->SetVariable("k", "float");
  prop->SetDiffuse(.5);
  prop->SetSurfaceShaderParameter("k", "5");
  prop->SetSurfaceShader("bozo");
  sphere->SetProperty(prop);
  renWin->Render();
  exporter->SetFilePrefix("UnitTestRIBSetSurfaceShader");
  exporter->Update();
  std::cout << ".PASSED" << std::endl;

  std::cout << "RIBExporter SurfaceShaderUsesDefaultParametersOff Update..";
  prop->SurfaceShaderUsesDefaultParametersOff();
  prop->SetVariable("k", "float");
  prop->SetDiffuse(.5);
  prop->SetSurfaceShaderParameter("k", "5");
  prop->SetSurfaceShader("bozo");
  sphere->SetProperty(prop);
  renWin->Render();
  exporter->SetFilePrefix("UnitTestRIBSurfaceShaderUsesDefaultParametersOff");
  exporter->Update();
  std::cout << ".PASSED" << std::endl;

  std::cout << "RIBExporter SetDisplacmentShader Update..";
  prop->SetVariable("Km", "float");
  prop->SetDisplacementShaderParameter("Km", "2");
  prop->SetDisplacementShader("dented");
  sphere->SetProperty(prop);
  renWin->Render();
  exporter->SetFilePrefix("UnitTestRIBSetDisplacementShader");
  exporter->Update();
  std::cout << ".PASSED" << std::endl;

  std::cout << "RIBExporter Errors and Warnings..";
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  exporter->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  prop->SetRepresentationToWireframe();
  exporter->Update();
  CHECK_ERROR_MSG("Bad representation. Only Surface is supported.");

  prop->SetRepresentationToSurface();
  prop2->SetRepresentationToWireframe();
  exporter->Update();
  CHECK_ERROR_MSG("Bad representation. Only Surface is supported.");

  exporter->SetFilePrefix(NULL);
  exporter->Update();
  CHECK_ERROR_MSG("Please specify file name for the rib file");

  vtkSmartPointer<vtkRenderer> ren2 =
    vtkSmartPointer<vtkRenderer>::New ();
  renWin->AddRenderer(ren2);
  exporter->SetFilePrefix("dummy");
  exporter->Update();
  CHECK_ERROR_MSG("RIB files only support one renderer per window");

  renWin->RemoveRenderer(ren2);
  ren1->RemoveActor(sphere);
  ren1->RemoveActor(strip);
  exporter->Update();
  CHECK_ERROR_MSG("No actors found for writing .RIB file");

  std::cout << ".PASSED" << std::endl;

  }
  return status;
}
