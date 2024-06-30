// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <Ioss_Utils.h>
#include <fstream>
#include <iostream>
#include <time.h>
#include <catalyst/Iocatalyst_CatalystLogging.h>

namespace Iocatalyst {

  CatalystLogging::CatalystLogging() { initializeDefaults(); }

  void CatalystLogging::initializeDefaults()
  {
    catalystLoggingEnabled = false;
    logFileName            = getDefaultLogFileName();
    logOutputDirectoryPath = getDefaultLogOutputDirectoryPath();
    properties             = nullptr;
  }

  void CatalystLogging::setProperties(const Ioss::PropertyManager *my_properties)
  {
    initializeDefaults();
    this->properties = my_properties;
    if (this->properties) {
      if (this->properties->exists(enabledProp)) {
        catalystLoggingEnabled = this->properties->get(enabledProp).get_int();
      }
      if (this->properties->exists(fileNameProp)) {
        logFileName = this->properties->get(fileNameProp).get_string();
      }
      if (this->properties->exists(directoryPathProp)) {
        logOutputDirectoryPath = this->properties->get(directoryPathProp).get_string();
      }
    }
  }

  std::vector<std::string> CatalystLogging::writeToLogFile()
  {
    std::vector<std::string> logLine;
    if (properties) {
      std::fstream logFile;
      logFile.open(getLogFilePath(), std::ios::out | std::ios::app);
      if (!logFile) {
        std::ostringstream errmsg;
        errmsg << "Unable to open Catalyst log file: " << getLogFilePath() << "\n";
        IOSS_ERROR(errmsg);
      }
      else {
        std::vector<std::string> headers = getLogFileHeaders();
        if (isLogFileEmpty()) {
          writeVectorWithDelimeter(logFile, headers, getDelimeter());
        }
        logLine = getLogOutputFromProps(headers);
        writeVectorWithDelimeter(logFile, logLine, getDelimeter());
        logFile.close();
      }
    }
    return logLine;
  }

  std::vector<std::string> CatalystLogging::getLogFileHeaders()
  {
    std::vector<std::string> headers;
    if (properties) {
      Ioss::NameList names = properties->describe();
      for (auto &name : names) {
        if (isCatalystLoggingProp(name)) {
          if (isSupportedPropType(name) && !isReservedPropName(name)) {
            headers.push_back(getHeaderNameFromPropName(name));
          }
        }
      }
    }
    std::sort(headers.begin(), headers.end());
    return headers;
  }

  bool CatalystLogging::isLogFileEmpty()
  {
    std::ifstream logFile;
    logFile.open(getLogFilePath());
    bool result = logFile.peek() == std::ifstream::traits_type::eof();
    logFile.close();
    return result;
  }

  void CatalystLogging::writeVectorWithDelimeter(std::fstream                   &file,
                                                 const std::vector<std::string> &string_vector,
                                                 char                            delimeter)
  {
    if (string_vector.empty()) {
      return;
    }
    for (size_t i = 0; i < string_vector.size(); i++) {
      file << string_vector[i];
      if (i < string_vector.size() - 1) {
        file << delimeter;
      }
    }
    file << "\n";
  }

  std::vector<std::vector<std::string>> CatalystLogging::readLogFile()
  {
    return readLogFile(getLogFilePath());
  }

  std::vector<std::vector<std::string>> CatalystLogging::readLogFile(const std::string &logFilePath)
  {
    std::vector<std::vector<std::string>> result;
    std::fstream                          logFile;
    logFile.open(logFilePath);
    if (logFile) {
      std::string line;
      while (getline(logFile, line)) {
        result.push_back(
            CatalystLogging::splitStringWithDelimeter(line, CatalystLogging::getDelimeter()));
      }
      logFile.close();
    }
    return result;
  }

  std::vector<std::string> CatalystLogging::splitStringWithDelimeter(const std::string &input,
                                                                     char               delimeter)
  {
    std::string              buffer = "";
    std::vector<std::string> result;
    enum ParseState { UNQUOTED, QUOTED, QUOTEDQUOTE };
    ParseState state = UNQUOTED;
    for (size_t i = 0; i < input.size(); i++) {
      switch (state) {
      case UNQUOTED:
        if (input[i] == delimeter) {
          result.push_back(buffer);
          buffer.clear();
        }
        else if (input[i] == '"') {
          state = QUOTED;
          buffer += "\"";
        }
        else {
          buffer += input[i];
        }
        break;
      case QUOTED:
        if (input[i] == '"') {
          state = UNQUOTED;
          buffer += "\"";
        }
        else {
          buffer += input[i];
        }
        break;
      case QUOTEDQUOTE:
        if (input[i] == delimeter) {
          state = UNQUOTED;
          result.push_back(buffer);
          buffer.clear();
        }
        else if (input[i] == '"') {
          state = QUOTED;
          buffer += "\"";
        }
        else {
          state = UNQUOTED;
          buffer += input[i];
        }
        break;
      }
    }
    if (!buffer.empty()) {
      result.push_back(buffer);
    }
    return result;
  }

  std::vector<std::string> CatalystLogging::getLogOutputFromProps(std::vector<std::string> &headers)
  {
    std::vector<std::string> logOutput;
    if (properties) {
      for (auto &header : headers) {
        std::string    propName = getPropNameFromHeaderName(header);
        Ioss::Property prop     = properties->get(propName);
        switch (prop.get_type()) {
        case Ioss::Property::REAL: logOutput.push_back(std::to_string(prop.get_real())); break;
        case Ioss::Property::INTEGER: logOutput.push_back(std::to_string(prop.get_int())); break;
        case Ioss::Property::STRING: logOutput.push_back(prop.get_string()); break;
        default: logOutput.push_back("Unsupported property type for " + propName);
        }
      }
    }
    return logOutput;
  }

} // namespace Iocatalyst
