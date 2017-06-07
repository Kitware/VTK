from __future__ import absolute_import, division, print_function

import json, base64, time, re

from vtk.vtkFiltersGeometry import vtkCompositeDataGeometryFilter
from vtk.vtkCommonCore import vtkTypeUInt32Array
from vtk.vtkWebCore import vtkWebApplication

from vtk.web import hashDataArray, getJSArrayType

# -----------------------------------------------------------------------------
# Convenience class for caching data arrays, storing computed sha sums, keeping
# track of valid actors, etc...
# -----------------------------------------------------------------------------

class SynchronizationContext():
  def __init__(self, debug=False):
    self.dataArrayCache = {}
    self.lastDependenciesMapping = {}
    self.ingoreLastDependencies = False
    self.debugSerializers = debug
    self.debugAll = debug

  def setIgnoreLastDependencies(self, force):
    self.ingoreLastDependencies = force

  def cacheDataArray(self, pMd5, data):
    self.dataArrayCache[pMd5] = data

  def getCachedDataArray(self, pMd5):
    cacheObj = self.dataArrayCache[pMd5]
    array = cacheObj['array']
    cacheTime = cacheObj['mTime']

    if cacheTime != array.GetMTime():
      if context.debugAll: print(' ***** ERROR: you asked for an old cache key! ***** ')

    if array.GetDataType() == 12:
      # IdType need to be converted to Uint32
      arraySize = array.GetNumberOfTuples() * array.GetNumberOfComponents()
      newArray = vtkTypeUInt32Array()
      newArray.SetNumberOfTuples(arraySize)
      for i in range(arraySize):
        newArray.SetValue(i, -1 if array.GetValue(i) < 0 else array.GetValue(i))
      pBuffer = buffer(newArray)
    else:
      pBuffer = buffer(array)

    return base64.b64encode(pBuffer)

  def checkForArraysToRelease(self, timeWindow = 20):
    cutOffTime = time.time() - timeWindow
    shasToDelete = []
    for sha in self.dataArrayCache:
      record = self.dataArrayCache[sha]
      array = record['array']
      count = array.GetReferenceCount()

      if count == 1 and record['ts'] < cutOffTime:
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
    calls += [ [addMethod, [ wrapId(x) ]] for x in newList if x not in oldList ]
    calls += [ [removeMethod, [ wrapId(x) ]] for x in oldList if x not in newList ]

    self.setNewDependencyList(idstr, newList)
    return calls

# -----------------------------------------------------------------------------
# Global variables
# -----------------------------------------------------------------------------

SERIALIZERS = {}
context = None

# -----------------------------------------------------------------------------
# Global API
# -----------------------------------------------------------------------------

def registerInstanceSerializer(name, method):
  global SERIALIZERS
  SERIALIZERS[name] = method

# -----------------------------------------------------------------------------

def serializeInstance(parent, instance, instanceId, context, depth):
  instanceType = instance.GetClassName()
  serializer = SERIALIZERS[instanceType] if instanceType in SERIALIZERS else None

  if serializer:
    return serializer(parent, instance, instanceId, context, depth)

  if context.debugSerializers:
    print('%s!!!No serializer for %s with id %s' % (pad(depth), instanceType, instanceId))

  return None

# -----------------------------------------------------------------------------

