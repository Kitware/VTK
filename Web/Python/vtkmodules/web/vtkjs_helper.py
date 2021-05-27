import base64
import json
import re
import os
import shutil
import sys
import zipfile

try:
    import zlib

    compression = zipfile.ZIP_DEFLATED
except:
    compression = zipfile.ZIP_STORED

# -----------------------------------------------------------------------------


def convertDirectoryToZipFile(directoryPath):
    if os.path.isfile(directoryPath):
        return

    zipFilePath = "%s.zip" % directoryPath
    zf = zipfile.ZipFile(zipFilePath, mode="w")

    try:
        for dirName, subdirList, fileList in os.walk(directoryPath):
            for fname in fileList:
                fullPath = os.path.join(dirName, fname)
                relPath = "%s" % (os.path.relpath(fullPath, directoryPath))
                zf.write(fullPath, arcname=relPath, compress_type=compression)
    finally:
        zf.close()

    shutil.rmtree(directoryPath)
    shutil.move(zipFilePath, directoryPath)


# -----------------------------------------------------------------------------


def addDataToViewer(dataPath, srcHtmlPath):
    if os.path.isfile(dataPath) and os.path.exists(srcHtmlPath):
        dstDir = os.path.dirname(dataPath)
        dstHtmlPath = os.path.join(dstDir, "%s.html" % os.path.basename(dataPath)[:-6])

        # Extract data as base64
        with open(dataPath, "rb") as data:
            dataContent = data.read()
            base64Content = base64.b64encode(dataContent)
            base64Content = base64Content.decode().replace("\n", "")

        # Create new output file
        with open(srcHtmlPath, mode="r", encoding="utf-8") as srcHtml:
            with open(dstHtmlPath, mode="w", encoding="utf-8") as dstHtml:
                for line in srcHtml:
                    if "</body>" in line:
                        dstHtml.write("<script>\n")
                        dstHtml.write('var contentToLoad = "%s";\n\n' % base64Content)
                        dstHtml.write(
                            'Glance.importBase64Dataset("%s" , contentToLoad, glanceInstance.proxyManager);\n'
                            % os.path.basename(dataPath)
                        )
                        dstHtml.write("glanceInstance.showApp();\n")
                        dstHtml.write("</script>\n")

                    dstHtml.write(line)


# -----------------------------------------------------------------------------


