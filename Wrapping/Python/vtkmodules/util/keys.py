"""
Utility module to make it easier to create new keys.
"""
from vtkmodules.vtkCommonCore import vtkInformationDataObjectKey as DataaObjectKey
from vtkmodules.vtkCommonCore import vtkInformationDoubleKey as DoubleKey
from vtkmodules.vtkCommonCore import vtkInformationDoubleVectorKey as DoubleVectorKey
from vtkmodules.vtkCommonCore import vtkInformationIdTypeKey as IdTypeKey
from vtkmodules.vtkCommonCore import vtkInformationInformationKey as InformationKey
from vtkmodules.vtkCommonCore import vtkInformationInformationVectorKey as InformationVectorKey
from vtkmodules.vtkCommonCore import vtkInformationIntegerKey as IntegerKey
from vtkmodules.vtkCommonCore import vtkInformationIntegerVectorKey as IntegerVectorKey
from vtkmodules.vtkCommonCore import vtkInformationKeyVectorKey as KeyVectorKey
from vtkmodules.vtkCommonCore import vtkInformationObjectBaseKey as ObjectBaseKey
from vtkmodules.vtkCommonCore import vtkInformationObjectBaseVectorKey as ObjectBaseVectorKey
from vtkmodules.vtkCommonCore import vtkInformationRequestKey as RequestKey
from vtkmodules.vtkCommonCore import vtkInformationStringKey as StringKey
from vtkmodules.vtkCommonCore import vtkInformationStringVectorKey as StringVectorKey
from vtkmodules.vtkCommonCore import vtkInformationUnsignedLongKey as UnsignedLongKey
from vtkmodules.vtkCommonCore import vtkInformationVariantKey as VariantKey
from vtkmodules.vtkCommonCore import vtkInformationVariantVectorKey as VariantVectorKey
from vtkmodules.vtkCommonExecutionModel import vtkInformationDataObjectMetaDataKey as DataObjectMetaDataKey
from vtkmodules.vtkCommonExecutionModel import vtkInformationExecutivePortKey as ExecutivePortKey
from vtkmodules.vtkCommonExecutionModel import vtkInformationExecutivePortVectorKey as ExecutivePortVectorKey
from vtkmodules.vtkCommonExecutionModel import vtkInformationIntegerRequestKey as IntegerRequestKey

def MakeKey(key_type, name, location, *args):
    """Given a key type, make a new key of given name
    and location."""
    return key_type.MakeKey(name, location, *args)
