import json, os, gzip, shutil

from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
from vtkmodules.vtkIOImage import vtkPNGReader, vtkPNGWriter, vtkJPEGWriter
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkCommonCore import vtkUnsignedCharArray
from vtkmodules.vtkFiltersParallel import vtkPResampleFilter

from vtkmodules.web import iteritems, getJSArrayType
from vtkmodules.web.camera import (
    update_camera,
    create_spherical_camera,
    create_cylindrical_camera,
)
from vtkmodules.web.query_data_model import DataHandler

# Global helper variables
encode_codes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

# -----------------------------------------------------------------------------
# Capture image from render window
# -----------------------------------------------------------------------------


class CaptureRenderWindow(object):
    def __init__(self, magnification=1):
        self.windowToImage = vtkWindowToImageFilter()
        self.windowToImage.SetScale(magnification)
        self.windowToImage.SetInputBufferTypeToRGB()
        self.windowToImage.ReadFrontBufferOn()
        self.writer = None

    def SetRenderWindow(self, renderWindow):
        self.windowToImage.SetInput(renderWindow)

    def SetFormat(self, mimeType):
        if mimeType == "image/png":
            self.writer = vtkPNGWriter()
            self.writer.SetInputConnection(self.windowToImage.GetOutputPort())
        elif mimeType == "image/jpg":
            self.writer = vtkJPEGWriter()
            self.writer.SetInputConnection(self.windowToImage.GetOutputPort())

    def writeImage(self, path):
        if self.writer:
            self.windowToImage.Modified()
            self.windowToImage.Update()
            self.writer.SetFileName(path)
            self.writer.Write()


# -----------------------------------------------------------------------------
# Basic Dataset Builder
# -----------------------------------------------------------------------------


class DataSetBuilder(object):
    def __init__(self, location, camera_data, metadata={}, sections={}):
        self.dataHandler = DataHandler(location)
        self.cameraDescription = camera_data
        self.camera = None
        self.imageCapture = CaptureRenderWindow()

        for key, value in iteritems(metadata):
            self.dataHandler.addMetaData(key, value)

        for key, value in iteritems(sections):
            self.dataHandler.addSection(key, value)

    def getDataHandler(self):
        return self.dataHandler

    def getCamera(self):
        return self.camera

    def updateCamera(self, camera):
        update_camera(self.renderer, camera)
        self.renderWindow.Render()

    def start(self, renderWindow=None, renderer=None):
        if renderWindow:
            # Keep track of renderWindow and renderer
            self.renderWindow = renderWindow
            self.renderer = renderer

            # Initialize image capture
            self.imageCapture.SetRenderWindow(renderWindow)

            # Handle camera if any
            if self.cameraDescription:
                if self.cameraDescription["type"] == "spherical":
                    self.camera = create_spherical_camera(
                        renderer,
                        self.dataHandler,
                        self.cameraDescription["phi"],
                        self.cameraDescription["theta"],
                    )
                elif self.cameraDescription["type"] == "cylindrical":
                    self.camera = create_cylindrical_camera(
                        renderer,
                        self.dataHandler,
                        self.cameraDescription["phi"],
                        self.cameraDescription["translation"],
                    )

            # Update background color
            bgColor = renderer.GetBackground()
            bgColorString = "rgb(%d, %d, %d)" % tuple(
                int(bgColor[i] * 255) for i in range(3)
            )
            self.dataHandler.addMetaData("backgroundColor", bgColorString)

        # Update file patterns
        self.dataHandler.updateBasePattern()

    def stop(self):
        self.dataHandler.writeDataDescriptor()


# -----------------------------------------------------------------------------
# Image Dataset Builder
# -----------------------------------------------------------------------------


class ImageDataSetBuilder(DataSetBuilder):
    def __init__(self, location, imageMimeType, cameraInfo, metadata={}, sections={}):
        DataSetBuilder.__init__(self, location, cameraInfo, metadata, sections)
        imageExtenstion = "." + imageMimeType.split("/")[1]
        self.dataHandler.registerData(
            name="image", type="blob", mimeType=imageMimeType, fileName=imageExtenstion
        )
        self.imageCapture.SetFormat(imageMimeType)

    def writeImage(self):
        self.imageCapture.writeImage(self.dataHandler.getDataAbsoluteFilePath("image"))

    def writeImages(self):
        for cam in self.camera:
            update_camera(self.renderer, cam)
            self.renderWindow.Render()
            self.imageCapture.writeImage(
                self.dataHandler.getDataAbsoluteFilePath("image")
            )


