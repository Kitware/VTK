"""
This script will attempt to install a 'vtkpython.pth' file so that python
can automatically find this directory and use the modules inside it.

It will also attempt to copy the vtkTkRenderWidget to a directory from
which tcl/tk can find it.

This script is meant primarily for use under Windows.  
Under UNIX, you should use 'make install' instead.
"""

import sys, os, shutil 

if os.name in ['nt','dos']:
    modulepath = sys.prefix
else:
    modulepath = os.path.join(sys.prefix,'lib','python'+sys.version[0:3],
                              'site-packages')

print 'creating',os.path.join(modulepath,'vtkpython.pth')

file = open(os.path.join(modulepath,'vtkpython.pth'),'w+')
file.write(os.getcwd())
file.close()

if os.name in ['nt']:
    libpath = os.path.join(os.environ['SYSTEMROOT'],'SYSTEM32')
    extension = '.dll'
elif os.name in ['dos']:
    libpath = os.path.join(os.environ['SYSTEMROOT'],'SYSTEM')
    extension = '.dll'
else: 
    # we assume that wish is installed with the same prefix
    # as python 
    libpath = os.path.join(sys.prefix,'lib')
    extension = ''

print 'copying vtkTkRenderWidget%s to %s' % (extension,libpath) 

shutil.copy('vtkTkRenderWidget'+extension,libpath)
try:
    shutil.copy('vtkTkImageWindowWidget'+extension,libpath)
    shutil.copy('vtkTkImageViewerWidget'+extension,libpath)
except:
    pass
