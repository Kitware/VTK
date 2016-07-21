from vtk import *
from math import *

# -----------------------------------------------------------------------------
# Set of helper functions
# -----------------------------------------------------------------------------

def normalize(vect, tolerance=0.00001):
    mag2 = sum(n * n for n in vect)
    if abs(mag2 - 1.0) > tolerance:
        mag = sqrt(mag2)
        vect = tuple(n / mag for n in vect)
    return vect

def q_mult(q1, q2):
    w1, x1, y1, z1 = q1
    w2, x2, y2, z2 = q2
    w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2
    x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2
    y = w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2
    z = w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2
    return w, x, y, z

def q_conjugate(q):
    w, x, y, z = q
    return (w, -x, -y, -z)

def qv_mult(q1, v1):
    q2 = (0.0,) + v1
    return q_mult(q_mult(q1, q2), q_conjugate(q1))[1:]

def axisangle_to_q(v, theta):
    v = normalize(v)
    x, y, z = v
    theta /= 2
    w = cos(theta)
    x = x * sin(theta)
    y = y * sin(theta)
    z = z * sin(theta)
    return w, x, y, z

def vectProduct(axisA, axisB):
    xa, ya, za = axisA
    xb, yb, zb = axisB
    normalVect = (ya*zb - za*yb, za*xb - xa*zb, xa*yb - ya*xb)
    normalVect = normalize(normalVect)
    return normalVect

def dotProduct(vecA, vecB):
    return (vecA[0] * vecB[0]) + (vecA[1] * vecB[1]) + (vecA[2] * vecB[2])

def rotate(axis, angle, center, point):
    angleInRad = 3.141592654 * angle / 180.0
    rotation = axisangle_to_q(axis, angleInRad)
    tPoint = tuple((point[i] - center[i]) for i in range(3))
    rtPoint = qv_mult(rotation, tPoint)
    rPoint = tuple((rtPoint[i] + center[i]) for i in range(3))
    return rPoint

# -----------------------------------------------------------------------------
# Spherical Camera
# -----------------------------------------------------------------------------

class SphericalCamera(object):

    def __init__(self, dataHandler, focalPoint, position, phiAxis, phiAngles, thetaAngles):
        self.dataHandler = dataHandler
        self.cameraSettings = []
        self.thetaBind = { "mouse" : { "drag" : { "modifier": 0, "coordinate": 1, "step": 30 , "orientation": 1} } }
        self.phiBind = { "mouse" : { "drag" : { "modifier": 0, "coordinate": 0, "step": 30 , "orientation": 1} } }

        # Convert to serializable type
        fp = tuple(i for i in focalPoint)

        # Register arguments to the data handler
        if len(phiAngles) > 1 and phiAngles[-1] + phiAngles[1] == 360:
            self.dataHandler.registerArgument(priority=0, name='phi', values=phiAngles, ui='slider', loop='modulo', bind=self.phiBind)
        else:
            self.dataHandler.registerArgument(priority=0, name='phi', values=phiAngles, ui='slider', bind=self.phiBind)
        if thetaAngles[0] < 0 and thetaAngles[0] >= -90:
            idx = 0
            for theta in thetaAngles:
                if theta < 0:
                    idx += 1

            self.dataHandler.registerArgument(priority=0, name='theta', values=[ (x+90) for x in thetaAngles ], ui='slider', default=idx, bind=self.thetaBind)
        else:
            self.dataHandler.registerArgument(priority=0, name='theta', values=thetaAngles, ui='slider', bind=self.thetaBind)

        # Compute all camera settings
        for theta in thetaAngles:
            for phi in phiAngles:
                phiPos = rotate(phiAxis, -phi, fp, position)
                thetaAxis = vectProduct(phiAxis, tuple(fp[i]-phiPos[i] for i in range(3)))
                thetaPhiPos = rotate(thetaAxis, theta, fp, phiPos)
                viewUp = rotate(thetaAxis, theta, (0,0,0), phiAxis)

                self.cameraSettings.append({
                    'theta': theta,
                    'thetaIdx': thetaAngles.index(theta),
                    'phi': phi,
                    'phiIdx': phiAngles.index(phi),
                    'focalPoint': fp,
                    'position': thetaPhiPos,
                    'viewUp': viewUp
                })

        self.dataHandler.updateBasePattern()

    def updatePriority(self, priorityList):
        keyList = ['theta', 'phi']
        for idx in range(min(len(priorityList), len(keyList))):
            self.dataHandler.updatePriority(keyList[idx], priorityList[idx])

    def __iter__(self):
        for cameraData in self.cameraSettings:
            self.dataHandler.setArguments(phi=cameraData['phiIdx'], theta=cameraData['thetaIdx'])
            yield cameraData

