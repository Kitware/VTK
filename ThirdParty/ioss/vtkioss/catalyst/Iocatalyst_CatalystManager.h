// Copyright(C) 1999-2021 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#ifndef IOSS_IOVS_CATALYST_MANAGER_H
#define IOSS_IOVS_CATALYST_MANAGER_H

#include "iocatalyst_export.h"
#include "vtk_ioss_mangle.h"
#include <Ioss_ParallelUtils.h>
#include <catalyst.hpp>

namespace Iocatalyst {

  class IOCATALYST_EXPORT CatalystManager
  {
  public:
    using CatalystPipelineID = unsigned int;

    enum mState { mInit, mExecute, mFinalize };
    enum pState { pWaitExecute, pExecute, pFinalize };

    static const std::string ARGS;
    static const std::string CATALYST;
    static const std::string CATALYST_BLOCK_PARSE_INPUT_DECK_NAME;
    static const std::string CATALYST_BLOCK_PARSE_JSON_STRING;
    static const std::string CATALYST_DEBUG_LEVEL;
    static const std::string CATALYST_ENABLE_LOGGING;
    static const std::string CATALYST_OUTPUT_DIRECTORY;
    static const std::string CATALYST_OUTPUT_DEFAULT;
    static const std::string CATALYST_INPUT_NAME;
    static const std::string CATALYST_INPUT_DEFAULT;
    static const std::string CATALYST_MULTI_INPUT_PIPELINE_NAME;
    static const std::string CATALYST_SCRIPT;
    static const std::string CATALYST_SCRIPT_EXTRA_FILE;
    static const std::string CHANNELS;
    static const std::string CYCLE;
    static const std::string DATA;
    static const std::string PHACTORI_JSON_SCRIPT;
    static const std::string PIPELINES;
    static const std::string FILENAME;
    static const std::string FS;
    static const std::string IOSS;
    static const std::string SCRIPTS;
    static const std::string STATE;
    static const std::string TIME;
    static const std::string TIMESTEP;
    static const std::string TYPE;

    static CatalystManager &getInstance()
    {
      static CatalystManager instance;
      return instance;
    }

    std::string getCatalystPythonDriverPath() { return "/todo/create/real/path"; }

    conduit_cpp::Node getInitializeConduit();

    mState getManagerState() { return managerState; }

    pState getPipelineState(CatalystPipelineID id);

    void writeToCatalystLogFile(const Ioss::ParallelUtils   &putils,
                                const Ioss::PropertyManager &props);

    struct CatalystProps
    {
      CatalystProps()
      {
        catalystPipelineID               = 0;
        enableLogging                    = false;
        debugLevel                       = 0;
        catalystOutputDirectory          = CATALYST_OUTPUT_DEFAULT;
        catalystInputName                = CATALYST_INPUT_DEFAULT;
        enableCatalystMultiInputPipeline = false;
        pipelineState                    = pExecute;
        state                            = 0;
        time                             = 0.0;
      }

      CatalystPipelineID catalystPipelineID;
      pState             pipelineState;
      bool               enableCatalystMultiInputPipeline;
      std::string        catalystMultiInputPipelineName;
      std::string        catalystPythonFilename;
      conduit_cpp::Node  data;
      int                state;
      double             time;

      std::string catalystInputName;
      std::string catalystBlockJSON;
      std::string catalystScriptExtraFile;
      std::string catalystInputDeckName;
      std::string catalystOutputDirectory;
      bool        enableLogging;
      int         debugLevel;
    };

    CatalystPipelineID initialize(const Ioss::PropertyManager &props,
                                  const Ioss::ParallelUtils   &putils);
    void execute(CatalystPipelineID id, int state, double time, conduit_cpp::Node &data);
    void finalize(CatalystPipelineID id);
    void addScriptProps(conduit_cpp::Node &n, const CatalystProps &p);
    void addExecuteProps(conduit_cpp::Node &n, const CatalystProps &p, int state, double time);
    void addExecuteData(conduit_cpp::Node &n, const std::string &channelName, int state,
                        double time, conduit_cpp::Node &data);
    CatalystProps &getCatalystProps(CatalystPipelineID id);
    std::string    getCatDataPath(const Ioss::PropertyManager &props);
    void           setMultiInputWaitState(CatalystPipelineID id, int state, double time,
                                          conduit_cpp::Node &data);
    bool           canExecuteMultiInputScript(CatalystPipelineID id);
    void           clearAllMultiInputWaitStates(CatalystPipelineID id);
    void           reset()
    {
      catalystOutputIDNumber = 0;
      catPipes.clear();
      managerState = mInit;
    }

    std::string getCatScriptFnamePath(const CatalystProps &p)
    {
      return getCatScriptPath(p) + FILENAME;
    }

    std::string getCatScriptArgsPath(const CatalystProps &p) { return getCatScriptPath(p) + ARGS; }

    std::string getCatScriptPath(const CatalystProps &p)
    {
      return CATALYST + FS + SCRIPTS + FS + std::to_string(p.catalystPipelineID) + FS;
    }

    std::string getCatStatePath() { return CATALYST + FS + STATE + FS; }

    std::string getCatChannelsPath() { return CATALYST + FS + CHANNELS + FS; }

  private:
    CatalystManager();
    ~CatalystManager();
    CatalystManager(const CatalystManager &)            = delete;
    CatalystManager &operator=(const CatalystManager &) = delete;

    void broadCastString(std::string &s, const Ioss::ParallelUtils &putils);
    void broadCastStatusCode(bool &statusCode, const Ioss::ParallelUtils &putils);

    void incrementOutputCounts();

    CatalystPipelineID                          catalystOutputIDNumber;
    std::map<CatalystPipelineID, CatalystProps> catPipes;
    mState                                      managerState;
  };
} // namespace Iocatalyst

#endif
