## Updated and Consolidated Documentation

### Overview

The VTK (Visualization Toolkit) documentation has undergone a major update and
consolidation to enhance its usefulness for developers. The
`/Documentation/docs` directory now contains the contents and configuration for
the Sphinx-based website, published on the ReadTheDocs platform at
https://docs.vtk.org. This consolidates all existing documentation for VTK,
including the newly added list of supported data formats, API of all VTK public
CMake modules, the VTK formats specification (previously part of vtk-examples),
and the general information about the VTK project. Software process and
conventions documentation has also been moved from docs.google.com to the new
website.

In addition to the documentation website, two new resources have been
introduced: the VTK book, which hosts the markdown version of the VTK book at
https://book.vtk.org and VTK examples at https://examples.vtk.org, which
contain many examples with redirects put in place to ensure the previous URL
remains functional. Many other updates to the documentation have also been
made, including improved documentation structure, removal of obsolete
documents, and addition of imported third-party projects to the developer
guide. VTK documentation now follows a versioning system and is actively
maintained alongside the code.

Contributions and feedback are welcome for all three websites to ensure that
the VTK documentation remains up-to-date.

### Next steps

The next steps for the VTK documentation project include setting up a
versioning system for docs, doxygen, and the book. Work is also underway to
include a description of each `Modules` (as well as a `README.md` file) in
`docs.vtk.org`. And there are plans to explore the possibility of using merge
request previews for the documentation so that contributors don't have to
compile it themselves.

For `examples.vtk.org`, the plan is to consolidate examples from VTK and
`vtk-examples`.

Pages on the `mediawiki` site will be marked as deprecated, and a link to
`docs.vtk.org` will be included.

These efforts will help ensure that VTK documentation remains user-friendly and
accessible to all developers.
