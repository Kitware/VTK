// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkVersionMacros.h"

#include <cstdlib>
#include <vtksys/SystemTools.hxx>

//------------------------------------------------------------------------------
// Mock class that can be overridden by object factories
class vtkMockOverridable : public vtkObject
{
public:
  vtkTypeMacro(vtkMockOverridable, vtkObject);
  static vtkMockOverridable* New() { VTK_OBJECT_FACTORY_NEW_BODY(vtkMockOverridable); }

protected:
  vtkMockOverridable() = default;
  ~vtkMockOverridable() override = default;

private:
  vtkMockOverridable(const vtkMockOverridable&) = delete;
  void operator=(const vtkMockOverridable&) = delete;
};

//------------------------------------------------------------------------------
// MockObjectFactory1 overrides vtkMockOverridable with vtkMockOverrideClass1
class vtkMockOverrideClass1 : public vtkMockOverridable
{
public:
  vtkTypeMacro(vtkMockOverrideClass1, vtkMockOverridable);
  static vtkMockOverrideClass1* New() { VTK_STANDARD_NEW_BODY(vtkMockOverrideClass1); }

  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes()
  {
    auto attrB = vtkOverrideAttribute::CreateAttributeChain("AttributeB", "M", nullptr);
    auto attrA = vtkOverrideAttribute::CreateAttributeChain("AttributeA", "X", attrB);
    return attrA;
  }

protected:
  vtkMockOverrideClass1() = default;
  ~vtkMockOverrideClass1() override = default;

private:
  vtkMockOverrideClass1(const vtkMockOverrideClass1&) = delete;
  void operator=(const vtkMockOverrideClass1&) = delete;
};

// not necessary, but useful to show how it's done in real classes
#define vtkMockOverrideClass1_OVERRIDE_ATTRIBUTES vtkMockOverrideClass1::CreateOverrideAttributes()

VTK_CREATE_CREATE_FUNCTION(vtkMockOverrideClass1);

//------------------------------------------------------------------------------
class vtkMockObjectFactory1 : public vtkObjectFactory
{
public:
  static vtkMockObjectFactory1* New() { VTK_STANDARD_NEW_BODY(vtkMockObjectFactory1); }
  vtkTypeMacro(vtkMockObjectFactory1, vtkObjectFactory);

  const char* GetDescription() VTK_FUTURE_CONST override
  {
    return "vtkMockObjectFactory1 factory overrides.";
  }
  const char* GetVTKSourceVersion() VTK_FUTURE_CONST override { return VTK_SOURCE_VERSION; }

  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

protected:
  vtkMockObjectFactory1()
  {
    this->RegisterOverride("vtkMockOverridable", "vtkMockOverrideClass1", "Factory1", 1,
      vtkObjectFactoryCreatevtkMockOverrideClass1,
#if defined(vtkMockOverrideClass1_OVERRIDE_ATTRIBUTES)
      vtkMockOverrideClass1_OVERRIDE_ATTRIBUTES
#else
      nullptr
#endif
    );
  }
  ~vtkMockObjectFactory1() override = default;

private:
  vtkMockObjectFactory1(const vtkMockObjectFactory1&) = delete;
  void operator=(const vtkMockObjectFactory1&) = delete;
};

//------------------------------------------------------------------------------
// MockObjectFactory1 overrides vtkMockOverridable with vtkMockOverrideClass1
class vtkMockOverrideClass2 : public vtkMockOverridable
{
public:
  vtkTypeMacro(vtkMockOverrideClass2, vtkMockOverridable);
  static vtkMockOverrideClass2* New() { VTK_STANDARD_NEW_BODY(vtkMockOverrideClass2); }

  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes()
  {
    auto attrB = vtkOverrideAttribute::CreateAttributeChain("AttributeB", "N", nullptr);
    auto attrA = vtkOverrideAttribute::CreateAttributeChain("AttributeA", "Y", attrB);
    return attrA;
  }

protected:
  vtkMockOverrideClass2() = default;
  ~vtkMockOverrideClass2() override = default;

private:
  vtkMockOverrideClass2(const vtkMockOverrideClass2&) = delete;
  void operator=(const vtkMockOverrideClass2&) = delete;
};

// not necessary, but useful to show how it's done in real classes
#define vtkMockOverrideClass2_OVERRIDE_ATTRIBUTES vtkMockOverrideClass2::CreateOverrideAttributes()

VTK_CREATE_CREATE_FUNCTION(vtkMockOverrideClass2);

//------------------------------------------------------------------------------
class vtkMockObjectFactory2 : public vtkObjectFactory
{
public:
  static vtkMockObjectFactory2* New() { VTK_STANDARD_NEW_BODY(vtkMockObjectFactory2); }
  vtkTypeMacro(vtkMockObjectFactory2, vtkObjectFactory);

  const char* GetDescription() VTK_FUTURE_CONST override
  {
    return "vtkMockObjectFactory2 factory overrides.";
  }
  const char* GetVTKSourceVersion() VTK_FUTURE_CONST override { return VTK_SOURCE_VERSION; }

  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

protected:
  vtkMockObjectFactory2()
  {
    this->RegisterOverride("vtkMockOverridable", "vtkMockOverrideClass2", "Factory2", 1,
      vtkObjectFactoryCreatevtkMockOverrideClass2,
#if defined(vtkMockOverrideClass2_OVERRIDE_ATTRIBUTES)
      vtkMockOverrideClass2_OVERRIDE_ATTRIBUTES
#else
      nullptr
#endif
    );
  }
  ~vtkMockObjectFactory2() override = default;

private:
  vtkMockObjectFactory2(const vtkMockObjectFactory2&) = delete;
  void operator=(const vtkMockObjectFactory2&) = delete;
};

