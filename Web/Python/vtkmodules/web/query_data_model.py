"""
Core Module for Web Base Data Generation
"""

import sys, os, json

from vtkmodules.web import iteritems


class DataHandler(object):
    def __init__(self, basePath):
        self.__root = basePath
        self.types = ["tonic-query-data-model"]
        self.metadata = {}
        self.data = {}
        self.arguments = {}
        self.current = {}
        self.sections = {}
        self.basePattern = None
        self.priority = []
        self.argOrder = []
        self.realValues = {}
        self.can_write = True

    def getBasePath(self):
        return self.__root

    def updateBasePattern(self):
        self.priority.sort(key=lambda item: item[1])
        self.basePattern = ""
        patternSeparator = ""
        currentPriority = -1

        for item in self.priority:
            if currentPriority != -1:
                if currentPriority == item[1]:
                    patternSeparator = "_"
                else:
                    patternSeparator = "/"
            currentPriority = item[1]
            self.basePattern = "{%s}%s%s" % (
                item[0],
                patternSeparator,
                self.basePattern,
            )

    def registerArgument(self, **kwargs):
        """
        We expect the following set of arguments
         - priority
         - name
         - label (optional)
         - values
         - uiType
         - defaultIdx
        """
        newArgument = {}
        argName = kwargs["name"]
        self.argOrder.append(argName)
        for key, value in iteritems(kwargs):
            if key == "priority":
                self.priority.append([argName, value])
            elif key == "values":
                self.realValues[argName] = value
                newArgument[key] = ["{value}".format(value=x) for x in value]
            else:
                newArgument[key] = value

        self.arguments[argName] = newArgument

    def updatePriority(self, argumentName, newPriority):
        for item in self.priority:
            if item[0] == argumentName:
                item[1] = newPriority

    def setArguments(self, **kwargs):
        """
        Update the arguments index
        """
        for key, value in iteritems(kwargs):
            self.current[key] = value

    def removeData(self, name):
        del self.data[name]

    def registerData(self, **kwargs):
        """
        name, type, mimeType, fileName, dependencies
        """
        newData = {"metadata": {}}
        argName = kwargs["name"]
        for key, value in iteritems(kwargs):
            if key == "fileName":
                if "rootFile" in kwargs and kwargs["rootFile"]:
                    newData["pattern"] = "{pattern}/%s" % value
                else:
                    newData["pattern"] = "{pattern}%s" % value
            else:
                newData[key] = value

        self.data[argName] = newData

    def addDataMetaData(self, name, key, value):
        self.data[name]["metadata"][key] = value

    def getDataAbsoluteFilePath(self, name, createDirectories=True):
        dataPattern = self.data[name]["pattern"]
        if "{pattern}" in dataPattern:
            if len(self.basePattern) == 0:
                dataPattern = dataPattern.replace(
                    "{pattern}/", self.basePattern
                ).replace("{pattern}", self.basePattern)
                self.data[name]["pattern"] = dataPattern
            else:
                dataPattern = dataPattern.replace("{pattern}", self.basePattern)
                self.data[name]["pattern"] = dataPattern

        keyValuePair = {}
        for key, value in iteritems(self.current):
            keyValuePair[key] = self.arguments[key]["values"][value]

        fullpath = os.path.join(self.__root, dataPattern.format(**keyValuePair))

        if createDirectories and self.can_write:
            if not os.path.exists(os.path.dirname(fullpath)):
                os.makedirs(os.path.dirname(fullpath))

        return fullpath

    def addTypes(self, *args):
        for arg in args:
            self.types.append(arg)

    def addMetaData(self, key, value):
        self.metadata[key] = value

    def addSection(self, key, value):
        self.sections[key] = value

    def computeDataPatterns(self):
        if self.basePattern == None:
            self.updateBasePattern()

        for name in self.data:
            dataPattern = self.data[name]["pattern"]
            if "{pattern}" in dataPattern:
                dataPattern = dataPattern.replace("{pattern}", self.basePattern)
                self.data[name]["pattern"] = dataPattern

    def __getattr__(self, name):
        if self.basePattern == None:
            self.updateBasePattern()

        for i in range(len(self.arguments[name]["values"])):
            self.current[name] = i
            yield self.realValues[name][i]

    def writeDataDescriptor(self):
        if not self.can_write:
            return

        self.computeDataPatterns()

        jsonData = {
            "arguments_order": self.argOrder,
            "type": self.types,
            "arguments": self.arguments,
            "metadata": self.metadata,
            "data": [],
        }

        # Add sections
        for key, value in iteritems(self.sections):
            jsonData[key] = value

        # Add data
        for key, value in iteritems(self.data):
            jsonData["data"].append(value)

        filePathToWrite = os.path.join(self.__root, "index.json")
        with open(filePathToWrite, "w") as fileToWrite:
            fileToWrite.write(json.dumps(jsonData))
