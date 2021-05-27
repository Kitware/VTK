import hashlib, base64

arrayTypesMapping = [
    " ",  # VTK_VOID            0
    " ",  # VTK_BIT             1
    "b",  # VTK_CHAR            2
    "B",  # VTK_UNSIGNED_CHAR   3
    "h",  # VTK_SHORT           4
    "H",  # VTK_UNSIGNED_SHORT  5
    "i",  # VTK_INT             6
    "I",  # VTK_UNSIGNED_INT    7
    "l",  # VTK_LONG            8
    "L",  # VTK_UNSIGNED_LONG   9
    "f",  # VTK_FLOAT          10
    "d",  # VTK_DOUBLE         11
    "L",  # VTK_ID_TYPE        12
    " ",  # unspecified        13
    " ",  # unspecified        14
    "b",  # signed_char        15
]

javascriptMapping = {
    "b": "Int8Array",
    "B": "Uint8Array",
    "h": "Int16Array",
    "H": "Int16Array",
    "i": "Int32Array",
    "I": "Uint32Array",
    "l": "Int32Array",
    "L": "Uint32Array",
    "f": "Float32Array",
    "d": "Float64Array",
}


def iteritems(d, **kwargs):
    return iter(d.items(**kwargs))


def base64Encode(x):
    return base64.b64encode(x).decode("utf-8")


def hashDataArray(dataArray):
    hashedBit = hashlib.md5(memoryview(dataArray)).hexdigest()
    typeCode = arrayTypesMapping[dataArray.GetDataType()]
    return "%s_%d%s" % (hashedBit, dataArray.GetSize(), typeCode)


def getJSArrayType(dataArray):
    return javascriptMapping[arrayTypesMapping[dataArray.GetDataType()]]


def getReferenceId(ref):
    if ref:
        try:
            return ref.__this__[1:17]
        except:
            idStr = str(ref)[-12:-1]
            # print('====> fallback ID %s for %s' % (idStr, ref))
            return idStr
    return "0x0"
