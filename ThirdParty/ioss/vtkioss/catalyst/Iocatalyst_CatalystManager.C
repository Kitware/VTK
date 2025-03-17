// Copyright(C) 1999-2021, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include <catalyst/Iocatalyst_CatalystLogging.h>
#include <catalyst/Iocatalyst_CatalystManager.h>
#include <fstream>

namespace Iocatalyst {

  CatalystManager::CatalystManager() { reset(); }

  CatalystManager::~CatalystManager() {}

  void CatalystManager::writeToCatalystLogFile(const Ioss::ParallelUtils   &putils,
                                               const Ioss::PropertyManager &props)
  {
    if (putils.parallel_rank() == 0) {
      CatalystLogging catLog = CatalystLogging();
      catLog.setProperties(&props);
      if (catLog.isCatalystLoggingON()) {
        catLog.writeToLogFile();
      }
    }
    putils.barrier();
  }

  std::string CatalystManager::getCatDataPath(const Ioss::PropertyManager &props)
  {
    auto inputName = CATALYST_INPUT_DEFAULT;
    if (props.exists(CATALYST_INPUT_NAME)) {
      inputName = props.get(CATALYST_INPUT_NAME).get_string();
    }
    return CATALYST + FS + CHANNELS + FS + inputName + FS + DATA;
  }

  CatalystManager::CatalystPipelineID
  CatalystManager::initialize(const Ioss::PropertyManager &props, const Ioss::ParallelUtils &putils)
  {
    if (getManagerState() != mInit) {
      std::ostringstream errmsg;
      errmsg << "Catalyst Manager not in mInit state";
      IOSS_ERROR(errmsg);
    }

    CatalystManager::CatalystProps catalystProps;
    catalystProps.catalystPipelineID = catalystOutputIDNumber;
    incrementOutputCounts();

    if (props.exists(CATALYST_BLOCK_PARSE_JSON_STRING)) {
      catalystProps.catalystBlockJSON = props.get(CATALYST_BLOCK_PARSE_JSON_STRING).get_string();
    }
    else if (props.exists(PHACTORI_JSON_SCRIPT)) {
      bool        readOkay             = false;
      std::string phactoriJSONFilePath = props.get(PHACTORI_JSON_SCRIPT).get_string();
      if (putils.parallel_rank() == 0) {
        std::ifstream f(phactoriJSONFilePath);
        if (f) {
          std::ostringstream ss;
          ss << f.rdbuf();
          catalystProps.catalystBlockJSON = ss.str();
          readOkay                        = true;
        }
      }
      this->broadCastStatusCode(readOkay, putils);
      if (!readOkay) {
        std::ostringstream errmsg;
        errmsg << "Unable to read input file: " << phactoriJSONFilePath << "\n";
        IOSS_ERROR(errmsg);
      }
      else {
        this->broadCastString(catalystProps.catalystBlockJSON, putils);
      }
    }

    if (props.exists(CATALYST_SCRIPT)) {
      catalystProps.catalystPythonFilename = props.get(CATALYST_SCRIPT).get_string();
    }
    else {
      catalystProps.catalystPythonFilename = this->getCatalystPythonDriverPath();
    }

    if (props.exists(CATALYST_SCRIPT_EXTRA_FILE)) {
      catalystProps.catalystScriptExtraFile = props.get(CATALYST_SCRIPT_EXTRA_FILE).get_string();
    }

    if (props.exists(CATALYST_BLOCK_PARSE_INPUT_DECK_NAME)) {
      catalystProps.catalystInputDeckName =
          props.get(CATALYST_BLOCK_PARSE_INPUT_DECK_NAME).get_string();
    }

    if (props.exists(CATALYST_ENABLE_LOGGING)) {
      catalystProps.enableLogging = props.get(CATALYST_ENABLE_LOGGING).get_int();
    }

    if (props.exists(CATALYST_DEBUG_LEVEL)) {
      catalystProps.debugLevel = props.get(CATALYST_DEBUG_LEVEL).get_int();
    }

    if (props.exists(CATALYST_OUTPUT_DIRECTORY)) {
      catalystProps.catalystOutputDirectory = props.get(CATALYST_OUTPUT_DIRECTORY).get_string();
    }

    if (props.exists(CATALYST_INPUT_NAME)) {
      catalystProps.catalystInputName = props.get(CATALYST_INPUT_NAME).get_string();
    }

    if (props.exists(CATALYST_MULTI_INPUT_PIPELINE_NAME)) {
      catalystProps.enableCatalystMultiInputPipeline = true;
      catalystProps.catalystMultiInputPipelineName =
          props.get(CATALYST_MULTI_INPUT_PIPELINE_NAME).get_string();
    }

    catPipes[catalystProps.catalystPipelineID] = catalystProps;
    return catalystProps.catalystPipelineID;
  }