namespace
{
enum TestObjectFactoryOverrideAttributeCase
{
  Programatically = 0,
  FromCommandLine,
  FromEnvironmentVariable
};

class ScopedTestCasePreferences
{
  std::string PreviousPreferences;
  std::string PreviousEnvVarValue;
  int TestCase;

public:
  explicit ScopedTestCasePreferences(const std::string& preferences, int testCase)
    : PreviousPreferences(vtkObjectFactory::GetPreferences())
    , TestCase(testCase)
  {
    switch (testCase)
    {
      case Programatically:
        vtkObjectFactory::SetPreferences(preferences);
        vtkLog(INFO, "Setting vtkObjectFactory preferences programatically: " << preferences);
        break;
      case FromCommandLine:
      {
        vtkLog(
          INFO, "Setting vtkObjectFactory preferences from command line argument: " << preferences);
        const std::vector<const char*> args = { "TestObjectFactoryOverrideAttribute",
          "--vtk-factory-prefer", preferences.c_str() };
        char** argv = new char*[args.size() + 1];
        int argc = 0;
        for (const auto arg : args)
        {
          argv[argc++] = strdup(arg);
        }
        argv[argc] = nullptr; // sentinel
        vtkObjectFactory::InitializePreferencesFromCommandLineArgs(argc, argv);
        for (int i = 0; i < argc; ++i)
        {
          delete[] argv[i];
        }
        delete[] argv;
        break;
      }
      case FromEnvironmentVariable:
        vtkLog(
          INFO, "Setting vtkObjectFactory preferences from environment variable: " << preferences);
        if (vtksys::SystemTools::HasEnv("VTK_FACTORY_PREFER"))
        {
          PreviousEnvVarValue = vtksys::SystemTools::GetEnv("VTK_FACTORY_PREFER");
        }
        const std::string envVar = "VTK_FACTORY_PREFER=" + preferences;
        vtksys::SystemTools::PutEnv(envVar);
        break;
    }
  }
  ~ScopedTestCasePreferences()
  {
    // restore previous preferences
    vtkObjectFactory::SetPreferences(PreviousPreferences);
    if (this->TestCase == TestObjectFactoryOverrideAttributeCase::FromEnvironmentVariable)
    {
      // restore previous environment variable's value, or unset it
      if (!PreviousEnvVarValue.empty())
      {
        const std::string envVar = "VTK_FACTORY_PREFER=" + PreviousEnvVarValue;
        vtksys::SystemTools::PutEnv(envVar);
      }
      else
      {
        vtksys::SystemTools::UnPutEnv("VTK_FACTORY_PREFER");
      }
    }
  }
};
}

int TestObjectFactoryOverrideAttribute(int, char*[])
{
  vtkNew<vtkMockObjectFactory1> factory1;
  vtkObjectFactory::RegisterFactory(factory1);
  vtkNew<vtkMockObjectFactory2> factory2;
  vtkObjectFactory::RegisterFactory(factory2);
  bool success = true;
  for (int i = TestObjectFactoryOverrideAttributeCase::Programatically;
       i <= TestObjectFactoryOverrideAttributeCase::FromEnvironmentVariable; ++i)
  {
    {
      vtkLogScopeF(INFO, "with no preferences set.");
      vtkNew<vtkMockOverridable> object;
      // Since factory1 was registered first, it takes precedence
      if (!object->IsA("vtkMockOverrideClass1"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass1, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=X,Y;AttributeB=N,M";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO, "Expect factory1 override with stronger preference");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass1"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass1, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Y,X;AttributeB=M,N";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO, "Expect factory2 override with stronger preference");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass2"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass2, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Z;AttributeB=M,N";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO, "Expect factory1 override with stronger preference");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass1"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass1, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Z;AttributeB=N,M";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO, "Expect factory2 override with stronger preference");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass2"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass2, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Z;AttributeB=O";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO,
        "Expect factory1 override to be chosen because no attributes match provided preferences "
        "and factory1 was registered first.");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass1"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass1, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Z;AttributeB=P,N,M";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO, "Expect factory2 override with stronger preference");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass2"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass2, got " << object->GetClassName());
        success = false;
      }
    }
    {
      const std::string preferences = "AttributeA=Z,X,Y;AttributeB=M,N";
      vtkLogScopeF(INFO, "with preferences set to %s", preferences.c_str());
      ScopedTestCasePreferences scopedTestCasePreferences(preferences, i);
      vtkLog(INFO,
        "Expect factory1 override to be chosen because no attributes match provided preferences "
        "and factory1 was registered first.");
      vtkNew<vtkMockOverridable> object;
      if (!object->IsA("vtkMockOverrideClass1"))
      {
        vtkLog(ERROR, << "Expected vtkMockOverrideClass1, got " << object->GetClassName());
        success = false;
      }
    }
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
