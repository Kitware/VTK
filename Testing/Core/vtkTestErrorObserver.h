// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTestErrorObserver_h
#define vtkTestErrorObserver_h

#include <iostream> // Needed for std::cout
#include <string>   // Needed for std::string

#include <vtkCommand.h>

namespace vtkTest
{
VTK_ABI_NAMESPACE_BEGIN
class ErrorObserver : public ::vtkCommand
{
public:
  vtkTypeMacro(ErrorObserver, vtkCommand);

  ErrorObserver()
    : Error(false)
    , Warning(false)
  {
  }
  static ErrorObserver* New() { return new ErrorObserver; }
  bool GetError() const { return this->Error; }
  bool GetWarning() const { return this->Warning; }

  int GetNumberOfWarnings() { return this->WarningCount; }
  int GetNumberOfErrors() { return this->ErrorCount; }

  void Clear()
  {
    this->Error = false;
    this->Warning = false;
    this->ErrorMessage = "";
    this->WarningMessage = "";
    this->ErrorCount = 0;
    this->WarningCount = 0;
  }

  /**
   * Actual callback that catch errors and warnings, and store them internally.
   */
  void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* calldata) override
  {
    switch (event)
    {
      case vtkCommand::ErrorEvent:
        this->ErrorMessage += static_cast<char*>(calldata);
        this->Error = true;
        this->ErrorCount++;
        break;
      case vtkCommand::WarningEvent:
        this->WarningMessage += static_cast<char*>(calldata);
        this->Warning = true;
        this->WarningCount++;
        break;
    }
  }
  std::string GetErrorMessage() { return this->ErrorMessage; }

  std::string GetWarningMessage() { return this->WarningMessage; }

  /**
   * Check to see if an error message exists.
   * If it exists, it clears the error list and return 0.
   * Return 1 otherwise.
   * see: HasErrorMessage()
   */
  int CheckErrorMessage(const std::string& expectedMsg)
  {
    if (this->HasErrorMessage(expectedMsg))
    {
      this->Clear();
      return 0;
    }

    return 1;
  }

  /**
   * Check to see if an error message exists.
   * Given an observer, a message and a status with an initial value,
   * Returns false if an error does not exist, or if msg is not contained
   * in the error message.
   * Returns true if the error exists and msg is contained in the error
   * message.
   * If the test fails, it reports the failing message, prepended with "ERROR:".
   * ctest will detect the ERROR and report a failure.
   *
   * Unlike CheckErrorMessage, this does not clear the error and warning list on success.
   * see: CheckErrorMessage.
   */
  bool HasErrorMessage(const std::string& expectedMsg)
  {
    if (!this->GetError())
    {
      std::cout << "ERROR: Failed to catch any error. Expected the error message to contain \""
                << expectedMsg << '\"' << std::endl;
      return false;
    }
    else
    {
      std::string gotMsg(this->GetErrorMessage());
      if (gotMsg.find(expectedMsg) == std::string::npos)
      {
        std::cout << "ERROR: Error message does not contain \"" << expectedMsg << "\" got \n\""
                  << gotMsg << '\"' << std::endl;
        return false;
      }
    }

    return true;
  }

  /**
   * Check to see if a warning message exists.
   * Given an observer, a message and a status with an initial value,
   * Returns false if an warning does not exist, or if msg is not contained
   * in the warning message.
   * Returns true if the warning exists and msg is contained in the warning
   * message.
   *
   * If the test fails, it reports the failing message, prepended with "ERROR:".
   * ctest will detect the ERROR and report a failure.
   *
   * Unlike CheckWarningMessage, this does not clear the error and warning list on success.
   * see CheckWarningMessage
   */
  bool HasWarningMessage(const std::string& expectedMsg)
  {
    if (!this->GetWarning())
    {
      std::cout << "ERROR: Failed to catch any warning. Expected the warning message to contain \""
                << expectedMsg << '\"' << std::endl;
      return false;
    }
    else
    {
      std::string gotMsg(this->GetWarningMessage());
      if (gotMsg.find(expectedMsg) == std::string::npos)
      {
        std::cout << "ERROR: Warning message does not contain \"" << expectedMsg << "\" got \n\""
                  << gotMsg << '\"' << std::endl;
        return false;
      }
    }

    return true;
  }

  /**
   * Check to see if a warning message exists.
   * If it exists, it clears the warning list and return 0.
   * Return 1 otherwise.
   * see: HasWarningMessage()
   */
  int CheckWarningMessage(const std::string& expectedMsg)
  {
    if (this->HasWarningMessage(expectedMsg))
    {
      this->Clear();
      return 0;
    }

    return 1;
  }

private:
  bool Error = false;
  bool Warning = false;
  int WarningCount = 0;
  int ErrorCount = 0;
  std::string ErrorMessage;
  std::string WarningMessage;
};
VTK_ABI_NAMESPACE_END
}
#endif
// VTK-HeaderTest-Exclude: vtkTestErrorObserver.h