# -----------------------------------------------------------------------------
# Volume Composite Dataset Builder
# -----------------------------------------------------------------------------
class VolumeCompositeDataSetBuilder(DataSetBuilder):
    def __init__(self, location, imageMimeType, cameraInfo, metadata={}, sections={}):
        DataSetBuilder.__init__(self, location, cameraInfo, metadata, sections)

        self.dataHandler.addTypes("volume-composite", "rgba+depth")

        self.imageMimeType = imageMimeType
        self.imageExtenstion = "." + imageMimeType.split("/")[1]

        if imageMimeType == "image/png":
            self.imageWriter = vtkPNGWriter()
        if imageMimeType == "image/jpg":
            self.imageWriter = vtkJPEGWriter()

        self.imageDataColor = vtkImageData()
        self.imageWriter.SetInputData(self.imageDataColor)

        self.imageDataDepth = vtkImageData()
        self.depthToWrite = None

        self.layerInfo = {}
        self.colorByMapping = {}
        self.compositePipeline = {
            "layers": [],
            "dimensions": [],
            "fields": {},
            "layer_fields": {},
            "pipeline": [],
        }
        self.activeDepthKey = ""
        self.activeRGBKey = ""
        self.nodeWithChildren = {}

    def _getColorCode(self, colorBy):
        if colorBy in self.colorByMapping:
            # The color code exist
            return self.colorByMapping[colorBy]
        else:
            # No color code assigned yet
            colorCode = encode_codes[len(self.colorByMapping)]
            # Assign color code
            self.colorByMapping[colorBy] = colorCode
            # Register color code with color by value
            self.compositePipeline["fields"][colorCode] = colorBy
            # Return the color code
            return colorCode

    def _getLayerCode(self, parent, layerName):
        if layerName in self.layerInfo:
            # Layer already exist
            return (self.layerInfo[layerName]["code"], False)
        else:
            layerCode = encode_codes[len(self.layerInfo)]
            self.layerInfo[layerName] = {
                "code": layerCode,
                "name": layerName,
                "parent": parent,
            }
            self.compositePipeline["layers"].append(layerCode)
            self.compositePipeline["layer_fields"][layerCode] = []

            # Let's register it in the pipeline
            if parent:
                if parent not in self.nodeWithChildren:
                    # Need to create parent
                    rootNode = {"name": parent, "ids": [], "children": []}
                    self.nodeWithChildren[parent] = rootNode
                    self.compositePipeline["pipeline"].append(rootNode)

                # Add node to its parent
                self.nodeWithChildren[parent]["children"].append(
                    {"name": layerName, "ids": [layerCode]}
                )
                self.nodeWithChildren[parent]["ids"].append(layerCode)

            else:
                self.compositePipeline["pipeline"].append(
                    {"name": layerName, "ids": [layerCode]}
                )

            return (layerCode, True)

    def _needToRegisterColor(self, layerCode, colorCode):
        if colorCode in self.compositePipeline["layer_fields"][layerCode]:
            return False
        else:
            self.compositePipeline["layer_fields"][layerCode].append(colorCode)
            return True

    def activateLayer(self, parent, name, colorBy):
        layerCode, needToRegisterDepth = self._getLayerCode(parent, name)
        colorCode = self._getColorCode(colorBy)
        needToRegisterColor = self._needToRegisterColor(layerCode, colorCode)

        # Update active keys
        self.activeDepthKey = "%s_depth" % layerCode
        self.activeRGBKey = "%s%s_rgb" % (layerCode, colorCode)

        # Need to register data
        if needToRegisterDepth:
            self.dataHandler.registerData(
                name=self.activeDepthKey,
                type="array",
                fileName="/%s_depth.uint8" % layerCode,
                categories=[layerCode],
            )

        if needToRegisterColor:
            self.dataHandler.registerData(
                name=self.activeRGBKey,
                type="blob",
                fileName="/%s%s_rgb%s" % (layerCode, colorCode, self.imageExtenstion),
                categories=["%s%s" % (layerCode, colorCode)],
                mimeType=self.imageMimeType,
            )

    def writeData(self, mapper):
        width = self.renderWindow.GetSize()[0]
        height = self.renderWindow.GetSize()[1]

        if not self.depthToWrite:
            self.depthToWrite = bytearray(width * height)

        for cam in self.camera:
            self.updateCamera(cam)
            imagePath = self.dataHandler.getDataAbsoluteFilePath(self.activeRGBKey)
            depthPath = self.dataHandler.getDataAbsoluteFilePath(self.activeDepthKey)

            # -----------------------------------------------------------------
            # Write Image
            # -----------------------------------------------------------------
            mapper.GetColorImage(self.imageDataColor)
            self.imageWriter.SetFileName(imagePath)
            self.imageWriter.Write()

            # -----------------------------------------------------------------
            # Write Depth
            # -----------------------------------------------------------------
            mapper.GetDepthImage(self.imageDataDepth)
            inputArray = self.imageDataDepth.GetPointData().GetArray(0)
            size = inputArray.GetNumberOfTuples()
            for idx in range(size):
                self.depthToWrite[idx] = int(inputArray.GetValue(idx))

            with open(depthPath, "wb") as f:
                f.write(self.depthToWrite)

    def start(self, renderWindow, renderer):
        DataSetBuilder.start(self, renderWindow, renderer)
        self.camera.updatePriority([2, 1])

    def stop(self, compress=True):
        # Push metadata
        self.compositePipeline["dimensions"] = self.renderWindow.GetSize()
        self.compositePipeline["default_pipeline"] = (
            "A".join(self.compositePipeline["layers"]) + "A"
        )
        self.dataHandler.addSection("CompositePipeline", self.compositePipeline)

        # Write metadata
        DataSetBuilder.stop(self)

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print("Compress", root)
                for name in files:
                    if ".uint8" in name and ".gz" not in name:
                        with open(os.path.join(root, name), "rb") as f_in:
                            with gzip.open(
                                os.path.join(root, name + ".gz"), "wb"
                            ) as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))