def initializeSerializers():
  # Actors/viewProps
  registerInstanceSerializer('vtkOpenGLActor', genericActorSerializer)
  registerInstanceSerializer('vtkPVLODActor', genericActorSerializer)

  # Mappers
  registerInstanceSerializer('vtkOpenGLPolyDataMapper', genericMapperSerializer)
  registerInstanceSerializer('vtkCompositePolyDataMapper2', genericMapperSerializer)

  # LookupTables/TransferFunctions
  registerInstanceSerializer('vtkLookupTable', lookupTableSerializer)
  registerInstanceSerializer('vtkPVDiscretizableColorTransferFunction', colorTransferFunctionSerializer)

  # Property
  registerInstanceSerializer('vtkOpenGLProperty', propertySerializer)

  # Datasets
  registerInstanceSerializer('vtkPolyData', polydataSerializer)
  registerInstanceSerializer('vtkMultiBlockDataSet', mergeToPolydataSerializer)

  # RenderWindows
  registerInstanceSerializer('vtkCocoaRenderWindow', renderWindowSerializer)
  registerInstanceSerializer('vtkXOpenGLRenderWindow', renderWindowSerializer)
  registerInstanceSerializer('vtkWin32OpenGLRenderWindow', renderWindowSerializer)

  # Renderers
  registerInstanceSerializer('vtkOpenGLRenderer', rendererSerializer)

  # Cameras
  registerInstanceSerializer('vtkOpenGLCamera', cameraSerializer)

# -----------------------------------------------------------------------------
# Helper functions
# -----------------------------------------------------------------------------

def pad(depth):
  padding = ''
  for d in range(depth):
    padding += '  '
  return padding

# -----------------------------------------------------------------------------

def wrapId(idStr):
  return 'instance:${%s}' % idStr

# -----------------------------------------------------------------------------

def getReferenceId(ref):
  return vtkWebApplication.GetObjectId(ref)

# -----------------------------------------------------------------------------

dataArrayShaMapping = {}

def digest(array):
  objId = getReferenceId(array)

  record = None
  if objId in dataArrayShaMapping:
    record = dataArrayShaMapping[objId]

  if record and record['mtime'] == array.GetMTime():
    return record['sha']

  record = {
    'sha': hashDataArray(array),
    'mtime': array.GetMTime()
  }

  dataArrayShaMapping[objId] = record
  return record['sha']

# -----------------------------------------------------------------------------

def getRangeInfo(array, component):
  r = array.GetRange(component)
  compRange = {}
  compRange['min'] = r[0]
  compRange['max'] = r[1]
  compRange['component'] = array.GetComponentName(component)
  return compRange

# -----------------------------------------------------------------------------

def getArrayDescription(array, context):
  if not array:
    return None

  pMd5 = digest(array)
  context.cacheDataArray(pMd5, {
    'array': array,
    'mTime': array.GetMTime(),
    'ts': time.time()
  })

  root = {}
  root['hash'] = pMd5
  root['vtkClass'] = 'vtkDataArray'
  root['name'] = array.GetName()
  root['dataType'] = getJSArrayType(array)
  root['numberOfComponents'] = array.GetNumberOfComponents()
  root['size'] = array.GetNumberOfComponents() * array.GetNumberOfTuples()
  root['ranges'] = []
  if root['numberOfComponents'] > 1:
    for i in range(root['numberOfComponents']):
      root['ranges'].append(getRangeInfo(array, i))
    root['ranges'].append(getRangeInfo(array, -1))
  else:
    root['ranges'].append(getRangeInfo(array, 0))

  return root

# -----------------------------------------------------------------------------

def extractRequiredFields(extractedFields, mapper, dataset, context, requestedFields=['Normals', 'TCoords']):
  # FIXME should evolve and support funky mapper which leverage many arrays
  if mapper.IsA('vtkMapper'):
    scalarVisibility = mapper.GetScalarVisibility()
    arrayAccessMode = mapper.GetArrayAccessMode()
    colorArrayName = mapper.GetArrayName() if arrayAccessMode == 1 else mapper.GetArrayId()
    colorMode = mapper.GetColorMode()
    scalarMode = mapper.GetScalarMode()
    if scalarMode == 3:
      arrayMeta = getArrayDescription(dataset.GetPointData().GetArray(colorArrayName), context)
      if arrayMeta:
        arrayMeta['location'] = 'pointData';
        extractedFields.append(arrayMeta)
    if scalarMode == 4:
      arrayMeta = getArrayDescription(dataset.GetCellData().GetArray(colorArrayName), context)
      if arrayMeta:
        arrayMeta['location'] = 'cellData';
        extractedFields.append(arrayMeta)

  # Normal handling
  if 'Normals' in requestedFields:
    normals = dataset.GetPointData().GetNormals()
    if normals:
      arrayMeta = getArrayDescription(normals, context)
      if arrayMeta:
        arrayMeta['location'] = 'pointData'
        arrayMeta['registration'] = 'setNormals'
        extractedFields.append(arrayMeta)

  # TCoord handling
  if 'TCoords' in requestedFields:
    tcoords = dataset.GetPointData().GetTCoords()
    if tcoords:
      arrayMeta = getArrayDescription(tcoords, context)
      if arrayMeta:
        arrayMeta['location'] = 'pointData'
        arrayMeta['registration'] = 'setTCoords'
        extractedFields.append(arrayMeta)

