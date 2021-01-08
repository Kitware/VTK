#!/usr/bin/env python

# This tests vtk.test.ErrorObserver.vtkErrorObserver

from vtkmodules.vtkCommonCore import vtkArchiver, vtkCommand, vtkRandomPool
from vtkmodules.test import ErrorObserver, Testing


class TestErrorObserver(Testing.vtkTest):
    def testErrorCheck(self):
        observer = ErrorObserver.vtkErrorObserver()
        archiver = vtkArchiver()
        archiver.AddObserver(vtkCommand.ErrorEvent, observer)
        archiver.OpenArchive()
        observer.check_error('Please specify ArchiveName to use')

    def testWarningCheck(self):
        observer = ErrorObserver.vtkErrorObserver()
        pool = vtkRandomPool()
        pool.AddObserver(vtkCommand.WarningEvent, observer)
        pool.PopulateDataArray(None, 0., 1.)
        observer.check_warning('Bad request')


if __name__ == '__main__':
    Testing.main([(TestErrorObserver, 'test')])