# -----------------------------------------------------------------------------
# Data Prober Dataset Builder
# -----------------------------------------------------------------------------
class DataProberDataSetBuilder(DataSetBuilder):
    def __init__(
        self,
        location,
        sampling_dimesions,
        fields_to_keep,
        custom_probing_bounds=None,
        metadata={},
    ):
        DataSetBuilder.__init__(self, location, None, metadata)
        self.fieldsToWrite = fields_to_keep
        self.resamplerFilter = vtkPResampleFilter()
        self.resamplerFilter.SetSamplingDimension(sampling_dimesions)
        if custom_probing_bounds:
            self.resamplerFilter.SetUseInputBounds(0)
            self.resamplerFilter.SetCustomSamplingBounds(custom_probing_bounds)
        else:
            self.resamplerFilter.SetUseInputBounds(1)

        # Register all fields
        self.dataHandler.addTypes("data-prober", "binary")
        self.DataProber = {
            "types": {},
            "dimensions": sampling_dimesions,
            "ranges": {},
            "spacing": [1, 1, 1],
        }
        for field in self.fieldsToWrite:
            self.dataHandler.registerData(
                name=field, type="array", fileName="/%s.array" % field
            )

    def setDataToProbe(self, dataset):
        self.resamplerFilter.SetInputData(dataset)

    def setSourceToProbe(self, source):
        self.resamplerFilter.SetInputConnection(source.GetOutputPort())

    def writeData(self):
        self.resamplerFilter.Update()
        arrays = self.resamplerFilter.GetOutput().GetPointData()
        for field in self.fieldsToWrite:
            array = arrays.GetArray(field)
            if array:
                b = memoryview(array)
                with open(self.dataHandler.getDataAbsoluteFilePath(field), "wb") as f:
                    f.write(b)

                self.DataProber["types"][field] = getJSArrayType(array)
                if field in self.DataProber["ranges"]:
                    dataRange = array.GetRange()
                    if dataRange[0] < self.DataProber["ranges"][field][0]:
                        self.DataProber["ranges"][field][0] = dataRange[0]
                    if dataRange[1] > self.DataProber["ranges"][field][1]:
                        self.DataProber["ranges"][field][1] = dataRange[1]
                else:
                    self.DataProber["ranges"][field] = [
                        array.GetRange()[0],
                        array.GetRange()[1],
                    ]
            else:
                print("No array for", field)
                print(self.resamplerFilter.GetOutput())

    def stop(self, compress=True):
        # Push metadata
        self.dataHandler.addSection("DataProber", self.DataProber)

        # Write metadata
        DataSetBuilder.stop(self)

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print("Compress", root)
                for name in files:
                    if ".array" in name and ".gz" not in name:
                        with open(os.path.join(root, name), "rb") as f_in:
                            with gzip.open(
                                os.path.join(root, name + ".gz"), "wb"
                            ) as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))


