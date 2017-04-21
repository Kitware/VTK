import sys, re, hashlib, base64

py3 = sys.version_info.major > 2

arrayTypesMapping = [
  ' ', # VTK_VOID            0
  ' ', # VTK_BIT             1
  'b', # VTK_CHAR            2
  'B', # VTK_UNSIGNED_CHAR   3
  'h', # VTK_SHORT           4
  'H', # VTK_UNSIGNED_SHORT  5
  'i', # VTK_INT             6
  'I', # VTK_UNSIGNED_INT    7
  'l', # VTK_LONG            8
  'L', # VTK_UNSIGNED_LONG   9
  'f', # VTK_FLOAT          10
  'd', # VTK_DOUBLE         11
  'L', # VTK_ID_TYPE        12
]

javascriptMapping = {
    'b': 'Int8Array',
    'B': 'Uint8Array',
    'h': 'Int16Array',
    'H': 'Int16Array',
    'i': 'Int32Array',
    'I': 'Uint32Array',
    'l': 'Int32Array',
    'L': 'Uint32Array',
    'f': 'Float32Array',
    'd': 'Float64Array'
}

if py3:
    def iteritems(d, **kwargs):
        return iter(d.items(**kwargs))
    buffer = memoryview
    base64Encode = lambda x: base64.b64encode(x).decode('utf-8')
else:
    def iteritems(d, **kwargs):
        return d.iteritems(**kwargs)
    buffer = buffer
    base64Encode = lambda x: x.encode('base64')

def hashDataArray(dataArray):
    hashedBit = base64Encode(hashlib.md5(buffer(dataArray)).digest()).strip()
    md5sum = re.sub('==$', '', hashedBit)
    typeCode = arrayTypesMapping[dataArray.GetDataType()]
    return '%s_%d%s' % (md5sum, dataArray.GetSize(), typeCode)

def getJSArrayType(dataArray):
	return javascriptMapping[arrayTypesMapping[dataArray.GetDataType()]]