# -----------------------------------------------------------------------------
# Concrete instance serializers
# -----------------------------------------------------------------------------

def genericActorSerializer(parent, actor, actorId, context, depth):
  # This kind of actor has two "children" of interest, a property and a mapper
  actorVisibility = actor.GetVisibility()
  mapperInstance = None
  propertyInstance = None
  calls = []
  dependencies = []

  if actorVisibility:
    mapper = None
    if not hasattr(actor, 'GetMapper'):
      if context.debugAll: print('This actor does not have a GetMapper method')
    else:
      mapper = actor.GetMapper()

    if mapper:
      mapperId = getReferenceId(mapper)
      mapperInstance = serializeInstance(actor, mapper, mapperId, context, depth + 1)
      if mapperInstance:
        dependencies.append(mapperInstance)
        calls.append(['setMapper', [ wrapId(mapperId) ]])

    prop = None
    if hasattr(actor, 'GetProperty'):
      prop = actor.GetProperty()
    else:
      if context.debugAll: print('This actor does not have a GetProperty method')

    if prop:
      propId = getReferenceId(prop)
      propertyInstance = serializeInstance(actor, prop, propId, context, depth + 1)
      if propertyInstance:
        dependencies.append(propertyInstance)
        calls.append(['setProperty', [ wrapId(propId) ]])

  if mapperInstance and propertyInstance:
    return {
      'parent': getReferenceId(parent),
      'id': actorId,
      'type': actor.GetClassName(),
      'properties': {
        # vtkProp
        'visibility': actorVisibility,
        'pickable': actor.GetPickable(),
        'dragable': actor.GetDragable(),
        'useBounds': actor.GetUseBounds(),
        # vtkProp3D
        'origin': actor.GetOrigin(),
        'position': actor.GetPosition(),
        'scale': actor.GetScale(),
        # vtkActor
        'forceOpaque': actor.GetForceOpaque(),
        'forceTranslucent': actor.GetForceTranslucent()
      },
      'calls': calls,
      'dependencies': dependencies
    }

  return None

# -----------------------------------------------------------------------------

