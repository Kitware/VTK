import sys

py3 = sys.version_info.major > 2

if py3:
    def iteritems(d, **kwargs):
        return iter(d.items(**kwargs))
    buffer = memoryview
else:
    def iteritems(d, **kwargs):
        return d.iteritems(**kwargs)
    buffer = buffer
