## Add function call feature in vtkObjectManager

You can now invoke methods on a registered object using the
new `nlohmann::json vtkObjectManager::Invoke(vtkTypeUInt32 identifier, const std::string& methodName, const nlohmann::json& args)`
function.