def genericMapperSerializer(parent, mapper, mapperId, context, depth):
  # This kind of mapper requires us to get 2 items: input data and lookup table
  dataObject = None
  dataObjectInstance = None
  lookupTableInstance = None
  calls = []
  dependencies = []

  if hasattr(mapper, 'GetInputDataObject'):
    dataObject = mapper.GetInputDataObject(0, 0)
  else:
    if context.debugAll: print('This mapper does not have GetInputDataObject method')

  if dataObject:
    dataObjectId = '%s-dataset' % mapperId
    dataObjectInstance = serializeInstance(mapper, dataObject, dataObjectId, context, depth + 1)
    if dataObjectInstance:
      dependencies.append(dataObjectInstance)
      calls.append(['setInputData', [ wrapId(dataObjectId) ]])

  lookupTable = None

  if hasattr(mapper, 'GetLookupTable'):
    lookupTable = mapper.GetLookupTable()
  else:
    if context.debugAll: print('This mapper does not have GetLookupTable method')

  if lookupTable:
    lookupTableId = getReferenceId(lookupTable)
    lookupTableInstance = serializeInstance(mapper, lookupTable, lookupTableId, context, depth + 1)
    if lookupTableInstance:
      dependencies.append(lookupTableInstance)
      calls.append(['setLookupTable', [ wrapId(lookupTableId) ]])

  if dataObjectInstance and lookupTableInstance:
    colorArrayName = mapper.GetArrayName() if mapper.GetArrayAccessMode() == 1 else mapper.GetArrayId()
    return {
      'parent': getReferenceId(parent),
      'id': mapperId,
      'type': mapper.GetClassName(),
      'properties': {
        'scalarRange': mapper.GetScalarRange(),
        'useLookupTableScalarRange': True if mapper.GetUseLookupTableScalarRange() else False,
        'scalarVisibility': mapper.GetScalarVisibility(),
        'colorByArrayName': colorArrayName,
        'colorMode': mapper.GetColorMode(),
        'scalarMode': mapper.GetScalarMode(),
        'interpolateScalarsBeforeMapping': True if mapper.GetInterpolateScalarsBeforeMapping() else False
      },
      'calls': calls,
      'dependencies': dependencies
    }

  return None

# -----------------------------------------------------------------------------

def lookupTableSerializer(parent, lookupTable, lookupTableId, context, depth):
  # No children in this case, so no additions to bindings and return empty list
  # But we do need to add instance

  lookupTableRange = lookupTable.GetRange()

  lookupTableHueRange = [0.5, 0]
  if hasattr(lookupTable, 'GetHueRange'):
    try:
      lookupTable.GetHueRange(lookupTableHueRange)
    except Exception as inst:
      pass

  lutSatRange = lookupTable.GetSaturationRange()
  lutAlphaRange = lookupTable.GetAlphaRange()

  return {
    'parent': getReferenceId(parent),
    'id': lookupTableId,
    'type': lookupTable.GetClassName(),
    'properties': {
      'numberOfColors': lookupTable.GetNumberOfColors(),
      'valueRange': lookupTableRange,
      'hueRange': lookupTableHueRange,
      # 'alphaRange': lutAlphaRange,  # Causes weird rendering artifacts on client
      'saturationRange': lutSatRange,
      'nanColor': lookupTable.GetNanColor(),
      'belowRangeColor': lookupTable.GetBelowRangeColor(),
      'aboveRangeColor': lookupTable.GetAboveRangeColor(),
      'useAboveRangeColor': True if lookupTable.GetUseAboveRangeColor() else False,
      'useBelowRangeColor': True if lookupTable.GetUseBelowRangeColor() else False,
      'alpha': lookupTable.GetAlpha(),
      'vectorSize': lookupTable.GetVectorSize(),
      'vectorComponent': lookupTable.GetVectorComponent(),
      'vectorMode': lookupTable.GetVectorMode(),
      'indexedLookup': lookupTable.GetIndexedLookup()
    }
  }

# -----------------------------------------------------------------------------

def propertySerializer(parent, propObj, propObjId, context, depth):
  representation = propObj.GetRepresentation() if hasattr(propObj, 'GetRepresentation') else 2
  colorToUse = propObj.GetDiffuseColor() if hasattr(propObj, 'GetDiffuseColor') else [1, 1, 1]
  if representation == 1 and hasattr(propObj, 'GetColor'):
    colorToUse = propObj.GetColor()

  return {
    'parent': getReferenceId(parent),
    'id': propObjId,
    'type': propObj.GetClassName(),
    'properties': {
      'representation': representation,
      'diffuseColor': colorToUse,
      'color': propObj.GetColor(),
      'ambientColor': propObj.GetAmbientColor(),
      'specularColor': propObj.GetSpecularColor(),
      'edgeColor': propObj.GetEdgeColor(),
      'ambient': propObj.GetAmbient(),
      'diffuse': propObj.GetDiffuse(),
      'specular': propObj.GetSpecular(),
      'specularPower': propObj.GetSpecularPower(),
      'opacity': propObj.GetOpacity(),
      'interpolation': propObj.GetInterpolation(),
      'edgeVisibility': True if propObj.GetEdgeVisibility() else False,
      'backfaceCulling': True if propObj.GetBackfaceCulling() else False,
      'frontfaceCulling': True if propObj.GetFrontfaceCulling() else False,
      'pointSize': propObj.GetPointSize(),
      'lineWidth': propObj.GetLineWidth(),
      'lighting': propObj.GetLighting()
    }
  }

