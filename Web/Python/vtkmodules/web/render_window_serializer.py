import io
import logging
import struct
import time
import zipfile

from vtkmodules.web import (
    base64Encode,
    hashDataArray,
    getJSArrayType,
    arrayTypesMapping,
    getReferenceId,
)

from vtkmodules.vtkCommonCore import vtkTypeUInt32Array
from vtkmodules.vtkFiltersGeometry import vtkCompositeDataGeometryFilter
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkRenderingCore import vtkColorTransferFunction

logger = logging.getLogger(__name__)
# Always DEBUG level for this logger. Users can change this
logger.setLevel(logging.DEBUG)

# -----------------------------------------------------------------------------
# Array helpers
# -----------------------------------------------------------------------------

def zipCompression(name, data):
    with io.BytesIO() as in_memory:
        with zipfile.ZipFile(in_memory, mode="w") as zf:
            zf.writestr("data/%s" % name, data, zipfile.ZIP_DEFLATED)
        in_memory.seek(0)
        return in_memory.read()


def dataTableToList(dataTable):
    dataType = arrayTypesMapping[dataTable.GetDataType()]
    elementSize = struct.calcsize(dataType)
    nbValues = dataTable.GetNumberOfValues()
    nbComponents = dataTable.GetNumberOfComponents()
    nbytes = elementSize * nbValues
    if dataType != " ":
        with io.BytesIO(memoryview(dataTable)) as stream:
            data = list(struct.unpack(dataType * nbValues, stream.read(nbytes)))
        return [
            data[idx * nbComponents : (idx + 1) * nbComponents]
            for idx in range(nbValues // nbComponents)
        ]

    return None


# -----------------------------------------------------------------------------


def linspace(start, stop, num):
    delta = (stop - start) / (num - 1)
    return [start + i * delta for i in range(num)]


# -----------------------------------------------------------------------------
# Convenience class for caching data arrays, storing computed sha sums, keeping
# track of valid actors, etc...
# -----------------------------------------------------------------------------


class SynchronizationContext:
    def __init__(self):
        self.dataArrayCache = {}
        self.lastDependenciesMapping = {}
        self.ingoreLastDependencies = False

    def setIgnoreLastDependencies(self, force):
        self.ingoreLastDependencies = force

    def cacheDataArray(self, pMd5, data):
        self.dataArrayCache[pMd5] = data

    def getCachedDataArray(self, pMd5, binary=False, compression=False):
        cacheObj = self.dataArrayCache[pMd5]
        array = cacheObj["array"]
        cacheTime = cacheObj["mTime"]

        if cacheTime != array.GetMTime():
            logger.debug(" ***** ERROR: you asked for an old cache key! ***** ")

        if array.GetDataType() == 12:
            # IdType need to be converted to Uint32
            arraySize = array.GetNumberOfTuples() * array.GetNumberOfComponents()
            newArray = vtkTypeUInt32Array()
            newArray.SetNumberOfTuples(arraySize)
            for i in range(arraySize):
                newArray.SetValue(i, -1 if array.GetValue(i) < 0 else array.GetValue(i))
            pBuffer = memoryview(newArray)
        else:
            pBuffer = memoryview(array)

        if binary:
            # Convert the vtkUnsignedCharArray into a bytes object, required by
            # Autobahn websockets
            return (
                pBuffer.tobytes()
                if not compression
                else zipCompression(pMd5, pBuffer.tobytes())
            )

        return base64Encode(
            pBuffer if not compression else zipCompression(pMd5, pBuffer.tobytes())
        )

    def checkForArraysToRelease(self, timeWindow=20):
        cutOffTime = time.time() - timeWindow
        shasToDelete = []
        for sha in self.dataArrayCache:
            record = self.dataArrayCache[sha]
            array = record["array"]
            count = array.GetReferenceCount()

            if count == 1 and record["ts"] < cutOffTime:
                shasToDelete.append(sha)

        for sha in shasToDelete:
            del self.dataArrayCache[sha]

    def getLastDependencyList(self, idstr):
        lastDeps = []
        if idstr in self.lastDependenciesMapping and not self.ingoreLastDependencies:
            lastDeps = self.lastDependenciesMapping[idstr]
        return lastDeps

    def setNewDependencyList(self, idstr, depList):
        self.lastDependenciesMapping[idstr] = depList

    def buildDependencyCallList(self, idstr, newList, addMethod, removeMethod):
        oldList = self.getLastDependencyList(idstr)

        calls = []
        calls += [[addMethod, [wrapId(x)]] for x in newList if x not in oldList]
        calls += [[removeMethod, [wrapId(x)]] for x in oldList if x not in newList]

        self.setNewDependencyList(idstr, newList)
        return calls


# -----------------------------------------------------------------------------
# Global variables
# -----------------------------------------------------------------------------

SERIALIZERS = {}
JS_CLASS_MAPPING = {}
context = None

# -----------------------------------------------------------------------------
# Global API
# -----------------------------------------------------------------------------


def registerInstanceSerializer(name, method):
    global SERIALIZERS
    SERIALIZERS[name] = method

def registerJSClass(vtk_class, js_class):
    global JS_CLASS_MAPPING
    JS_CLASS_MAPPING[vtk_class] = js_class

def class_name(vtk_obj):
    vtk_class = vtk_obj.GetClassName()
    if vtk_class in JS_CLASS_MAPPING:
        return JS_CLASS_MAPPING[vtk_class]

    return vtk_class


# -----------------------------------------------------------------------------


def serializeInstance(parent, instance, instanceId, context, depth):
    instanceType = class_name(instance)
    serializer = SERIALIZERS[instanceType] if instanceType in SERIALIZERS else None

    if serializer:
        return serializer(parent, instance, instanceId, context, depth)

    logger.error(f"!!!No serializer for {instanceType} with id {instanceId}")

    return None


# -----------------------------------------------------------------------------


def initializeSerializers():
    # Actors/viewProps
    registerInstanceSerializer("vtkActor", genericActorSerializer)
    registerInstanceSerializer("vtkOpenGLActor", genericActorSerializer)
    registerInstanceSerializer("vtkPVLODActor", genericActorSerializer)

    # Volume/viewProps
    registerInstanceSerializer("vtkVolume", genericVolumeSerializer)

    # Mappers
    registerInstanceSerializer("vtkMapper", genericMapperSerializer)
    registerInstanceSerializer("vtkDataSetMapper", genericMapperSerializer)
    registerInstanceSerializer("vtkPolyDataMapper", genericMapperSerializer)
    registerInstanceSerializer("vtkImageDataMapper", genericMapperSerializer)
    registerInstanceSerializer("vtkOpenGLPolyDataMapper", genericMapperSerializer)
    registerInstanceSerializer("vtkCompositePolyDataMapper2", genericMapperSerializer)
    registerJSClass("vtkPolyDataMapper", "vtkMapper")
    registerJSClass("vtkDataSetMapper", "vtkMapper")
    registerJSClass("vtkOpenGLPolyDataMapper", "vtkMapper")
    registerJSClass("vtkCompositePolyDataMapper2", "vtkMapper")

    registerInstanceSerializer("vtkVolumeMapper", genericVolumeMapperSerializer)
    registerInstanceSerializer("vtkFixedPointVolumeRayCastMapper", genericVolumeMapperSerializer)
    registerJSClass("vtkFixedPointVolumeRayCastMapper", "vtkVolumeMapper")

    # LookupTables/TransferFunctions
    registerInstanceSerializer("vtkLookupTable", lookupTableSerializer2)
    registerInstanceSerializer(
        "vtkPVDiscretizableColorTransferFunction", discretizableColorTransferFunctionSerializer
    )
    registerInstanceSerializer(
        "vtkColorTransferFunction", colorTransferFunctionSerializer
    )
    registerInstanceSerializer("vtkPiecewiseFunction", pwfSerializer)

    # Textures
    registerInstanceSerializer("vtkTexture", textureSerializer)
    registerInstanceSerializer("vtkOpenGLTexture", textureSerializer)

    # Property
    registerInstanceSerializer("vtkProperty", propertySerializer)
    registerInstanceSerializer("vtkOpenGLProperty", propertySerializer)

    # VolumeProperty
    registerInstanceSerializer("vtkVolumeProperty", volumePropertySerializer)

    # Datasets
    registerInstanceSerializer("vtkPolyData", polydataSerializer)
    registerInstanceSerializer("vtkImageData", imagedataSerializer)
    registerInstanceSerializer("vtkUnstructuredGrid", mergeToPolydataSerializer)
    registerInstanceSerializer("vtkMultiBlockDataSet", mergeToPolydataSerializer)
    registerInstanceSerializer("vtkStructuredPoints", imagedataSerializer)
    registerJSClass("vtkStructuredPoints", "vtkImageData")


    # RenderWindows
    registerInstanceSerializer("vtkRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkCocoaRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkXOpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkWin32OpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkEGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkOpenVRRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkOpenXRRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkGenericOpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkOSOpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkOpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkIOSRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkExternalOpenGLRenderWindow", renderWindowSerializer)
    registerInstanceSerializer("vtkOffscreenOpenGLRenderWindow", renderWindowSerializer)

    # Renderers
    registerInstanceSerializer("vtkRenderer", rendererSerializer)
    registerInstanceSerializer("vtkOpenGLRenderer", rendererSerializer)

    # Cameras
    registerInstanceSerializer("vtkCamera", cameraSerializer)
    registerInstanceSerializer("vtkOpenGLCamera", cameraSerializer)

    # Lights
    registerInstanceSerializer("vtkLight", lightSerializer)
    registerInstanceSerializer("vtkPVLight", lightSerializer)
    registerInstanceSerializer("vtkOpenGLLight", lightSerializer)

    # Annotations (ScalarBar/CubeAxes
    registerInstanceSerializer("vtkCubeAxesActor", cubeAxesSerializer)
    registerInstanceSerializer("vtkScalarBarActor", scalarBarActorSerializer)


# -----------------------------------------------------------------------------
# Helper functions
# -----------------------------------------------------------------------------


def pad(depth):
    padding = ""
    for _ in range(depth):
        padding += "  "
    return padding


# -----------------------------------------------------------------------------


def wrapId(idStr):
    return "instance:${%s}" % idStr


# -----------------------------------------------------------------------------

dataArrayShaMapping = {}


def digest(array):
    objId = getReferenceId(array)

    record = None
    if objId in dataArrayShaMapping:
        record = dataArrayShaMapping[objId]

    if record and record["mtime"] == array.GetMTime():
        return record["sha"]

    record = {"sha": hashDataArray(array), "mtime": array.GetMTime()}

    dataArrayShaMapping[objId] = record
    return record["sha"]


# -----------------------------------------------------------------------------


def getRangeInfo(array, component):
    r = array.GetRange(component)
    compRange = {}
    compRange["min"] = r[0]
    compRange["max"] = r[1]
    compRange["component"] = array.GetComponentName(component)
    return compRange


# -----------------------------------------------------------------------------


def getArrayDescription(array, context):
    if not array:
        return None

    pMd5 = digest(array)
    context.cacheDataArray(
        pMd5, {"array": array, "mTime": array.GetMTime(), "ts": time.time()}
    )

    root = {}
    root["hash"] = pMd5
    root["vtkClass"] = "vtkDataArray"
    root["name"] = array.GetName()
    root["dataType"] = getJSArrayType(array)
    root["numberOfComponents"] = array.GetNumberOfComponents()
    root["size"] = array.GetNumberOfComponents() * array.GetNumberOfTuples()
    root["ranges"] = []
    if root["numberOfComponents"] > 1:
        for i in range(root["numberOfComponents"]):
            root["ranges"].append(getRangeInfo(array, i))
        root["ranges"].append(getRangeInfo(array, -1))
    else:
        root["ranges"].append(getRangeInfo(array, 0))

    return root


# -----------------------------------------------------------------------------


def extractRequiredFields(
    extractedFields, parent, dataset, context, requestedFields=["Normals", "TCoords"]
):
    arrays_to_export = set()
    export_all = "*" in requestedFields
    # Identify arrays to export
    if not export_all:
        # FIXME should evolve and support funky mapper which leverage many arrays
        if parent and parent.IsA("vtkMapper"):
            mapper = parent
            scalarVisibility = mapper.GetScalarVisibility()
            arrayAccessMode = mapper.GetArrayAccessMode()
            colorArrayName = (
                mapper.GetArrayName() if arrayAccessMode == 1 else mapper.GetArrayId()
            )
            # colorMode = mapper.GetColorMode()
            scalarMode = mapper.GetScalarMode()
            if scalarVisibility and scalarMode in (1, 3):
                array_to_export = dataset.GetPointData().GetArray(colorArrayName)
                if array_to_export is None:
                    array_to_export = dataset.GetPointData().GetScalars()
                arrays_to_export.add(array_to_export)
            if scalarVisibility and scalarMode in (2, 4):
                array_to_export = dataset.GetCellData().GetArray(colorArrayName)
                if array_to_export is None:
                    array_to_export = dataset.GetCellData().GetScalars()
                arrays_to_export.add(array_to_export)
            if scalarVisibility and scalarMode == 0:
                array_to_export = dataset.GetPointData().GetScalars()
                if array_to_export is None:
                    array_to_export = dataset.GetCellData().GetScalars()
                arrays_to_export.add(array_to_export)

        if parent and parent.IsA("vtkTexture") and dataset.GetPointData().GetScalars():
            arrays_to_export.add(dataset.GetPointData().GetScalars())

        arrays_to_export.update(
            [
                getattr(dataset.GetPointData(), "Get" + requestedField, lambda: None)()
                for requestedField in requestedFields
            ]
        )

    # Browse all arrays
    for location, field_data in [
        ("pointData", dataset.GetPointData()),
        ("cellData", dataset.GetCellData()),
    ]:
        for array_index in range(field_data.GetNumberOfArrays()):
            array = field_data.GetArray(array_index)
            if export_all or array in arrays_to_export:
                arrayMeta = getArrayDescription(array, context)
                if arrayMeta:
                    arrayMeta["location"] = location
                    attribute = field_data.IsArrayAnAttribute(array_index)
                    arrayMeta["registration"] = (
                        "set" + field_data.GetAttributeTypeAsString(attribute)
                        if attribute >= 0
                        else "addArray"
                    )
                    extractedFields.append(arrayMeta)

# -----------------------------------------------------------------------------
# Concrete instance serializers
# -----------------------------------------------------------------------------


def genericActorSerializer(parent, actor, actorId, context, depth):
    # This kind of actor has two "children" of interest, a property and a
    # mapper
    actorVisibility = actor.GetVisibility()
    mapperInstance = None
    propertyInstance = None
    calls = []
    dependencies = []

    if actorVisibility:
        mapper = None
        if not hasattr(actor, "GetMapper"):
            logger.debug("This actor does not have a GetMapper method")
        else:
            mapper = actor.GetMapper()

        if mapper:
            mapperId = getReferenceId(mapper)
            mapperInstance = serializeInstance(
                actor, mapper, mapperId, context, depth + 1
            )
            if mapperInstance:
                dependencies.append(mapperInstance)
                calls.append(["setMapper", [wrapId(mapperId)]])

        prop = None
        if hasattr(actor, "GetProperty"):
            prop = actor.GetProperty()
        else:
            logger.debug("This actor does not have a GetProperty method")

        if prop:
            propId = getReferenceId(prop)
            propertyInstance = serializeInstance(
                actor, prop, propId, context, depth + 1
            )
            if propertyInstance:
                dependencies.append(propertyInstance)
                calls.append(["setProperty", [wrapId(propId)]])

        # Handle texture if any
        texture = None
        if hasattr(actor, "GetTexture"):
            texture = actor.GetTexture()
        else:
            logger.debug("This actor does not have a GetTexture method")

        if texture:
            textureId = getReferenceId(texture)
            textureInstance = serializeInstance(
                actor, texture, textureId, context, depth + 1
            )
            if textureInstance:
                dependencies.append(textureInstance)
                calls.append(["addTexture", [wrapId(textureId)]])

    if actorVisibility == 0 or (mapperInstance and propertyInstance):
        return {
            "parent": getReferenceId(parent),
            "id": actorId,
            "type": class_name(actor),
            "properties": {
                # vtkProp
                "visibility": actorVisibility,
                "pickable": actor.GetPickable(),
                "dragable": actor.GetDragable(),
                "useBounds": actor.GetUseBounds(),
                # vtkProp3D
                "origin": actor.GetOrigin(),
                "position": actor.GetPosition(),
                "scale": actor.GetScale(),
                # vtkActor
                "forceOpaque": actor.GetForceOpaque(),
                "forceTranslucent": actor.GetForceTranslucent(),
            },
            "calls": calls,
            "dependencies": dependencies,
        }

    return None


# -----------------------------------------------------------------------------


def genericVolumeSerializer(parent, actor, actorId, context, depth):
    # This kind of actor has two "children" of interest, a property and a
    # mapper
    actorVisibility = actor.GetVisibility()
    mapperInstance = None
    propertyInstance = None
    calls = []
    dependencies = []

    if actorVisibility:
        mapper = None
        if not hasattr(actor, "GetMapper"):
            logger.debug("This actor does not have a GetMapper method")
        else:
            mapper = actor.GetMapper()

        if mapper:
            mapperId = getReferenceId(mapper)
            mapperInstance = serializeInstance(
                actor, mapper, mapperId, context, depth + 1
            )
            if mapperInstance:
                dependencies.append(mapperInstance)
                calls.append(["setMapper", [wrapId(mapperId)]])

        prop = None
        if hasattr(actor, "GetProperty"):
            prop = actor.GetProperty()
        else:
            logger.debug("This actor does not have a GetProperty method")

        if prop:
            propId = getReferenceId(prop)
            propertyInstance = serializeInstance(
                actor, prop, propId, context, depth + 1
            )
            if propertyInstance:
                dependencies.append(propertyInstance)
                calls.append(["setProperty", [wrapId(propId)]])

    if actorVisibility == 0 or (mapperInstance and propertyInstance):
        return {
            "parent": getReferenceId(parent),
            "id": actorId,
            "type": class_name(actor),
            "properties": {
                # vtkProp
                "visibility": actorVisibility,
                "pickable": actor.GetPickable(),
                "dragable": actor.GetDragable(),
                "useBounds": actor.GetUseBounds(),
                # vtkProp3D
                "origin": actor.GetOrigin(),
                "position": actor.GetPosition(),
                "scale": actor.GetScale(),
            },
            "calls": calls,
            "dependencies": dependencies,
        }

    return None

# -----------------------------------------------------------------------------


def textureSerializer(parent, texture, textureId, context, depth):
    # This kind of mapper requires us to get 2 items: input data and lookup
    # table
    dataObject = None
    dataObjectInstance = None
    calls = []
    dependencies = []

    if hasattr(texture, "GetInput"):
        dataObject = texture.GetInput()
    else:
        logger.debug("This texture does not have GetInput method")

    if dataObject:
        dataObjectId = "%s-texture" % textureId
        dataObjectInstance = serializeInstance(
            texture, dataObject, dataObjectId, context, depth + 1
        )
        if dataObjectInstance:
            dependencies.append(dataObjectInstance)
            calls.append(["setInputData", [wrapId(dataObjectId)]])

    if dataObjectInstance:
        return {
            "parent": getReferenceId(parent),
            "id": textureId,
            "type": "vtkTexture",
            "properties": {
                "interpolate": texture.GetInterpolate(),
                "repeat": texture.GetRepeat(),
                "edgeClamp": texture.GetEdgeClamp(),
            },
            "calls": calls,
            "dependencies": dependencies,
        }

    return None


# -----------------------------------------------------------------------------


def genericMapperSerializer(parent, mapper, mapperId, context, depth):
    # This kind of mapper requires us to get 2 items: input data and lookup
    # table
    dataObject = None
    dataObjectInstance = None
    lookupTableInstance = None
    calls = []
    dependencies = []

    if hasattr(mapper, "GetInputDataObject"):
        mapper.GetInputAlgorithm().Update()
        dataObject = mapper.GetInputDataObject(0, 0)
    else:
        logger.debug("This mapper does not have GetInputDataObject method")

    if dataObject:
        if dataObject.IsA("vtkDataSet"):
            alg = vtkDataSetSurfaceFilter()
            alg.SetInputData(dataObject)
            alg.Update()
            dataObject = alg.GetOutput()

        dataObjectId = "%s-dataset" % mapperId
        dataObjectInstance = serializeInstance(
            mapper, dataObject, dataObjectId, context, depth + 1
        )

        if dataObjectInstance:
            dependencies.append(dataObjectInstance)
            calls.append(["setInputData", [wrapId(dataObjectId)]])

    lookupTable = None

    if hasattr(mapper, "GetLookupTable"):
        lookupTable = mapper.GetLookupTable()
    else:
        logger.debug("This mapper does not have GetLookupTable method")

    if lookupTable:
        lookupTableId = getReferenceId(lookupTable)
        lookupTableInstance = serializeInstance(
            mapper, lookupTable, lookupTableId, context, depth + 1
        )
        if lookupTableInstance:
            dependencies.append(lookupTableInstance)
            calls.append(
                ["setLookupTable", [wrapId(lookupTableId)]]
            )

    if dataObjectInstance:
        colorArrayName = (
            mapper.GetArrayName()
            if mapper.GetArrayAccessMode() == 1
            else mapper.GetArrayId()
        )
        return {
            "parent": getReferenceId(parent),
            "id": mapperId,
            "type": class_name(mapper),
            "properties": {
                "resolveCoincidentTopology": mapper.GetResolveCoincidentTopology(),
                "renderTime": mapper.GetRenderTime(),
                "arrayAccessMode": mapper.GetArrayAccessMode(),
                "scalarRange": mapper.GetScalarRange(),
                "useLookupTableScalarRange": 1
                if mapper.GetUseLookupTableScalarRange()
                else 0,
                "scalarVisibility": mapper.GetScalarVisibility(),
                "colorByArrayName": colorArrayName,
                "colorMode": mapper.GetColorMode(),
                "scalarMode": mapper.GetScalarMode(),
                "interpolateScalarsBeforeMapping": 1
                if mapper.GetInterpolateScalarsBeforeMapping()
                else 0,
            },
            "calls": calls,
            "dependencies": dependencies,
        }

    return None


# -----------------------------------------------------------------------------


def genericVolumeMapperSerializer(parent, mapper, mapperId, context, depth):
    # This kind of mapper requires us to get 2 items: input data and lookup
    # table
    dataObject = None
    dataObjectInstance = None
    lookupTableInstance = None
    calls = []
    dependencies = []

    if hasattr(mapper, "GetInputDataObject"):
        mapper.GetInputAlgorithm().Update()
        dataObject = mapper.GetInputDataObject(0, 0)
    else:
        logger.debug("This mapper does not have GetInputDataObject method")

    if dataObject:
        dataObjectId = "%s-dataset" % mapperId
        dataObjectInstance = serializeInstance(
            mapper, dataObject, dataObjectId, context, depth + 1
        )

        if dataObjectInstance:
            dependencies.append(dataObjectInstance)
            calls.append(["setInputData", [wrapId(dataObjectId)]])

    if dataObjectInstance:
        return {
            "parent": getReferenceId(parent),
            "id": mapperId,
            "type": class_name(mapper),
            "properties": {
                # VolumeMapper
                "sampleDistance": mapper.GetSampleDistance(),
                "imageSampleDistance": mapper.GetImageSampleDistance(),
                # "maximumSamplesPerRay": mapper.GetMaximumSamplesPerRay(),
                "autoAdjustSampleDistances": mapper.GetAutoAdjustSampleDistances(),
                "blendMode": mapper.GetBlendMode(),
                # "ipScalarRange": mapper.GetIpScalarRange(),
                # "filterMode": mapper.GetFilterMode(),
                # "preferSizeOverAccuracy": mapper.Get(),
            },
            "calls": calls,
            "dependencies": dependencies,
        }

    return None

# -----------------------------------------------------------------------------


def lookupTableSerializer(parent, lookupTable, lookupTableId, context, depth):
    # No children in this case, so no additions to bindings and return empty list
    # But we do need to add instance

    lookupTableRange = lookupTable.GetRange()

    lookupTableHueRange = [0.5, 0]
    if hasattr(lookupTable, "GetHueRange"):
        try:
            lookupTable.GetHueRange(lookupTableHueRange)
        except Exception as inst:
            pass

    lutSatRange = lookupTable.GetSaturationRange()
    lutAlphaRange = lookupTable.GetAlphaRange()

    return {
        "parent": getReferenceId(parent),
        "id": lookupTableId,
        "type": class_name(lookupTable),
        "properties": {
            "numberOfColors": lookupTable.GetNumberOfColors(),
            "valueRange": lookupTableRange,
            "hueRange": lookupTableHueRange,
            # 'alphaRange': lutAlphaRange,  # Causes weird rendering artifacts on client
            "saturationRange": lutSatRange,
            "nanColor": lookupTable.GetNanColor(),
            "belowRangeColor": lookupTable.GetBelowRangeColor(),
            "aboveRangeColor": lookupTable.GetAboveRangeColor(),
            "useAboveRangeColor": True
            if lookupTable.GetUseAboveRangeColor()
            else False,
            "useBelowRangeColor": True
            if lookupTable.GetUseBelowRangeColor()
            else False,
            "alpha": lookupTable.GetAlpha(),
            "vectorSize": lookupTable.GetVectorSize(),
            "vectorComponent": lookupTable.GetVectorComponent(),
            "vectorMode": lookupTable.GetVectorMode(),
            "indexedLookup": lookupTable.GetIndexedLookup(),
        },
    }


# -----------------------------------------------------------------------------


def lookupTableToColorTransferFunction(lookupTable):
    dataTable = lookupTable.GetTable()
    table = dataTableToList(dataTable)
    if table:
        ctf = vtkColorTransferFunction()
        tableRange = lookupTable.GetTableRange()
        points = linspace(*tableRange, num=len(table))
        for x, rgba in zip(points, table):
            ctf.AddRGBPoint(x, *[x / 255 for x in rgba[:3]])

        return ctf

    return None


def lookupTableSerializer2(parent, lookupTable, lookupTableId, context, depth):
    ctf = lookupTableToColorTransferFunction(lookupTable)
    if ctf:
        return colorTransferFunctionSerializer(
            parent, ctf, lookupTableId, context, depth
        )

    return None


# -----------------------------------------------------------------------------


def propertySerializer(parent, propObj, propObjId, context, depth):
    representation = (
        propObj.GetRepresentation() if hasattr(propObj, "GetRepresentation") else 2
    )
    colorToUse = (
        propObj.GetDiffuseColor() if hasattr(propObj, "GetDiffuseColor") else [1, 1, 1]
    )
    if representation == 1 and hasattr(propObj, "GetColor"):
        colorToUse = propObj.GetColor()

    return {
        "parent": getReferenceId(parent),
        "id": propObjId,
        "type": class_name(propObj),
        "properties": {
            "representation": representation,
            "diffuseColor": colorToUse,
            "color": propObj.GetColor(),
            "ambientColor": propObj.GetAmbientColor(),
            "specularColor": propObj.GetSpecularColor(),
            "edgeColor": propObj.GetEdgeColor(),
            "ambient": propObj.GetAmbient(),
            "diffuse": propObj.GetDiffuse(),
            "specular": propObj.GetSpecular(),
            "specularPower": propObj.GetSpecularPower(),
            "opacity": propObj.GetOpacity(),
            "interpolation": propObj.GetInterpolation(),
            "edgeVisibility": 1 if propObj.GetEdgeVisibility() else 0,
            "backfaceCulling": 1 if propObj.GetBackfaceCulling() else 0,
            "frontfaceCulling": 1 if propObj.GetFrontfaceCulling() else 0,
            "pointSize": propObj.GetPointSize(),
            "lineWidth": propObj.GetLineWidth(),
            "lighting": 1 if propObj.GetLighting() else 0,
        },
    }

def volumePropertySerializer(parent, propObj, propObjId, context, depth):
    calls = []
    dependencies = []

    # Color handling
    lut = propObj.GetRGBTransferFunction()
    if lut:
        lookupTableId = getReferenceId(lut)
        lookupTableInstance = serializeInstance(
            propObj, lut, lookupTableId, context, depth + 1
        )

        if lookupTableInstance:
            dependencies.append(lookupTableInstance)
            calls.append(["setRGBTransferFunction", [0, wrapId(lookupTableId)]])

    # Piecewise handling
    pwf = propObj.GetScalarOpacity()
    if pwf:
        pwfId = getReferenceId(pwf)
        pwfInstance = serializeInstance(
            propObj, pwf, pwfId, context, depth + 1
        )

        if pwfInstance:
            dependencies.append(pwfInstance)
            calls.append(["setScalarOpacity", [0, wrapId(pwfId)]])

    return {
        "parent": getReferenceId(parent),
        "id": propObjId,
        "type": class_name(propObj),
        "properties": {
            "independentComponents": propObj.GetIndependentComponents(),
            "interpolationType": propObj.GetInterpolationType(),
            "shade": propObj.GetShade(),
            "ambient": propObj.GetAmbient(),
            "diffuse": propObj.GetDiffuse(),
            "specular": propObj.GetSpecular(),
            "specularPower": propObj.GetSpecularPower(),
            # "useLabelOutline": propObj.GetUseLabelOutline(),
            # "labelOutlineThickness": propObj.GetLabelOutlineThickness(),
        },
        "calls": calls,
        "dependencies": dependencies,
    }

# -----------------------------------------------------------------------------


def imagedataSerializer(parent, dataset, datasetId, context, depth, requested_fields = ["Normals", "TCoords"]):
    if hasattr(dataset, "GetDirectionMatrix"):
        direction = [dataset.GetDirectionMatrix().GetElement(0, i) for i in range(9)]
    else:
        direction = [1, 0, 0, 0, 1, 0, 0, 0, 1]

    # Extract dataset fields
    fields = []
    extractRequiredFields(fields, parent, dataset, context, "*")

    return {
        "parent": getReferenceId(parent),
        "id": datasetId,
        "type": class_name(dataset),
        "properties": {
            "spacing": dataset.GetSpacing(),
            "origin": dataset.GetOrigin(),
            "dimensions": dataset.GetDimensions(),
            "direction": direction,
            "fields": fields,
        },
    }


# -----------------------------------------------------------------------------


def polydataSerializer(parent, dataset, datasetId, context, depth, requested_fields = ["Normals", "TCoords"]):
    if dataset and dataset.GetPoints():
        properties = {}

        # Points
        points = getArrayDescription(dataset.GetPoints().GetData(), context)
        points["vtkClass"] = "vtkPoints"
        properties["points"] = points

        # Verts
        if dataset.GetVerts() and dataset.GetVerts().GetData().GetNumberOfTuples() > 0:
            _verts = getArrayDescription(dataset.GetVerts().GetData(), context)
            properties["verts"] = _verts
            properties["verts"]["vtkClass"] = "vtkCellArray"

        # Lines
        if dataset.GetLines() and dataset.GetLines().GetData().GetNumberOfTuples() > 0:
            _lines = getArrayDescription(dataset.GetLines().GetData(), context)
            properties["lines"] = _lines
            properties["lines"]["vtkClass"] = "vtkCellArray"

        # Polys
        if dataset.GetPolys() and dataset.GetPolys().GetData().GetNumberOfTuples() > 0:
            _polys = getArrayDescription(dataset.GetPolys().GetData(), context)
            properties["polys"] = _polys
            properties["polys"]["vtkClass"] = "vtkCellArray"

        # Strips
        if (
            dataset.GetStrips()
            and dataset.GetStrips().GetData().GetNumberOfTuples() > 0
        ):
            _strips = getArrayDescription(dataset.GetStrips().GetData(), context)
            properties["strips"] = _strips
            properties["strips"]["vtkClass"] = "vtkCellArray"

        # Fields
        properties["fields"] = []
        extractRequiredFields(properties["fields"], parent, dataset, context, requested_fields)

        return {
            "parent": getReferenceId(parent),
            "id": datasetId,
            "type": class_name(dataset),
            "properties": properties,
        }

    logger.debug("This dataset has no points!")
    return None


# -----------------------------------------------------------------------------


def mergeToPolydataSerializer(parent, dataObject, dataObjectId, context, depth, requested_fields=["Normals", "TCoords"]):
    dataset = None

    if dataObject.IsA("vtkCompositeDataSet"):
        gf = vtkCompositeDataGeometryFilter()
        gf.SetInputData(dataObject)
        gf.Update()
        dataset = gf.GetOutput()
    elif dataObject.IsA("vtkUnstructuredGrid"):
        gf = vtkDataSetSurfaceFilter()
        gf.SetInputData(dataObject)
        gf.Update()
        dataset = gf.GetOutput()
    else:
        dataset = mapper.GetInput()

    return polydataSerializer(parent, dataset, dataObjectId, context, depth, requested_fields)


# -----------------------------------------------------------------------------


def colorTransferFunctionSerializer(parent, instance, objId, context, depth):
    nodes = []

    for i in range(instance.GetSize()):
        # x, r, g, b, midpoint, sharpness
        node = [0, 0, 0, 0, 0, 0]
        instance.GetNodeValue(i, node)
        nodes.append(node)

    return {
        "parent": getReferenceId(parent),
        "id": objId,
        "type": class_name(instance),
        "properties": {
            "clamping": 1 if instance.GetClamping() else 0,
            "colorSpace": instance.GetColorSpace(),
            "hSVWrap": 1 if instance.GetHSVWrap() else 0,
            # 'nanColor': instance.GetNanColor(),                  # Breaks client
            # 'belowRangeColor': instance.GetBelowRangeColor(),    # Breaks client
            # 'aboveRangeColor': instance.GetAboveRangeColor(),    # Breaks client
            # 'useAboveRangeColor': 1 if instance.GetUseAboveRangeColor() else 0,
            # 'useBelowRangeColor': 1 if instance.GetUseBelowRangeColor() else 0,
            "allowDuplicateScalars": 1 if instance.GetAllowDuplicateScalars() else 0,
            "alpha": instance.GetAlpha(),
            "vectorComponent": instance.GetVectorComponent(),
            "vectorSize": instance.GetVectorSize(),
            "vectorMode": instance.GetVectorMode(),
            "indexedLookup": instance.GetIndexedLookup(),
            "nodes": nodes,
        },
    }

def discretizableColorTransferFunctionSerializer(parent, instance, objId, context, depth):
    ctf = colorTransferFunctionSerializer(parent, instance, objId, context, depth)
    ctf["properties"]["discretize"] = instance.GetDiscretize()
    ctf["properties"]["numberOfValues"] = instance.GetNumberOfValues()
    return ctf

# -----------------------------------------------------------------------------

def pwfSerializer(parent, instance, objId, context, depth):
    nodes = []

    for i in range(instance.GetSize()):
        # x, y, midpoint, sharpness
        node = [0, 0, 0, 0]
        instance.GetNodeValue(i, node)
        nodes.append(node)

    return {
        "parent": getReferenceId(parent),
        "id": objId,
        "type": class_name(instance),
        "properties": {
            "range": list(instance.GetRange()),
            "clamping": instance.GetClamping(),
            "allowDuplicateScalars": instance.GetAllowDuplicateScalars(),
            "nodes": nodes,
        },
    }

# -----------------------------------------------------------------------------

def cubeAxesSerializer(parent, actor, actorId, context, depth):
    """
    Possible add-on properties for vtk.js:
        gridLines: True,
        axisLabels: None,
        axisTitlePixelOffset: 35.0,
        axisTextStyle: {
            fontColor: 'white',
            fontStyle: 'normal',
            fontSize: 18,
            fontFamily: 'serif',
        },
        tickLabelPixelOffset: 12.0,
        tickTextStyle: {
            fontColor: 'white',
            fontStyle: 'normal',
            fontSize: 14,
            fontFamily: 'serif',
        },
    """
    axisLabels = ["", "", ""]
    if actor.GetXAxisLabelVisibility():
        axisLabels[0] = actor.GetXTitle()
    if actor.GetYAxisLabelVisibility():
        axisLabels[1] = actor.GetYTitle()
    if actor.GetZAxisLabelVisibility():
        axisLabels[2] = actor.GetZTitle()

    return {
        "parent": getReferenceId(parent),
        "id": actorId,
        "type": "vtkCubeAxesActor",
        "properties": {
            # vtkProp
            "visibility": actor.GetVisibility(),
            "pickable": actor.GetPickable(),
            "dragable": actor.GetDragable(),
            "useBounds": actor.GetUseBounds(),
            # vtkProp3D
            "origin": actor.GetOrigin(),
            "position": actor.GetPosition(),
            "scale": actor.GetScale(),
            # vtkActor
            "forceOpaque": actor.GetForceOpaque(),
            "forceTranslucent": actor.GetForceTranslucent(),
            # vtkCubeAxesActor
            "dataBounds": actor.GetBounds(),
            "faceVisibilityAngle": 8,
            "gridLines": True,
            "axisLabels": axisLabels,
            "axisTitlePixelOffset": 35.0,
            "axisTextStyle": {
                "fontColor": "white",
                "fontStyle": "normal",
                "fontSize": 18,
                "fontFamily": "serif",
            },
            "tickLabelPixelOffset": 12.0,
            "tickTextStyle": {
                "fontColor": "white",
                "fontStyle": "normal",
                "fontSize": 14,
                "fontFamily": "serif",
            },
        },
        "calls": [["setCamera", [wrapId(getReferenceId(actor.GetCamera()))]]],
        "dependencies": [],
    }

# -----------------------------------------------------------------------------

def scalarBarActorSerializer(parent, actor, actorId, context, depth):
    dependencies = []
    calls = []
    lut = actor.GetLookupTable()
    if not lut:
        return None

    lutId = getReferenceId(lut)
    lutInstance = serializeInstance(actor, lut, lutId, context, depth + 1)
    if not lutInstance:
        return None

    dependencies.append(lutInstance)
    calls.append(["setScalarsToColors", [wrapId(lutId)]])

    prop = None
    if hasattr(actor, "GetProperty"):
        prop = actor.GetProperty()
    else:
        logger.debug("This scalarBarActor does not have a GetProperty method")

        if prop:
            propId = getReferenceId(prop)
            propertyInstance = serializeInstance(
                actor, prop, propId, context, depth + 1
            )
            if propertyInstance:
                dependencies.append(propertyInstance)
                calls.append(["setProperty", [wrapId(propId)]])

    axisLabel = actor.GetTitle()
    width = actor.GetWidth()
    height = actor.GetHeight()

    return {
        "parent": getReferenceId(parent),
        "id": actorId,
        "type": "vtkScalarBarActor",
        "properties": {
            # vtkProp
            "visibility": actor.GetVisibility(),
            "pickable": actor.GetPickable(),
            "dragable": actor.GetDragable(),
            "useBounds": actor.GetUseBounds(),
            # vtkActor2D
            # "position": actor.GetPosition(),
            # "position2": actor.GetPosition2(),
            # "width": actor.GetWidth(),
            # "height": actor.GetHeight(),
            # vtkScalarBarActor
            "automated": True,
            "axisLabel": axisLabel,
            # 'barPosition': [0, 0],
            # 'barSize': [0, 0],
            "boxPosition": [0.88, -0.92],
            "boxSize": [width, height],
            "axisTitlePixelOffset": 36.0,
            "axisTextStyle": {
                "fontColor": actor.GetTitleTextProperty().GetColor(),
                "fontStyle": "normal",
                "fontSize": 18,
                "fontFamily": "serif",
            },
            "tickLabelPixelOffset": 14.0,
            "tickTextStyle": {
                "fontColor": actor.GetTitleTextProperty().GetColor(),
                "fontStyle": "normal",
                "fontSize": 14,
                "fontFamily": "serif",
            },
            "drawNanAnnotation": actor.GetDrawNanAnnotation(),
            "drawBelowRangeSwatch": actor.GetDrawBelowRangeSwatch(),
            "drawAboveRangeSwatch": actor.GetDrawAboveRangeSwatch(),
        },
        "calls": calls,
        "dependencies": dependencies,
    }

# -----------------------------------------------------------------------------


def rendererSerializer(parent, instance, objId, context, depth):
    dependencies = []
    viewPropIds = []
    lightsIds = []
    calls = []

    # Camera
    camera = instance.GetActiveCamera()
    cameraId = getReferenceId(camera)
    cameraInstance = serializeInstance(instance, camera, cameraId, context, depth + 1)
    if cameraInstance:
        dependencies.append(cameraInstance)
        calls.append(["setActiveCamera", [wrapId(cameraId)]])

    # View prop as representation containers
    viewPropCollection = instance.GetViewProps()
    for rpIdx in range(viewPropCollection.GetNumberOfItems()):
        viewProp = viewPropCollection.GetItemAsObject(rpIdx)
        viewPropId = getReferenceId(viewProp)

        viewPropInstance = serializeInstance(
            instance, viewProp, viewPropId, context, depth + 1
        )
        if viewPropInstance:
            dependencies.append(viewPropInstance)
            viewPropIds.append(viewPropId)

    calls += context.buildDependencyCallList(
        "%s-props" % objId, viewPropIds, "addViewProp", "removeViewProp"
    )

    # Lights
    lightCollection = instance.GetLights()
    for lightIdx in range(lightCollection.GetNumberOfItems()):
        light = lightCollection.GetItemAsObject(lightIdx)
        lightId = getReferenceId(light)

        lightInstance = serializeInstance(instance, light, lightId, context, depth + 1)
        if lightInstance:
            dependencies.append(lightInstance)
            lightsIds.append(lightId)

    calls += context.buildDependencyCallList(
        "%s-lights" % objId, lightsIds, "addLight", "removeLight"
    )

    if len(dependencies) > 1:
        return {
            "parent": getReferenceId(parent),
            "id": objId,
            "type": class_name(instance),
            "properties": {
                "background": instance.GetBackground(),
                "background2": instance.GetBackground2(),
                "viewport": instance.GetViewport(),
                # These commented properties do not yet have real setters in vtk.js
                # 'gradientBackground': instance.GetGradientBackground(),
                # 'aspect': instance.GetAspect(),
                # 'pixelAspect': instance.GetPixelAspect(),
                # 'ambient': instance.GetAmbient(),
                "twoSidedLighting": instance.GetTwoSidedLighting(),
                "lightFollowCamera": instance.GetLightFollowCamera(),
                "layer": instance.GetLayer(),
                "preserveColorBuffer": instance.GetPreserveColorBuffer(),
                "preserveDepthBuffer": instance.GetPreserveDepthBuffer(),
                "nearClippingPlaneTolerance": instance.GetNearClippingPlaneTolerance(),
                "clippingRangeExpansion": instance.GetClippingRangeExpansion(),
                "useShadows": instance.GetUseShadows(),
                "useDepthPeeling": instance.GetUseDepthPeeling(),
                "occlusionRatio": instance.GetOcclusionRatio(),
                "maximumNumberOfPeels": instance.GetMaximumNumberOfPeels(),
                "interactive": instance.GetInteractive(),
            },
            "dependencies": dependencies,
            "calls": calls,
        }

    return None


# -----------------------------------------------------------------------------


def cameraSerializer(parent, instance, objId, context, depth):
    return {
        "parent": getReferenceId(parent),
        "id": objId,
        "type": class_name(instance),
        "properties": {
            "focalPoint": instance.GetFocalPoint(),
            "position": instance.GetPosition(),
            "viewUp": instance.GetViewUp(),
            "clippingRange": instance.GetClippingRange(),
        },
    }


# -----------------------------------------------------------------------------


def lightTypeToString(value):
    """
    #define VTK_LIGHT_TYPE_HEADLIGHT    1
    #define VTK_LIGHT_TYPE_CAMERA_LIGHT 2
    #define VTK_LIGHT_TYPE_SCENE_LIGHT  3

    'HeadLight';
    'SceneLight';
    'CameraLight'
    """
    if value == 1:
        return "HeadLight"
    elif value == 2:
        return "CameraLight"

    return "SceneLight"


def lightSerializer(parent, instance, objId, context, depth):
    return {
        "parent": getReferenceId(parent),
        "id": objId,
        "type": class_name(instance),
        "properties": {
            # 'specularColor': instance.GetSpecularColor(),
            # 'ambientColor': instance.GetAmbientColor(),
            "switch": instance.GetSwitch(),
            "intensity": instance.GetIntensity(),
            "color": instance.GetDiffuseColor(),
            "position": instance.GetPosition(),
            "focalPoint": instance.GetFocalPoint(),
            "positional": instance.GetPositional(),
            "exponent": instance.GetExponent(),
            "coneAngle": instance.GetConeAngle(),
            "attenuationValues": instance.GetAttenuationValues(),
            "lightType": lightTypeToString(instance.GetLightType()),
            "shadowAttenuation": instance.GetShadowAttenuation(),
        },
    }


# -----------------------------------------------------------------------------


def renderWindowSerializer(parent, instance, objId, context, depth):
    dependencies = []
    rendererIds = []

    rendererCollection = instance.GetRenderers()
    for rIdx in range(rendererCollection.GetNumberOfItems()):
        # Grab the next vtkRenderer
        renderer = rendererCollection.GetItemAsObject(rIdx)
        rendererId = getReferenceId(renderer)
        rendererInstance = serializeInstance(
            instance, renderer, rendererId, context, depth + 1
        )
        if rendererInstance:
            dependencies.append(rendererInstance)
            rendererIds.append(rendererId)

    calls = context.buildDependencyCallList(
        objId, rendererIds, "addRenderer", "removeRenderer"
    )

    return {
        "parent": getReferenceId(parent),
        "id": objId,
        "type": class_name(instance),
        "properties": {"numberOfLayers": instance.GetNumberOfLayers()},
        "dependencies": dependencies,
        "calls": calls,
        "mtime": instance.GetMTime(),
    }
