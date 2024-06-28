// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_IOVS_CATALYST_LOGGING_H
#define IOSS_IOVS_CATALYST_LOGGING_H

#include "iocatalyst_export.h"
#include "vtk_ioss_mangle.h"

#include <Ioss_CodeTypes.h>
#include <Ioss_PropertyManager.h>
#include <string>
#include <vector>

namespace Iocatalyst {

  class IOCATALYST_EXPORT CatalystLogging
  {
    // Enables Catalyst log output from IOSS when Catalyst CGNS or
    // Exodus IOSS databases are created.  Rank 0 of the application
    // writes an entry to a CSV (comma separted value) log file upon
    // IOSS database creation. Log output is controlled through
    // IOSS properties. IOSS properties that control logging must be
    // prepened with the string "CATALYST_LOGGING_".
    //
    // There are three reserved IOSS property names for logging.
    //
    // "CATALYST_LOGGING_ENABLED" : enables Catalyst log output when passed
    // an argument of "true or a non-zero integer". Default is "false".
    //
    // "CATALYST_LOGGING_FILE_NAME" : string specifying the log file output
    // name. Default is "catalyst_log.csv".
    //
    // "CATALYST_LOGGING_OUTPUT_DIRECTORY_PATH" : string specifying an absolute
    // or relative path to the log file. Default is current working directory, "".
    //
    // Real, Integer, and String IOSS properties prepended by the string
    // "CATALYST_LOGGING_" are used to specify output for the log line in the CSV
    // output log file. The logger will remove the prepended string and create
    // a header line in the CSV file sorted by name.
    //
    // Example application code creating a Catalyst IOSS database with logging:
    //
    // Ioss::PropertyManager *p;
    // p->add(Ioss::Property("CATALYST_LOGGING_ENABLED", true));
    // p->add(Ioss::Property("CATALYST_LOGGING_FILE_NAME", "app_log.csv"));
    // p->add(Ioss::Property("CATALYST_LOGGING_OUTPUT_DIRECTORY_PATH, "/etc/logs/"));
    // p->add(Ioss::Property("CATALYST_LOGGING_NUM_RANKS", getNumRanks()));
    // p->add(Ioss::Property("CATALYST_LOGGING_DATE", getDate()));
    // p->add(Ioss::Property("CATALYST_LOGGING_USER", getUser()));
    // p->add(Ioss::Property("CATALYST_LOGGING_APP_NAME", getAppName()));
    //
    // The IOSS properties contained in *p are passed to the IOSS
    // Ioss::IOFactory::create(). A log entry will be appended to the log file
    // at "/etc/logs/app_log_.csv" every time a Catalyst database is created by
    // a running instance of the application. The CSV log file will have the following
    // format, which can be easily read by Python.
    //
    // APP_NAME,DATE,NUM_RANKS,USER
    // goo,November 10th,16,joe
    // bar,December 12th,8,stan
    //
    // If an IOSS string property contains commas, these need to be quoted.
    //
    // p->add(Ioss::Property("CATALYST_LOGGING_ADDRESS", "\"123 main, PO 4, ND, 34422\""))
    //
    // Quotes inside strings must also be quoted.
    //
    // p->add(Ioss::Property("CATALYST_LOGGING_QUOTE", "I am \"\"Dave\"\""))

  public:
    CatalystLogging();

    bool                     isCatalystLoggingON() { return catalystLoggingEnabled; };
    std::string              getLogFileName() { return logFileName; };
    static std::string       getDefaultLogFileName() { return "catalyst_log.csv"; };
    std::string              getLogOutputDirectoryPath() { return logOutputDirectoryPath; };
    std::string              getDefaultLogOutputDirectoryPath() { return ""; };
    void                     setProperties(const Ioss::PropertyManager *properties);
    std::vector<std::string> getLogFileHeaders();
    std::vector<std::string> writeToLogFile();
    std::vector<std::vector<std::string>>        readLogFile();
    static std::vector<std::vector<std::string>> readLogFile(const std::string &logFilePath);
    std::string                                  getLogFilePath()
    {
      if (logOutputDirectoryPath.empty()) {
        return getLogFileName();
      }
      else {
        std::string opath = getLogOutputDirectoryPath();
        if (opath.back() != '/') {
          opath += '/';
        }
        return opath + getLogFileName();
      }
    };
    static char getDelimeter() { return ','; };
    bool isCatalystLoggingProp(std::string &propName) { return propName.rfind(logPrefix, 0) == 0; };
    std::string getHeaderNameFromPropName(std::string &propName)
    {
      return propName.substr(logPrefix.length());
    };
    std::string getPropNameFromHeaderName(std::string &headerName)
    {
      return logPrefix + headerName;
    };
    bool isReservedPropName(std::string &propName)
    {
      return propName == enabledProp || propName == fileNameProp || propName == directoryPathProp;
    };
    bool isSupportedPropType(std::string &propName)
    {
      bool retVal = false;
      if (properties && properties->exists(propName)) {
        Ioss::Property::BasicType type = properties->get(propName).get_type();
        retVal = (type == Ioss::Property::INTEGER || type == Ioss::Property::REAL ||
                  type == Ioss::Property::STRING);
      }
      return retVal;
    };

  private:
    bool                         catalystLoggingEnabled;
    std::string                  logFileName;
    std::string                  logOutputDirectoryPath;
    const Ioss::PropertyManager *properties;
    std::string                  logPrefix         = "CATALYST_LOGGING_";
    std::string                  enabledProp       = logPrefix + "ENABLED";
    std::string                  fileNameProp      = logPrefix + "FILE_NAME";
    std::string                  directoryPathProp = logPrefix + "OUTPUT_DIRECTORY_PATH";
    void                         initializeDefaults();
    void writeVectorWithDelimeter(std::fstream &file, const std::vector<std::string> &string_vector,
                                  char delimeter);
    static std::vector<std::string> splitStringWithDelimeter(const std::string &input,
                                                             char               delimeter);
    bool                            isLogFileEmpty();
    std::vector<std::string>        getLogOutputFromProps(std::vector<std::string> &headers);
  };

} // namespace Iocatalyst

#endif
