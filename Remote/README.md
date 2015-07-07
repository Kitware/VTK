Remote Modules
==============

This directory is a place-holder for all remote modules distributed
outside VTK's main repository.  Remote modules share the same
directory structure as modules in the main repository.  They will be
picked up by the configuration system and entered into the build after
they are downloaded into this directory.

Modules can be easily downloaded and accessible to the community by
listing the module in the current directory.  For more information on
adding a new module to the list of new modules, see the page that
describes the policy and procedures for adding a new module:

[Adding Remote Modules to VTK](http://www.vtk.org/Wiki/VTK/Remote_Modules)

__NOTE__ that in each `<remote module name>.remote.cmake`, the first
argument of the function vtk_fetch_module() is the name of the remote
module, and it has to be consistent with the module name defined in
the correponding module.cmake.