# -----------------------------------------------------------------------------
# Cylindrical Camera
# -----------------------------------------------------------------------------

class CylindricalCamera(object):

    def __init__(self, dataHandler, focalPoint, position, rotationAxis, phiAngles, translationValues):
        self.dataHandler = dataHandler
        self.cameraSettings = []

        # Register arguments to the data handler
        self.dataHandler.registerArgument(priority=0, name='phi', values=phiAngles, ui='slider', loop='modulo')
        self.dataHandler.registerArgument(priority=0, name='n_pos', values=translationValues, ui='slider')

        # Compute all camera settings
        for translation in translationValues:
            for phi in phiAngles:
                phiPos = rotate(rotationAxis, phi, focalPoint, position)
                newfocalPoint = tuple(focalPoint[i] + (translation*rotationAxis[i]) for i in range(3))
                transPhiPoint = tuple(phiPos[i] + (translation*rotationAxis[i]) for i in range(3))

                self.cameraSettings.append({
                    'n_pos': translation,
                    'n_posIdx': translationValues.index(translation),
                    'phi': phi,
                    'phiIdx': phiAngles.index(phi),
                    'focalPoint': newfocalPoint,
                    'position': transPhiPoint,
                    'viewUp': rotationAxis
                })

        self.dataHandler.updateBasePattern()

    def updatePriority(self, priorityList):
        keyList = ['n_pos', 'phi']
        for idx in range(min(len(priorityList), len(keyList))):
            self.dataHandler.updatePriority(keyList[idx], priorityList[idx])

    def __iter__(self):
        for cameraData in self.cameraSettings:
            self.dataHandler.setArguments(phi=cameraData['phiIdx'], n_pos=cameraData['n_posIdx'])
            yield cameraData

# -----------------------------------------------------------------------------
# MultiView Camera
# -----------------------------------------------------------------------------

class MultiViewCamera(object):

    def __init__(self, dataHandler):
        self.dataHandler = dataHandler
        self.cameraSettings = []
        self.positionNames = []

    def registerViewPoint(self, name, focalPoint, position, viewUp):
        self.cameraSettings.append({'name': name, 'nameIdx': len(self.positionNames), 'focalPoint': focalPoint, 'position': position, 'viewUp': viewUp})
        self.positionNames.append(name)
        self.dataHandler.registerArgument(priority=0, name='multiView', values=self.positionNames)
        self.dataHandler.updateBasePattern()

    def updatePriority(self, priorityList):
        keyList = ['multiView']
        for idx in range(min(len(priorityList), len(keyList))):
            self.dataHandler.updatePriority(keyList[idx], priorityList[idx])

    def __iter__(self):
        for cameraData in self.cameraSettings:
            self.dataHandler.setArguments(multiView=cameraData['nameIdx'])
            yield cameraData

# -----------------------------------------------------------------------------
# Helper methods
# -----------------------------------------------------------------------------

def update_camera(renderer, cameraData):
    camera = renderer.GetActiveCamera()
    camera.SetPosition(cameraData['position'])
    camera.SetFocalPoint(cameraData['focalPoint'])
    camera.SetViewUp(cameraData['viewUp'])

def create_spherical_camera(renderer, dataHandler, phiValues, thetaValues):
    camera = renderer.GetActiveCamera()
    return SphericalCamera(dataHandler, camera.GetFocalPoint(), camera.GetPosition(), camera.GetViewUp(), phiValues, thetaValues)

def create_cylindrical_camera(renderer, dataHandler, phiValues, translationValues):
    camera = renderer.GetActiveCamera()
    return CylindricalCamera(dataHandler, camera.GetFocalPoint(), camera.GetPosition(), camera.GetViewUp(), phiValues, translationValues)