# -----------------------------------------------------------------------------

def polydataSerializer(parent, dataset, datasetId, context, depth):
  datasetType = dataset.GetClassName()

  if dataset and dataset.GetPoints():
    properties = {}

    # Points
    points = getArrayDescription(dataset.GetPoints().GetData(), context)
    points['vtkClass'] = 'vtkPoints'
    properties['points'] = points

    ## Verts
    if dataset.GetVerts() and dataset.GetVerts().GetData().GetNumberOfTuples() > 0:
      _verts = getArrayDescription(dataset.GetVerts().GetData(), context)
      properties['verts'] = _verts
      properties['verts']['vtkClass'] = 'vtkCellArray'

    ## Lines
    if dataset.GetLines() and dataset.GetLines().GetData().GetNumberOfTuples() > 0:
      _lines = getArrayDescription(dataset.GetLines().GetData(), context)
      properties['lines'] = _lines
      properties['lines']['vtkClass'] = 'vtkCellArray'

    ## Polys
    if dataset.GetPolys() and dataset.GetPolys().GetData().GetNumberOfTuples() > 0:
      _polys = getArrayDescription(dataset.GetPolys().GetData(), context)
      properties['polys'] = _polys
      properties['polys']['vtkClass'] = 'vtkCellArray'

    ## Strips
    if dataset.GetStrips() and dataset.GetStrips().GetData().GetNumberOfTuples() > 0:
      _strips = getArrayDescription(dataset.GetStrips().GetData(), context)
      properties['strips'] = _strips
      properties['strips']['vtkClass'] = 'vtkCellArray'

    ## Fields
    properties['fields'] = []
    extractRequiredFields(properties['fields'], parent, dataset, context)

    return {
      'parent': getReferenceId(parent),
      'id': datasetId,
      'type': datasetType,
      'properties': properties
    }

  if context.debugAll: print('This dataset has no points!')
  return None

# -----------------------------------------------------------------------------

def mergeToPolydataSerializer(parent, dataObject, dataObjectId, context, depth):
  dataset = None

  if dataObject.IsA('vtkCompositeDataSet'):
    dataMTime = dataObject.GetMTime()
    gf = vtkCompositeDataGeometryFilter()
    gf.SetInputData(dataObject)
    gf.Update()
    tempDS = gf.GetOutput()
    dataset = tempDS
  else:
    dataset = mapper.GetInput()

  return polydataSerializer(parent, dataset, dataObjectId, context, depth)

# -----------------------------------------------------------------------------

def colorTransferFunctionSerializer(parent, instance, objId, context, depth):
  nodes = []

  for i in range(instance.GetSize()):
    # x, r, g, b, midpoint, sharpness
    node = [0, 0, 0, 0, 0, 0]
    instance.GetNodeValue(i, node)
    nodes.append(node)

  return {
    'parent': getReferenceId(parent),
    'id': objId,
    'type': instance.GetClassName(),
    'properties': {
      'clamping': True if instance.GetClamping() else False,
      'colorSpace': instance.GetColorSpace(),
      'hSVWrap': True if instance.GetHSVWrap() else False,
      # 'nanColor': instance.GetNanColor(),                  # Breaks client
      # 'belowRangeColor': instance.GetBelowRangeColor(),    # Breaks client
      # 'aboveRangeColor': instance.GetAboveRangeColor(),    # Breaks client
      # 'useAboveRangeColor': True if instance.GetUseAboveRangeColor() else False,
      # 'useBelowRangeColor': True if instance.GetUseBelowRangeColor() else False,
      'allowDuplicateScalars': True if instance.GetAllowDuplicateScalars() else False,
      'alpha': instance.GetAlpha(),
      'vectorComponent': instance.GetVectorComponent(),
      'vectorSize': instance.GetVectorSize(),
      'vectorMode': instance.GetVectorMode(),
      'indexedLookup': instance.GetIndexedLookup(),
      'nodes': nodes
    }
  }