# -----------------------------------------------------------------------------
# Sorted Composite Dataset Builder
# -----------------------------------------------------------------------------
class ConvertVolumeStackToSortedStack(object):
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.layers = 0

    def convert(self, directory):
        imagePaths = {}
        depthPaths = {}
        layerNames = []
        for fileName in os.listdir(directory):
            if "_rgb" in fileName or "_depth" in fileName:
                fileId = fileName.split("_")[0][0]
                if "_rgb" in fileName:
                    imagePaths[fileId] = os.path.join(directory, fileName)
                else:
                    layerNames.append(fileId)
                    depthPaths[fileId] = os.path.join(directory, fileName)

        layerNames.sort()

        if len(layerNames) == 0:
            return

        # Load data in Memory
        depthArrays = []
        imageReader = vtkPNGReader()
        numberOfValues = self.width * self.height * len(layerNames)
        imageSize = self.width * self.height
        self.layers = len(layerNames)

        # Write all images as single memoryview
        opacity = vtkUnsignedCharArray()
        opacity.SetNumberOfComponents(1)
        opacity.SetNumberOfTuples(numberOfValues)

        intensity = vtkUnsignedCharArray()
        intensity.SetNumberOfComponents(1)
        intensity.SetNumberOfTuples(numberOfValues)

        for layer in range(self.layers):
            imageReader.SetFileName(imagePaths[layerNames[layer]])
            imageReader.Update()

            rgbaArray = imageReader.GetOutput().GetPointData().GetArray(0)

            for idx in range(imageSize):
                intensity.SetValue(
                    (layer * imageSize) + idx, rgbaArray.GetValue(idx * 4)
                )
                opacity.SetValue(
                    (layer * imageSize) + idx, rgbaArray.GetValue(idx * 4 + 3)
                )

            with open(depthPaths[layerNames[layer]], "rb") as depthFile:
                depthArrays.append(depthFile.read())

        # Apply pixel sorting
        destOrder = vtkUnsignedCharArray()
        destOrder.SetNumberOfComponents(1)
        destOrder.SetNumberOfTuples(numberOfValues)

        opacityOrder = vtkUnsignedCharArray()
        opacityOrder.SetNumberOfComponents(1)
        opacityOrder.SetNumberOfTuples(numberOfValues)

        intensityOrder = vtkUnsignedCharArray()
        intensityOrder.SetNumberOfComponents(1)
        intensityOrder.SetNumberOfTuples(numberOfValues)

        for pixelIdx in range(imageSize):
            depthStack = []
            for depthArray in depthArrays:
                depthStack.append((depthArray[pixelIdx], len(depthStack)))
            depthStack.sort(key=lambda tup: tup[0])

            for destLayerIdx in range(len(depthStack)):
                sourceLayerIdx = depthStack[destLayerIdx][1]

                # Copy Idx
                destOrder.SetValue(
                    (imageSize * destLayerIdx) + pixelIdx, sourceLayerIdx
                )
                opacityOrder.SetValue(
                    (imageSize * destLayerIdx) + pixelIdx,
                    opacity.GetValue((imageSize * sourceLayerIdx) + pixelIdx),
                )
                intensityOrder.SetValue(
                    (imageSize * destLayerIdx) + pixelIdx,
                    intensity.GetValue((imageSize * sourceLayerIdx) + pixelIdx),
                )

        with open(os.path.join(directory, "alpha.uint8"), "wb") as f:
            f.write(memoryview(opacityOrder))

        with open(os.path.join(directory, "intensity.uint8"), "wb") as f:
            f.write(memoryview(intensityOrder))

        with open(os.path.join(directory, "order.uint8"), "wb") as f:
            f.write(memoryview(destOrder))


