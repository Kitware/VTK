# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

"""
Versions for Python packages.

See L{Version}.
"""

from __future__ import division, absolute_import

import os
import sys
import warnings

#
# Compat functions
#

if sys.version_info < (3, 0):
    _PY3 = False
else:
    _PY3 = True
    unicode = str


def _nativeString(s):
    """
    Convert C{bytes} or C{unicode} to the native C{str} type, using ASCII
    encoding if conversion is necessary.

    @raise UnicodeError: The input string is not ASCII encodable/decodable.
    @raise TypeError: The input is neither C{bytes} nor C{unicode}.
    """
    if not isinstance(s, (bytes, unicode)):
        raise TypeError("%r is neither bytes nor unicode" % s)
    if _PY3:
        if isinstance(s, bytes):
            return s.decode("ascii")
        else:
            # Ensure we're limited to ASCII subset:
            s.encode("ascii")
    else:
        if isinstance(s, unicode):
            return s.encode("ascii")
        else:
            # Ensure we're limited to ASCII subset:
            s.decode("ascii")
    return s


try:
    _cmp = cmp
except NameError:
    def _cmp(a, b):
        """
        Compare two objects.

        Returns a negative number if C{a < b}, zero if they are equal, and a
        positive number if C{a > b}.
        """
        if a < b:
            return -1
        elif a == b:
            return 0
        else:
            return 1


def _comparable(klass):
    """
    Class decorator that ensures support for the special C{__cmp__} method.

    On Python 2 this does nothing.

    On Python 3, C{__eq__}, C{__lt__}, etc. methods are added to the class,
    relying on C{__cmp__} to implement their comparisons.
    """
    # On Python 2, __cmp__ will just work, so no need to add extra methods:
    if not _PY3:
        return klass

    def __eq__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c == 0

    def __ne__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c != 0

    def __lt__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c < 0

    def __le__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c <= 0

    def __gt__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c > 0

    def __ge__(self, other):
        c = self.__cmp__(other)
        if c is NotImplemented:
            return c
        return c >= 0

    klass.__lt__ = __lt__
    klass.__gt__ = __gt__
    klass.__le__ = __le__
    klass.__ge__ = __ge__
    klass.__eq__ = __eq__
    klass.__ne__ = __ne__
    return klass

#
# Versioning
#


@_comparable
class _inf(object):
    """
    An object that is bigger than all other objects.
    """
    def __cmp__(self, other):
        """
        @param other: Another object.
        @type other: any

        @return: 0 if other is inf, 1 otherwise.
        @rtype: C{int}
        """
        if other is _inf:
            return 0
        return 1

_inf = _inf()


class IncomparableVersions(TypeError):
    """
    Two versions could not be compared.
    """