  CatalystManager::CatalystProps &CatalystManager::getCatalystProps(CatalystPipelineID id)
  {
    if (catPipes.find(id) == catPipes.end()) {
      std::ostringstream errmsg;
      errmsg << "Catalyst Pipeline ID does not exist:  " << id << "\n";
      IOSS_ERROR(errmsg);
    }
    return catPipes[id];
  }

  void CatalystManager::execute(CatalystManager::CatalystPipelineID id, int state, double time,
                                conduit_cpp::Node &data)
  {
    if (getManagerState() == mFinalize) {
      std::ostringstream errmsg;
      errmsg << "Catalyst Manager in mFinalize state, cannot execute()";
      IOSS_ERROR(errmsg);
    }
    auto p = getCatalystProps(id);
    if (p.pipelineState != pExecute) {
      std::ostringstream errmsg;
      errmsg << "Database not in pExecute state, cannot execute()";
      IOSS_ERROR(errmsg);
    }
    if (getManagerState() == mInit) {
      auto n = getInitializeConduit();
      catalyst_initialize(conduit_cpp::c_node(&n));
      managerState = mExecute;
    }

    conduit_cpp::Node n;
    addExecuteProps(n, p, state, time);

    if (p.enableCatalystMultiInputPipeline) {
      setMultiInputWaitState(p.catalystPipelineID, state, time, data);
      if (canExecuteMultiInputScript(p.catalystPipelineID)) {
        for (auto cp : catPipes) {
          addExecuteData(n, cp.second.catalystInputName, cp.second.state, cp.second.time,
                         cp.second.data);
        }
      }
      else {
        return;
      }
    }
    else {
      addExecuteData(n, p.catalystInputName, state, time, data);
    }

    catalyst_execute(conduit_cpp::c_node(&n));

    if (p.enableCatalystMultiInputPipeline) {
      if (canExecuteMultiInputScript(p.catalystPipelineID)) {
        clearAllMultiInputWaitStates(p.catalystPipelineID);
      }
    }
  }

  void CatalystManager::finalize(CatalystPipelineID id)
  {
    getCatalystProps(id).pipelineState = pFinalize;
    managerState                       = mExecute;
    bool canFinalizeManager            = true;
    for (auto p : catPipes) {
      canFinalizeManager &= p.second.pipelineState == pFinalize;
    }
    if (canFinalizeManager) {
      managerState = mFinalize;
      conduit_cpp::Node node;
      catalyst_finalize(conduit_cpp::c_node(&node));
    }
  }

  CatalystManager::pState CatalystManager::getPipelineState(CatalystPipelineID id)
  {
    return getCatalystProps(id).pipelineState;
  }

  conduit_cpp::Node CatalystManager::getInitializeConduit()
  {
    conduit_cpp::Node n;
    for (auto p : catPipes) {
      addScriptProps(n, p.second);
    }
    return n;
  }

  void CatalystManager::addScriptProps(conduit_cpp::Node &n, const CatalystProps &p)
  {
    n[getCatScriptFnamePath(p)] = p.catalystPythonFilename;
    n[getCatScriptArgsPath(p)].append().set(p.catalystInputName);
    n[getCatScriptArgsPath(p)].append().set(p.catalystBlockJSON);
    n[getCatScriptArgsPath(p)].append().set(p.catalystScriptExtraFile);
    n[getCatScriptArgsPath(p)].append().set(p.catalystInputDeckName);
    n[getCatScriptArgsPath(p)].append().set(p.catalystOutputDirectory);
    n[getCatScriptArgsPath(p)].append().set(std::to_string(p.enableLogging));
    n[getCatScriptArgsPath(p)].append().set(std::to_string(p.debugLevel));
  }

