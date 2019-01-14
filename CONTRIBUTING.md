Contributing to VTK
===================

This page documents at a very high level how to contribute to VTK.
Please check our [developer instructions] for a more detailed guide to
developing and contributing to the project, and our [VTK Git README]
for additional information.

1.  The canonical VTK source is maintained on a GitLab instance
    at https://gitlab.kitware.com/vtk/vtk.<br/>
    [Create an account] there if you don't have one yet.<br/>
    **Note:** *If you're reading this document on GitHub,
    bear in mind that it is just a mirror, so pull requests here aren't merged.
    But don't worry, the workflow in GitLab is pretty similar
    and you can even sign up using your GitHub account.*

2.  [Fork VTK] into your user's namespace on GitLab.

3.  Follow the [download instructions] to create a
    local clone of the main VTK repository:

        $ git clone https://gitlab.kitware.com/vtk/vtk.git VTK
        $ cd VTK
    The main repository will be configured as your `origin` remote.

    For more information see: [Setup]

4.  Run the [developer setup script] to prepare your VTK work tree and
    create Git command aliases used below:

        $ ./Utilities/SetupForDevelopment.sh
    This will prompt for your GitLab user name and configure a remote
    called `gitlab` to refer to it.

    For more information see: [Setup]

5.  Edit files and create commits (repeat as needed):

        $ edit file1 file2 file3
        $ git add file1 file2 file3
        $ git commit

    For more information see: [Create a Topic]

6.  Push commits in your topic branch to your fork in GitLab:

        $ git gitlab-push

    For more information see: [Share a Topic]

7.  Visit your fork in GitLab, browse to the "**Merge Requests**" link on the
    left, and use the "**New Merge Request**" button in the upper right to
    create a Merge Request.

    For more information see: [Create a Merge Request]


VTK uses GitLab for code review and Buildbot to test proposed
patches before they are merged.

Our [Wiki] is used to document features, flesh out designs and host other
documentation. Our API is documented using [Doxygen] with updated
documentation generated nightly. We have several [Mailing Lists]
to coordinate development and to provide support.

[VTK Git README]: Documentation/dev/git/README.md
[developer instructions]: Documentation/dev/git/develop.md
[Create an account]: https://gitlab.kitware.com/users/sign_in
[Fork VTK]: https://gitlab.kitware.com/vtk/vtk/forks/new
[download instructions]: Documentation/dev/git/download.md#clone
[developer setup script]: /Utilities/SetupForDevelopment.sh
[Setup]: Documentation/dev/git/develop.md#Setup
[Create a Topic]: Documentation/dev/git/develop.md#create-a-topic
[Share a Topic]: Documentation/dev/git/develop.md#share-a-topic
[Create a Merge Request]: Documentation/dev/git/develop.md#create-a-merge-request

[Wiki]: http://www.vtk.org/Wiki/VTK
[Doxygen]: http://www.vtk.org/doc/nightly/html
[Mailing Lists]: http://www.vtk.org/VTK/help/mailing.html