class SortedCompositeDataSetBuilder(VolumeCompositeDataSetBuilder):
    def __init__(self, location, cameraInfo, metadata={}, sections={}):
        VolumeCompositeDataSetBuilder.__init__(
            self, location, "image/png", cameraInfo, metadata, sections
        )
        self.dataHandler.addTypes("sorted-composite", "rgba")

        # Register order and color textures
        self.layerScalars = []
        self.dataHandler.registerData(
            name="order", type="array", fileName="/order.uint8"
        )
        self.dataHandler.registerData(
            name="alpha", type="array", fileName="/alpha.uint8"
        )
        self.dataHandler.registerData(
            name="intensity",
            type="array",
            fileName="/intensity.uint8",
            categories=["intensity"],
        )

    def start(self, renderWindow, renderer):
        VolumeCompositeDataSetBuilder.start(self, renderWindow, renderer)
        imageSize = self.renderWindow.GetSize()
        self.dataConverter = ConvertVolumeStackToSortedStack(imageSize[0], imageSize[1])

    def activateLayer(self, colorBy, scalar):
        VolumeCompositeDataSetBuilder.activateLayer(
            self, "root", "%s" % scalar, colorBy
        )
        self.layerScalars.append(scalar)

    def writeData(self, mapper):
        VolumeCompositeDataSetBuilder.writeData(self, mapper)

        # Fill data pattern
        self.dataHandler.getDataAbsoluteFilePath("order")
        self.dataHandler.getDataAbsoluteFilePath("alpha")
        self.dataHandler.getDataAbsoluteFilePath("intensity")

    def stop(self, clean=True, compress=True):
        VolumeCompositeDataSetBuilder.stop(self, compress=False)

        # Go through all directories and convert them
        for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
            for name in dirs:
                print("Process", os.path.join(root, name))
                self.dataConverter.convert(os.path.join(root, name))

        # Rename index.json to info_origin.json
        os.rename(
            os.path.join(self.dataHandler.getBasePath(), "index.json"),
            os.path.join(self.dataHandler.getBasePath(), "index_origin.json"),
        )

        # Update index.json
        with open(
            os.path.join(self.dataHandler.getBasePath(), "index_origin.json"), "r"
        ) as infoFile:
            metadata = json.load(infoFile)
            metadata["SortedComposite"] = {
                "dimensions": metadata["CompositePipeline"]["dimensions"],
                "layers": self.dataConverter.layers,
                "scalars": self.layerScalars[0 : self.dataConverter.layers],
            }

            # Clean metadata
            dataToKeep = []
            del metadata["CompositePipeline"]
            for item in metadata["data"]:
                if item["name"] in ["order", "alpha", "intensity"]:
                    dataToKeep.append(item)
            metadata["data"] = dataToKeep
            metadata["type"] = ["tonic-query-data-model", "sorted-composite", "alpha"]

            # Override index.json
            with open(
                os.path.join(self.dataHandler.getBasePath(), "index.json"), "w"
            ) as newMetaFile:
                newMetaFile.write(json.dumps(metadata))

        # Clean temporary data
        if clean:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print("Clean", root)
                for name in files:
                    if (
                        "_rgb.png" in name
                        or "_depth.uint8" in name
                        or name == "index_origin.json"
                    ):
                        os.remove(os.path.join(root, name))

        if compress:
            for root, dirs, files in os.walk(self.dataHandler.getBasePath()):
                print("Compress", root)
                for name in files:
                    if ".uint8" in name and ".gz" not in name:
                        with open(os.path.join(root, name), "rb") as f_in:
                            with gzip.open(
                                os.path.join(root, name + ".gz"), "wb"
                            ) as f_out:
                                shutil.copyfileobj(f_in, f_out)
                        os.remove(os.path.join(root, name))