# -----------------------------------------------------------------------------

def rendererSerializer(parent, instance, objId, context, depth):
  dependencies = []
  viewPropIds = []
  calls = []

  # Camera
  camera = instance.GetActiveCamera()
  cameraId = getReferenceId(camera)
  cameraBindings = []
  cameraInstance = serializeInstance(instance, camera, cameraId, context, depth + 1)
  if cameraInstance:
    dependencies.append(cameraInstance)
    calls.append(['setActiveCamera', [ wrapId(cameraId) ]])

  # View prop as representation containers
  viewPropCollection = instance.GetViewProps()
  for rpIdx in range(viewPropCollection.GetNumberOfItems()):
    viewProp = viewPropCollection.GetItemAsObject(rpIdx)
    viewPropId = getReferenceId(viewProp)

    viewPropInstance = serializeInstance(instance, viewProp, viewPropId, context, depth + 1)
    if viewPropInstance:
      dependencies.append(viewPropInstance)
      viewPropIds.append(viewPropId)

  calls += context.buildDependencyCallList(objId, viewPropIds, 'addViewProp', 'removeViewProp')

  if len(dependencies) > 1:
    return {
      'parent': getReferenceId(parent),
      'id': objId,
      'type': instance.GetClassName(),
      'properties': {
        'background': instance.GetBackground(),
        'background2': instance.GetBackground2(),
        'viewport': instance.GetViewport(),
        ### These commented properties do not yet have real setters in vtk.js
        # 'gradientBackground': instance.GetGradientBackground(),
        # 'aspect': instance.GetAspect(),
        # 'pixelAspect': instance.GetPixelAspect(),
        # 'ambient': instance.GetAmbient(),
        'twoSidedLighting': instance.GetTwoSidedLighting(),
        'lightFollowCamera': instance.GetLightFollowCamera(),
        'layer': instance.GetLayer(),
        'preserveColorBuffer': instance.GetPreserveColorBuffer(),
        'preserveDepthBuffer': instance.GetPreserveDepthBuffer(),
        'nearClippingPlaneTolerance': instance.GetNearClippingPlaneTolerance(),
        'clippingRangeExpansion': instance.GetClippingRangeExpansion(),
        'useShadows': instance.GetUseShadows(),
        'useDepthPeeling': instance.GetUseDepthPeeling(),
        'occlusionRatio': instance.GetOcclusionRatio(),
        'maximumNumberOfPeels': instance.GetMaximumNumberOfPeels()
      },
      'dependencies': dependencies,
      'calls': calls
    }

  return None

# -----------------------------------------------------------------------------

def cameraSerializer(parent, instance, objId, context, depth):
  return {
    'parent': getReferenceId(parent),
    'id': objId,
    'type': instance.GetClassName(),
    'properties': {
      'focalPoint': instance.GetFocalPoint(),
      'position': instance.GetPosition(),
      'viewUp': instance.GetViewUp(),
    }
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
    rendererInstance = serializeInstance(instance, renderer, rendererId, context, depth + 1)
    if rendererInstance:
      dependencies.append(rendererInstance)
      rendererIds.append(rendererId)

  calls = context.buildDependencyCallList(objId, rendererIds, 'addRenderer', 'removeRenderer')

  return {
    'parent': getReferenceId(parent),
    'id': objId,
    'type': instance.GetClassName(),
    'properties': {
      'numberOfLayers': instance.GetNumberOfLayers()
    },
    'dependencies': dependencies,
    'calls': calls
  }