def zipAllTimeSteps(directoryPath):
    if os.path.isfile(directoryPath):
        return

    class UrlCounterDict(dict):
        Counter = 0

        def GetUrlName(self, name):
            if name not in self.keys():
                self[name] = str(objNameToUrls.Counter)
                self.Counter = self.Counter + 1
            return self[name]

    def InitIndex(sourcePath, destObj):
        with open(sourcePath, "r") as sourceFile:
            sourceData = sourceFile.read()
            sourceObj = json.loads(sourceData)
            for key in sourceObj:
                destObj[key] = sourceObj[key]
            # remove vtkHttpDataSetReader information
            for obj in destObj["scene"]:
                obj.pop(obj["type"])
                obj.pop("type")

    def getUrlToNameDictionary(indexObj):
        urls = {}
        for obj in indexObj["scene"]:
            urls[obj[obj["type"]]["url"]] = obj["name"]
        return urls

    def addDirectoryToZip(
        dirname, zipobj, storedData, rootIdx, timeStep, objNameToUrls
    ):
        # Update root index.json file from index.json of this timestep
        with open(os.path.join(dirname, "index.json"), "r") as currentIdxFile:
            currentIdx = json.loads(currentIdxFile.read())
            urlToName = getUrlToNameDictionary(currentIdx)
            rootTimeStepSection = rootIdx["animation"]["timeSteps"][timeStep]
            for key in currentIdx:
                if key == "scene" or key == "version":
                    continue
                rootTimeStepSection[key] = currentIdx[key]
            for obj in currentIdx["scene"]:
                objName = obj["name"]
                rootTimeStepSection[objName] = {}
                rootTimeStepSection[objName]["actor"] = obj["actor"]
                rootTimeStepSection[objName]["actorRotation"] = obj["actorRotation"]
                rootTimeStepSection[objName]["mapper"] = obj["mapper"]
                rootTimeStepSection[objName]["property"] = obj["property"]

        # For every object in the current timestep
        for folder in sorted(os.listdir(dirname)):
            currentItem = os.path.join(dirname, folder)
            if os.path.isdir(currentItem) is False:
                continue
            # Write all data array of the current timestep in the archive
            for filename in os.listdir(os.path.join(currentItem, "data")):
                fullpath = os.path.join(currentItem, "data", filename)
                if os.path.isfile(fullpath) and filename not in storedData:
                    storedData.add(filename)
                    relPath = os.path.join("data", filename)
                    zipobj.write(fullpath, arcname=relPath, compress_type=compression)
            # Write the index.json containing pointers to these data arrays
            # while replacing every basepath as '../../data'
            objIndexFilePath = os.path.join(dirname, folder, "index.json")
            with open(objIndexFilePath, "r") as objIndexFile:
                objIndexObjData = json.loads(objIndexFile.read())
            for elm in objIndexObjData.keys():
                try:
                    if "ref" in objIndexObjData[elm].keys():
                        objIndexObjData[elm]["ref"]["basepath"] = "../../data"
                    if "arrays" in objIndexObjData[elm].keys():
                        for array in objIndexObjData[elm]["arrays"]:
                            array["data"]["ref"]["basepath"] = "../../data"
                except AttributeError:
                    continue
            currentObjName = urlToName[folder]
            objIndexRelPath = os.path.join(
                objNameToUrls.GetUrlName(currentObjName), str(timeStep), "index.json"
            )
            zipobj.writestr(
                objIndexRelPath,
                json.dumps(objIndexObjData, indent=2),
                compress_type=compression,
            )

    # ---

    zipFilePath = "%s.zip" % directoryPath
    currentDirectory = os.path.abspath(os.path.join(directoryPath, os.pardir))
    rootIndexPath = os.path.join(currentDirectory, "index.json")
    rootIndexFile = open(rootIndexPath, "r")
    rootIndexObj = json.loads(rootIndexFile.read())

    zf = zipfile.ZipFile(zipFilePath, mode="w")
    try:
        # We copy the scene from an index of a specific timestep to the root index
        # Scenes should all have the same objects so only do it for the first one
        isSceneInitialized = False
        # currentlyAddedData set stores hashes of every data we already added to the
        # vtkjs archive to prevent data duplication
        currentlyAddedData = set()
        # Regex that folders storing timestep data from paraview should follow
        reg = re.compile(r"^" + os.path.basename(directoryPath) + r"\.[0-9]+$")
        # We assume an object will not be deleted from a timestep to another so we create a generic index.json for each object
        genericIndexObj = {}
        genericIndexObj["series"] = []
        timeStep = 0
        for item in rootIndexObj["animation"]["timeSteps"]:
            genericIndexObj["series"].append({})
            genericIndexObj["series"][timeStep]["url"] = str(timeStep)
            genericIndexObj["series"][timeStep]["timeStep"] = float(item["time"])
            timeStep = timeStep + 1
        # Keep track of the url for every object
        objNameToUrls = UrlCounterDict()

        timeStep = 0
        # zip all timestep directories
        for folder in sorted(os.listdir(currentDirectory)):
            fullPath = os.path.join(currentDirectory, folder)
            if os.path.isdir(fullPath) and reg.match(folder):
                if not isSceneInitialized:
                    InitIndex(os.path.join(fullPath, "index.json"), rootIndexObj)
                    isSceneInitialized = True
                addDirectoryToZip(
                    fullPath,
                    zf,
                    currentlyAddedData,
                    rootIndexObj,
                    timeStep,
                    objNameToUrls,
                )
                shutil.rmtree(fullPath)
                timeStep = timeStep + 1

        # Write every index.json holding time information for each object
        for name in objNameToUrls:
            zf.writestr(
                os.path.join(objNameToUrls[name], "index.json"),
                json.dumps(genericIndexObj, indent=2),
                compress_type=compression,
            )

        # Update root index.json urls and write it in the archive
        for obj in rootIndexObj["scene"]:
            obj["id"] = obj["name"]
            obj["type"] = "vtkHttpDataSetSeriesReader"
            obj["vtkHttpDataSetSeriesReader"] = {}
            obj["vtkHttpDataSetSeriesReader"]["url"] = objNameToUrls[obj["name"]]
        zf.writestr(
            "index.json", json.dumps(rootIndexObj, indent=2), compress_type=compression
        )
        os.remove(rootIndexPath)

    finally:
        zf.close()

    shutil.move(zipFilePath, directoryPath)


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(
            "Usage: directoryToFile /path/to/directory.vtkjs [/path/to/ParaViewGlance.html]"
        )
    else:
        fileName = sys.argv[1]
        convertDirectoryToZipFile(fileName)

        if len(sys.argv) == 3:
            addDataToViewer(fileName, sys.argv[2])
