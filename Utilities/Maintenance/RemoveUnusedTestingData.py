#!/usr/bin/env python3
"""Find (and optionally delete) unused ExternalData content links in VTK.

VTK stores test input data and test baseline images out of tree; the repository
only tracks ``<name>.sha512`` content links that ``ExternalData`` resolves at
configure time. Over time, links accumulate for data that no test references
any more. This script scans the tracked sources for references to each content
link and reports the ones that nothing mentions.

Usage::

    Utilities/Maintenance/RemoveUnusedTestingData.py            # dry run
    Utilities/Maintenance/RemoveUnusedTestingData.py --delete   # git rm the links
    Utilities/Maintenance/RemoveUnusedTestingData.py --list     # paths only

A dry run exits non-zero when it reports anything, so CI can use it to keep new
orphans from accumulating.

Deleting a content link discards the only reference the repository keeps to the
uploaded object, so review the report before passing ``--delete``.
"""

import argparse
import os
import re
import subprocess
import sys

# Directories whose contents are data links rather than references to them.
DATA_DIR_RE = re.compile(r'(^|/)(Testing/Data|Testing/Baseline|Data/Baseline)(/|$)')

# Extensions of tracked files that can never mention a data file.
BINARY_SUFFIXES = (
    '.png', '.jpg', '.jpeg', '.gif', '.ico', '.pdf', '.zip', '.gz', '.bz2',
    '.xz', '.tgz', '.jar', '.so', '.dll', '.dylib', '.a', '.lib', '.o',
    '.ttf', '.otf', '.woff', '.woff2', '.mp4', '.ogv', '.wav',
)

# Anything that could plausibly be a file name fragment in source code.
TOKEN_RE = re.compile(rb'[A-Za-z0-9_.+-]+')

# Baselines alternate as ``Foo_1.png``, ``Foo_2.png``, ...
BASELINE_ALT_RE = re.compile(r'_\d+$')

# Directory names that only group data links but never identify one.
GENERIC_DIRS = frozenset(('Baseline', 'Input', 'Data', 'Testing'))

# Shortest prefix considered a meaningful match for a run-time assembled path.
MIN_PREFIX = 6

# Shortest stem two files must share to be considered companions of each other.
MIN_COMPANION_STEM = 3

# Trailing characters of a name that a variable expansion was spliced onto.
CONCAT_ENDINGS = '_-.'

# ``vtk_add_test_*`` derives the baseline name from the test source file name,
# so a mention of ``TestFoo.cxx`` is a reference to ``TestFoo.png``.
TEST_SOURCE_SUFFIXES = ('.cxx', '.cpp', '.cc', '.py', '.java', '.js', '.mjs', '.cs')


def run(args, cwd):
    return subprocess.run(
        args, cwd=cwd, check=True, stdout=subprocess.PIPE).stdout.decode('utf-8')


def repo_root():
    return run(['git', 'rev-parse', '--show-toplevel'], os.getcwd()).strip()


def tracked_files(root):
    out = run(['git', 'ls-files', '-z'], root)
    return [p for p in out.split('\0') if p]


def is_data_link(path):
    return path.endswith('.sha512') and DATA_DIR_RE.search(os.path.dirname(path))


def link_scope(link):
    """The module directory owning ``link``, or ``''`` for shared test data."""
    rel = os.path.dirname(link)
    marker = DATA_DIR_RE.search(rel)
    return rel[:marker.start()] if marker else ''


def file_scope(path, scopes):
    """The longest scope in ``scopes`` containing ``path``, or ``''``."""
    best = ''
    for scope in scopes:
        if len(scope) > len(best) and path.startswith(scope + '/'):
            best = scope
    return best


def add_tokens(tokens, concat, name):
    tokens.add(name)
    # Tokens are often whole paths; index each component too.
    if '/' in name:
        tokens.update(part for part in name.split('/') if part)
    if name.endswith(TEST_SOURCE_SUFFIXES):
        concat.add(os.path.splitext(os.path.basename(name))[0])
    elif name[-1] in CONCAT_ENDINGS:
        # A name cut short by a variable expansion: "TestLASReader_${postfix}".
        concat.add(name)


def build_token_index(root, paths, scopes):
    """Collect the filename-ish tokens of each source, grouped by scope.

    Returns a mapping of scope to the ``(tokens, concat)`` pair contributed by
    the files under it, where ``concat`` holds the subset of names that a
    generated name may have been built on top of. The ``''`` entry holds the
    shared files that are not owned by any module.
    """
    by_scope = {scope: (set(), set()) for scope in scopes}
    by_scope[''] = (set(), set())
    for path in paths:
        tokens, concat = by_scope[file_scope(path, scopes)]
        add_tokens(tokens, concat, path)
        try:
            with open(os.path.join(root, path), 'rb') as handle:
                blob = handle.read()
        except OSError:
            continue
        if b'\0' in blob[:8192]:
            continue
        for match in TOKEN_RE.finditer(blob):
            try:
                add_tokens(tokens, concat, match.group(0).decode('utf-8'))
            except UnicodeDecodeError:
                continue
    return by_scope