@_comparable
class Version(object):
    """
    An encapsulation of a version for a project, with support for outputting
    PEP-440 compatible version strings.

    This class supports the standard major.minor.micro[rcN] scheme of
    versioning, with support for "local versions" which may include a SVN
    revision or Git SHA1 hash.
    """
    def __init__(self, package, major, minor, micro, release_candidate=None,
                 prerelease=None, dev=None):
        """
        @param package: Name of the package that this is a version of.
        @type package: C{str}
        @param major: The major version number.
        @type major: C{int} or C{str} (for the "NEXT" symbol)
        @param minor: The minor version number.
        @type minor: C{int}
        @param micro: The micro version number.
        @type micro: C{int}
        @param release_candidate: The release candidate number.
        @type release_candidate: C{int}
        @param prerelease: The prerelease number. (Deprecated)
        @type prerelease: C{int}
        @param dev: The development release number.
        @type dev: C{int}
        """
        if release_candidate and prerelease:
            raise ValueError("Please only return one of these.")
        elif prerelease and not release_candidate:
            release_candidate = prerelease
            warnings.warn(("Passing prerelease to incremental.Version was "
                           "deprecated in Incremental 16.9.0. Please pass "
                           "release_candidate instead."),
                          DeprecationWarning, stacklevel=2)

        if major == "NEXT":
            if minor or micro or release_candidate or dev:
                raise ValueError(("When using NEXT, all other values except "
                                  "Package must be 0."))

        self.package = package
        self.major = major
        self.minor = minor
        self.micro = micro
        self.release_candidate = release_candidate
        self.dev = dev

    @property
    def prerelease(self):
        warnings.warn(("Accessing incremental.Version.prerelease was "
                       "deprecated in Incremental 16.9.0. Use "
                       "Version.release_candidate instead."),
                      DeprecationWarning, stacklevel=2),
        return self.release_candidate

    def short(self):
        """
        Return a string in canonical short version format,
        <major>.<minor>.<micro>[+rSVNVer/+gitsha1].
        """
        s = self.base()
        gitver = self._getGitVersion()

        if not gitver:
            svnver = self._getSVNVersion()
            if svnver:
                s += '+r' + _nativeString(svnver)
        else:
            s += '+' + gitver
        return s

    def local(self):
        """
        Return a PEP440-compatible "local" representation of this L{Version}.

        This includes a SVN revision or Git commit SHA1 hash, if available.

        Examples:

          - 14.4.0+r1223
          - 1.2.3rc1+rb2e812003b5d5fcf08efd1dffed6afa98d44ac8c
          - 12.10.1
          - 3.4.8rc2
          - 11.93.0rc1dev3
        """
        return self.short()

    def public(self):
        """
        Return a PEP440-compatible "public" representation of this L{Version}.

        Examples:

          - 14.4.0
          - 1.2.3rc1
          - 14.2.1rc1dev9
          - 16.04.0dev0
        """
        return self.base()

    def base(self):
        """
        Like L{short}, but without the +rSVNVer or @gitsha1.
        """
        if self.major == "NEXT":
            return self.major

        if self.release_candidate is None:
            rc = ""
        else:
            rc = "rc%s" % (self.release_candidate,)

        if self.dev is None:
            dev = ""
        else:
            dev = "dev%s" % (self.dev,)

        return '%r.%d.%d%s%s' % (self.major,
                                 self.minor,
                                 self.micro,
                                 rc, dev)

    def __repr__(self):
        # Git repr
        gitver = self._formatGitVersion()
        if gitver:
            gitver = '  #' + gitver

        # SVN repr
        svnver = self._formatSVNVersion()
        if svnver:
            svnver = '  #' + svnver

        if self.release_candidate is None:
            release_candidate = ""
        else:
            release_candidate = ", release_candidate=%r" % (
                self.release_candidate,)

        if self.dev is None:
            dev = ""
        else:
            dev = ", dev=%r" % (self.dev,)

        return '%s(%r, %r, %d, %d%s%s)%s' % (
            self.__class__.__name__,
            self.package,
            self.major,
            self.minor,
            self.micro,
            release_candidate,
            dev,
            gitver or svnver)

    def __str__(self):
        return '[%s, version %s]' % (
            self.package,
            self.short())

    def __cmp__(self, other):
        """
        Compare two versions, considering major versions, minor versions, micro
        versions, then release candidates. Package names are case insensitive.

        A version with a release candidate is always less than a version
        without a release candidate. If both versions have release candidates,
        they will be included in the comparison.

        @param other: Another version.
        @type other: L{Version}

        @return: NotImplemented when the other object is not a Version, or one
            of -1, 0, or 1.

        @raise IncomparableVersions: when the package names of the versions
            differ.
        """
        if not isinstance(other, self.__class__):
            return NotImplemented
        if self.package.lower() != other.package.lower():
            raise IncomparableVersions("%r != %r"
                                       % (self.package, other.package))

        if self.major == "NEXT":
            major = _inf
        else:
            major = self.major

        if self.release_candidate is None:
            release_candidate = _inf
        else:
            release_candidate = self.release_candidate

        if self.dev is None:
            dev = _inf
        else:
            dev = self.dev

        if other.major == "NEXT":
            othermajor = _inf
        else:
            othermajor = other.major

        if other.release_candidate is None:
            otherrc = _inf
        else:
            otherrc = other.release_candidate

        if other.dev is None:
            otherdev = _inf
        else:
            otherdev = other.dev

        x = _cmp((major,
                  self.minor,
                  self.micro,
                  release_candidate,
                  dev),
                 (othermajor,
                  other.minor,
                  other.micro,
                  otherrc,
                  otherdev))
        return x

    def _parseGitDir(self, directory):

        headFile = os.path.abspath(os.path.join(directory, 'HEAD'))

        with open(headFile, "r") as f:
            headContent = f.read().strip()

        if headContent.startswith("ref: "):
            with open(os.path.abspath(
                    os.path.join(directory,
                                 headContent.split(" ")[1]))) as f:
                commit = f.read()
                return commit.strip()

        return headContent

    def _getGitVersion(self):
        """
        Given a package directory, walk up and find the git commit sha.
        """
        mod = sys.modules.get(self.package)
        if mod:
            basepath = os.path.dirname(mod.__file__)

            upOne = os.path.abspath(os.path.join(basepath, '..'))

            if ".git" in os.listdir(upOne):
                return self._parseGitDir(os.path.join(upOne, '.git'))

            while True:

                upOneMore = os.path.abspath(os.path.join(upOne, '..'))

                if upOneMore == upOne:
                    return None

                if ".git" in os.listdir(upOneMore):
                    return self._parseGitDir(os.path.join(upOneMore, '.git'))

                upOne = upOneMore

    def _parseSVNEntries_4(self, entriesFile):
        """
        Given a readable file object which represents a .svn/entries file in
        format version 4, return the revision as a string.  We do this by
        reading first XML element in the document that has a 'revision'
        attribute.
        """
        from xml.dom.minidom import parse
        doc = parse(entriesFile).documentElement
        for node in doc.childNodes:
            if hasattr(node, 'getAttribute'):
                rev = node.getAttribute('revision')
                if rev is not None:
                    return rev.encode('ascii')

    def _parseSVNEntries_8(self, entriesFile):
        """
        Given a readable file object which represents a .svn/entries file in
        format version 8, return the revision as a string.
        """
        entriesFile.readline()
        entriesFile.readline()
        entriesFile.readline()
        return entriesFile.readline().strip()

    # Add handlers for version 9 and 10 formats, which are the same as
    # version 8 as far as revision information is concerned.
    _parseSVNEntries_9 = _parseSVNEntries_8
    _parseSVNEntriesTenPlus = _parseSVNEntries_8

    def _getSVNVersion(self):
        """
        Figure out the SVN revision number based on the existence of
        <package>/.svn/entries, and its contents. This requires discovering the
        format version from the 'format' file and parsing the entries file
        accordingly.

        @return: None or string containing SVN Revision number.
        """
        mod = sys.modules.get(self.package)
        if mod:
            svn = os.path.join(os.path.dirname(mod.__file__), '.svn')
            if not os.path.exists(svn):
                # It's not an svn working copy
                return None

            formatFile = os.path.join(svn, 'format')
            if os.path.exists(formatFile):
                # It looks like a less-than-version-10 working copy.
                with open(formatFile, 'rb') as fObj:
                    format = fObj.read().strip()
                parser = getattr(self,
                                 '_parseSVNEntries_' + format.decode('ascii'),
                                 None)
            else:
                # It looks like a version-10-or-greater working copy, which
                # has version information in the entries file.
                parser = self._parseSVNEntriesTenPlus

            if parser is None:
                return b'Unknown'

            entriesFile = os.path.join(svn, 'entries')
            entries = open(entriesFile, 'rb')
            try:
                try:
                    return parser(entries)
                finally:
                    entries.close()
            except:
                return b'Unknown'

    def _formatSVNVersion(self):
        ver = self._getSVNVersion()
        if ver is None:
            return ''
        return ' (SVN r%s)' % (ver,)

    def _formatGitVersion(self):
        ver = self._getGitVersion()
        if ver is None:
            return ''
        return ' (Git %s)' % (ver,)


def getVersionString(version):
    """
    Get a friendly string for the given version object.

    @param version: A L{Version} object.
    @return: A string containing the package and short version number.
    """
    result = '%s %s' % (version.package, version.short())
    return result


def _get_version(dist, keyword, value):
    """
    Get the version from the package listed in the Distribution.
    """
    if not value:
        return

    from distutils.command import build_py

    sp_command = build_py.build_py(dist)
    sp_command.finalize_options()

    for item in sp_command.find_all_modules():
        if item[1] == "_version":
            version_file = {}

            with open(item[2]) as f:
                exec(f.read(), version_file)

            dist.metadata.version = version_file["__version__"].public()
            return None

    raise Exception("No _version.py found.")


from ._version import __version__ # noqa


__all__ = ["__version__", "Version", "getVersionString"]