  void CatalystManager::addExecuteProps(conduit_cpp::Node &n, const CatalystProps &p, int state,
                                        double time)
  {
    n[getCatStatePath() + TIMESTEP].set(state - 1);
    n[getCatStatePath() + CYCLE].set(state - 1);
    n[getCatStatePath() + TIME].set(time);
    auto scriptName                                    = std::to_string(p.catalystPipelineID);
    n[getCatStatePath() + PIPELINES + FS + scriptName] = scriptName;
  }

  void CatalystManager::addExecuteData(conduit_cpp::Node &n, const std::string &channelName,
                                       int state, double time, conduit_cpp::Node &data)
  {
    auto ipath = getCatChannelsPath() + channelName + FS;
    n[ipath + TYPE].set(std::string(IOSS));
    auto dpath = ipath + FS + DATA;
    n[dpath].set_external(data);
    n[dpath + FS + TIMESTEP].set(state - 1);
    n[dpath + FS + CYCLE].set(state - 1);
    n[dpath + FS + TIME].set(time);
  }

  void CatalystManager::setMultiInputWaitState(CatalystPipelineID id, int state, double time,
                                               conduit_cpp::Node &data)
  {
    auto &p = CatalystManager::getInstance().getCatalystProps(id);
    if (!p.enableCatalystMultiInputPipeline) {
      std::ostringstream errmsg;
      errmsg << "Catalyst pipeline is not a multi-input pipeline";
      IOSS_ERROR(errmsg);
    }
    p.pipelineState = pWaitExecute;
    p.data.set_external(data);
    p.state = state;
    p.time  = time;
  }

  bool CatalystManager::canExecuteMultiInputScript(CatalystPipelineID id)
  {
    bool  canExecute = true;
    auto &p          = CatalystManager::getInstance().getCatalystProps(id);
    if (!p.enableCatalystMultiInputPipeline) {
      std::ostringstream errmsg;
      errmsg << "Catalyst pipeline is not a multi-input pipeline";
      IOSS_ERROR(errmsg);
    }
    const auto name = p.catalystMultiInputPipelineName;
    for (auto cp : catPipes) {
      if (cp.second.enableCatalystMultiInputPipeline &&
          cp.second.catalystMultiInputPipelineName == name) {
        canExecute &= cp.second.pipelineState == pWaitExecute;
      }
    }
    return canExecute;
  }

  void CatalystManager::clearAllMultiInputWaitStates(CatalystPipelineID id)
  {
    auto &p = CatalystManager::getInstance().getCatalystProps(id);
    if (!p.enableCatalystMultiInputPipeline) {
      std::ostringstream errmsg;
      errmsg << "Catalyst pipeline is not a multi-input pipeline";
      IOSS_ERROR(errmsg);
    }
    const auto name = p.catalystMultiInputPipelineName;
    for (auto &cp : catPipes) {
      if (cp.second.enableCatalystMultiInputPipeline &&
          cp.second.catalystMultiInputPipelineName == name) {
        cp.second.pipelineState = pExecute;
        cp.second.data.set(conduit_cpp::Node());
      }
    }
  }

  void CatalystManager::incrementOutputCounts() { catalystOutputIDNumber++; }

  void CatalystManager::broadCastString(IOSS_MAYBE_UNUSED std::string &s,
                                        IOSS_MAYBE_UNUSED const Ioss::ParallelUtils &putils)
  {
    IOSS_PAR_UNUSED(s);
    IOSS_PAR_UNUSED(putils);
#ifdef SEACAS_HAVE_MPI
    int size = s.size();
    putils.broadcast(size);
    if (putils.parallel_rank() != 0) {
      s.resize(size);
    }
    putils.broadcast(s);
#endif
  }

  void CatalystManager::broadCastStatusCode(IOSS_MAYBE_UNUSED bool &statusCode,
                                            IOSS_MAYBE_UNUSED const Ioss::ParallelUtils &putils)
  {
    IOSS_PAR_UNUSED(statusCode);
    IOSS_PAR_UNUSED(putils);
#ifdef SEACAS_HAVE_MPI

    int code = statusCode;
    putils.broadcast(code);
    statusCode = code;
#endif
  }

} // namespace Iocatalyst