class TokenIndex(object):
    """Exact and shared-prefix lookup over the tokens found in the sources."""

    def __init__(self, tokens, concat):
        self._tokens = tokens
        self._concat = concat

    def classify(self, names, strict_prefix=False):
        """Return ``'used'``, ``'uncertain'`` or ``'unused'`` for a name set."""
        prefixes = self._concat if strict_prefix else self._tokens
        uncertain = False
        for name in names:
            if name in self._tokens or name in self._concat:
                return 'used'
            if len(name) < MIN_PREFIX or uncertain:
                continue
            # A referenced name is a prefix of this one: "Foo_MPI_" + rank.
            for end in range(len(name) - 1, MIN_PREFIX - 1, -1):
                if name[:end] in prefixes:
                    uncertain = True
                    break
        return 'uncertain' if uncertain else 'unused'


def is_baseline(link):
    return 'Baseline' in os.path.dirname(link).split('/')


def link_stems(link):
    """The file name of ``link`` with extensions progressively stripped off."""
    name = os.path.basename(link)[: -len('.sha512')]
    stems = {name}
    stem = name
    while True:
        stem, ext = os.path.splitext(stem)
        if not ext or not stem:
            break
        stems.add(stem)
    return stems


def candidate_names(link, strict=False):
    """Names that, if referenced, keep ``link`` alive.

    ``strict`` drops the names a file merely happens to share with unrelated
    data, its extension-less stems, and the directories above it, keeping
    only the ones that can identify it from another module.
    """
    name = os.path.basename(link)[: -len('.sha512')]
    rel = os.path.dirname(link)
    names = {name} if strict else link_stems(link)

    marker = DATA_DIR_RE.search(rel)
    if marker and not strict:
        tail = rel[marker.end():]
        names.update(part for part in tail.split('/')
                     if part and part not in GENERIC_DIRS)

    if is_baseline(link):
        base = BASELINE_ALT_RE.sub('', os.path.splitext(name)[0])
        names.add(base)
        if '-' in base:
            names.add(base.split('-')[0])

    return {n for n in names if n}


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--delete', action='store_true',
                        help='git rm the unused content links (default: dry run)')
    parser.add_argument('--include-uncertain', action='store_true',
                        help='also report/remove links matched only by a shared '
                             'name prefix or by a companion of the same stem')
    parser.add_argument('--list', action='store_true',
                        help='print only the reported paths, one per line')
    parser.add_argument('--path', action='append', default=[],
                        help='limit the scan to content links under this prefix')
    args = parser.parse_args()

    root = repo_root()
    paths = tracked_files(root)

    all_links = [p for p in paths if is_data_link(p)]
    links = all_links
    if args.path:
        links = [p for p in links if any(p.startswith(prefix) for prefix in args.path)]

    searchable = [
        p for p in paths
        if not p.endswith('.sha512') and not p.lower().endswith(BINARY_SUFFIXES)
    ]

    # Scopes come from every link, not just the selected ones, so that --path
    # does not change how the remaining sources are partitioned.
    scopes = {scope for scope in map(link_scope, all_links) if scope}
    by_scope = build_token_index(root, searchable, scopes)

    # Shared test data may be referenced from any module, so it is matched
    # against every token; a module's own data only against its own sources
    # plus the shared files that belong to no module.
    shared_tokens, shared_concat = by_scope['']
    everything = (set(), set())
    for tokens, concat in by_scope.values():
        everything[0].update(tokens)
        everything[1].update(concat)
    indexes = {'': TokenIndex(*everything)}
    for scope in scopes:
        tokens, concat = by_scope[scope]
        indexes[scope] = TokenIndex(tokens | shared_tokens, concat | shared_concat)

    rest = indexes['']
    unused = []
    uncertain = []
    for link in links:
        scope = link_scope(link)
        strict_prefix = is_baseline(link)
        verdict = indexes[scope].classify(candidate_names(link), strict_prefix)
        if scope and verdict != 'used':
            elsewhere = rest.classify(
                candidate_names(link, strict=True), strict_prefix)
            if elsewhere != 'unused':
                verdict = elsewhere
        if verdict == 'unused':
            unused.append(link)
        elif verdict == 'uncertain':
            uncertain.append(link)

    # A companion file that the reader opens by itself is never named in the
    # build files; keep it whenever its sibling of the same stem is used.
    reported = set(unused) | set(uncertain)
    used_stems = {}
    for link in links:
        if link not in reported:
            stems = used_stems.setdefault(os.path.dirname(link), set())
            stems.update(stem for stem in link_stems(link)
                         if len(stem) >= MIN_COMPANION_STEM)
    companions = {link for link in unused
                  if link_stems(link) & used_stems.get(os.path.dirname(link), set())}
    if companions:
        unused = [link for link in unused if link not in companions]
        uncertain.extend(companions)

    unused.sort()
    uncertain.sort()
    doomed = sorted(unused + uncertain) if args.include_uncertain else unused

    if args.list:
        for link in doomed:
            print(link)
    else:
        for link in unused:
            print('unused:    %s' % link)
        if args.include_uncertain:
            for link in uncertain:
                print('uncertain: %s' % link)
        print('\n%d of %d content links are unreferenced'
              ' (%d more match only weakly).'
              % (len(unused), len(links), len(uncertain)), file=sys.stderr)

    if args.delete and doomed:
        for start in range(0, len(doomed), 500):
            subprocess.run(['git', 'rm', '-q', '--'] + doomed[start:start + 500],
                           cwd=root, check=True)
        print('Deleted %d content links.' % len(doomed), file=sys.stderr)

    return 1 if len(doomed) > 0 else 0


if __name__ == '__main__':
    sys.exit(main())
