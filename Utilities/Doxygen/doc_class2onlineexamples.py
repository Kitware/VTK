#!/usr/bin/env python3

from argparse import ArgumentParser
from dataclasses import dataclass
from os.path import basename
from urllib.error import HTTPError
from urllib.request import urlretrieve
import logging
import pathlib
import re
import time
import itertools


# -------------------------------------------------------------------------------------------------
@dataclass(frozen=True)
class refs:
    raw_base_url: str = "https://raw.githubusercontent.com/sankhesh/vtk-examples/"
    base_url: str = "https://kitware.github.io/vtk-examples/"
    site_url: str = base_url + "site/"
    hash: str = "104a1aed92827ecf58b609af4e0852a46c7ea73c"
    selectExamples: str = "src/Python/Utilities/SelectExamples.py"
    img_base: str =\
        "https://raw.githubusercontent.com/Kitware/vtk-examples/gh-pages/src/Testing/Baseline/"


# -------------------------------------------------------------------------------------------------
def parse_arguments():
    import argparse

    description = 'Cross-reference vtk-examples with classes'
    epilog = '''
This script generates a cross-reference between each class to a specific number of
examples that use that class. The examples are hyperlinked to the vtk-examples GitHub site. The
script displays the image from the example page in the doxygen documentation.
'''

    parser = ArgumentParser(description=description, epilog=epilog,
                            formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-d', '--dir', type=pathlib.Path,
                        help='Directory containing the vtk classes')
    parser.add_argument('-t', '--tmpdir', type=pathlib.Path,
                        help='temp directory')
    parser.add_argument('-l', '--label', default="Online Examples",
                        help="Label to use in the documentation")
    parser.add_argument('-n', '--number', type=int, default=10,
                        help='Maximum number of examples to show')
    parser.add_argument('-v', '--log-level',
                        type=lambda x: getattr(logging, x),
                        default=logging.INFO, help="Configure the logging level", )
    args = parser.parse_args()
    return args


# -------------------------------------------------------------------------------------------------
def download_file(dl_path, dl_url, overwrite=False):
    """
    Use the URL to get a file.

    :param dl_path: The path to download the file to.
    :param dl_url: The URL of the file.
    :param overwrite: If true, do a download even if the file exists.
    :return: The path to the file as a pathlib Path.
    """
    file_name = dl_url.split('/')[-1]
    # Create necessary sub-directories in the dl_path
    # (if they don't exist).
    pathlib.Path(dl_path).mkdir(parents=True, exist_ok=True)
    # Download if it doesn't exist in the directory overriding if overwrite is True.
    path = pathlib.Path(dl_path, file_name)
    if not path.is_file() or overwrite:
        try:
            urlretrieve(dl_url, path)
        except HTTPError as e:
            raise RuntimeError(f'Failed to download {dl_url}. {e.reason}')
    return path


# -------------------------------------------------------------------------------------------------
def preamble_for_label(label):
    """
    Return the preamble for the documentation block for the given label.

    :param label: The label to use as the paragraph title.
    :return: The string that should be preamble of the new section.
    """
    preamble = str("\n" + " "*4 + "@par " + label + ":\n\n")
    preamble += str(" "*4 + "@htmlonly\n\n")
    return preamble


# -------------------------------------------------------------------------------------------------
def conclusion_block():
    """
    Return the conclusion for the documentation block.

    :return: The string representing the conclusion of the documentation block added by this script.
    """
    return str("\n" + " "*4 + "@endhtmlonly\n")


# -------------------------------------------------------------------------------------------------
def docblock_from_example(example_url, label):
    """
    Return the documentation block to be inserted in the header file for the given example.

    :param example_url: The URL for the example page.
    :param label: The label to use as the header.
    :return: The documentation string to be inserted.
    """
    doc_block = "\n"

    e = decompose_exampleurl(example_url)
    if e is None:
        logging.error("Could not decompose example url to its language, module and name. Skipping..",
                      example_url)
        return None
    doc_block += " "*6 + "<a href=\"" + example_url + "\">\n"
    example_img_url = imageurl_from_exampleurl(e[0], e[1], e[2])
    if example_img_url is not None:
        # Hover feedback container
        doc_block += " "*8 + "<div class=\"examplegrid_container\">\n"
        # Image url
        doc_block += " "*10 + "<img src=\"" + example_img_url + "\">\n"
        # Overlay
        doc_block += " "*10 + "<div class=\"examplegrid_overlay\">\n"
        # Overlay text
        doc_block += " "*12 + \
            "<div class=\"examplegrid_text\">" + e[2] + "</div>\n"
        # Close overlay
        doc_block += " "*10 + "</div>\n"
        # Close hover feedback container
        doc_block += " "*8 + "</div>\n"
    doc_block += " "*6 + "</a>\n"
    return doc_block


# -------------------------------------------------------------------------------------------------
def decompose_exampleurl(example_url):
    """
    Given an example url, decompose it to find the language, module and name.

    :param: example_url: The URL for the example page.
    :return: lang: The language that the example is written in.
    :return: module: The VTK module that the example belongs to.
    :return: name: The name of the example.
    """
    example_name_matcher_str = r'^{u}(Cxx|CSharp|Python)/(.*)/(.*)$'.format(
        u=refs.site_url)
    example_name_matcher = re.compile(example_name_matcher_str)
    example_name_match = example_name_matcher.match(example_url)
    if example_name_match is None:
        logging.error("Could not parse example name from url:",
                      example_url, example_name_matcher_str)
        return None
    if len(example_name_match.groups()) != 3:
        logging.error("Example regex matcher could not find sufficient matches:",
                      example_name_match.groups())
        return None
    return [i for i in example_name_match.groups()]


# -------------------------------------------------------------------------------------------------
def imageurl_from_exampleurl(lang, module, name):
    """
    Return a valid url for the image on the example page for given language, module and  example
    name.

    :param example_url: The URL for the example page.
    :return: The URL for the image or None if not found.
    """

    example_img_url = refs.img_base + \
        str("/") + lang + "/" + module + str("/Test") + name + str(".png")
    return example_img_url


# -------------------------------------------------------------------------------------------------
def main(indir, tmpdir, label, max_num):
    """
    The main function of the script.

    This method downloads the selectExamples script, finds and parses headers in the doxygen source,
    and adds the necessary documenation blocks to the headers.
    """

    intermediate_time = time.time()
    logging.info("Downloading and importing selectExamples...")
    # download the select examples script from the vtk-examples repository
    selUrl = refs.raw_base_url + "/" + refs.hash + "/" + refs.selectExamples
    path = download_file(tmpdir, selUrl, overwrite=False)

    # import the select examples script
    import importlib.util
    spec = importlib.util.spec_from_file_location("SelectExamples", path)
    selectExamples = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(selectExamples)
    logging.debug("Downloaded and imported selectExamples script in {t:.2f}s."
                  .format(t=time.time() - intermediate_time))
    logging.info("Collecting files...")
    intermediate_time = time.time()
    # iterate over all header files
    headerlist = pathlib.Path(indir).rglob('*.h')
    headerlist, headerlist_backup = itertools.tee(headerlist)
    num_headers = sum(1 for _ in headerlist_backup)
    logging.debug("Collected {i} files in {t:.2f}s."
                  .format(i=num_headers, t=(time.time() - intermediate_time)))
    # eliminate_matcher
    eliminate_matcher = re.compile(r'^vtkCommand.h$')
    # class name matcher
    class_matcher = re.compile(r'^.*@class\s+(.*)')
    # end of documentation block matcher
    enddoc_matcher = re.compile(r'^.*\*\/')
    # label for the new documentation par
    label_matcher_str = r'^\s*\@par\s+{l}:\s*$'.format(l=label)
    label_matcher = re.compile(label_matcher_str)
    logging.debug("Iterating over collected files...")
    intermediate_time = time.time()
    for headerfile in headerlist:
        if not headerfile.name.startswith('vtk'):
            logging.debug("Ignoring non-vtk header {f}"
                          .format(f=headerfile.name))
            continue
        # eliminate matcher
        if eliminate_matcher.match(headerfile.name) is not None:
            logging.warning("Ignoring header {f}".format(f=headerfile.name))
            continue
        classname = None
        classlineno = -1
        enddoclineno = -1
        labellineno = -1
        header_time = time.time()
        logging.debug("Parsing header {f}".format(f=headerfile.name))
        with open(headerfile, "r") as f:
            num = 0
            for line in f:
                if classlineno < 0:
                    class_match = class_matcher.match(line)
                    if class_match is not None:
                        # found the @class block
                        classlineno = num
                        classname = class_match.group(1)
                        logging.debug("Class name found @{l}: '{c}'"
                                      .format(l=classlineno, c=classname))
                elif enddoclineno < 0:
                    # Haven't found the end of the documentation block yet.
                    enddoc_match = enddoc_matcher.match(line)
                    # Is this the end of the documentation block?
                    if enddoc_match is None:
                        # This is not the end doc line.
                        # So, check if the xref is not already there
                        label_match = label_matcher.match(line)
                        if label_match is not None:
                            # The xref is already there.
                            labellineno = num
                            logging.debug("Label found @{lc}: '{l}'"
                                          .format(lc=labellineno, l=line.strip()))
                    else:
                        # end of doc found. Record and abort
                        enddoclineno = num
                        logging.debug("End of documentation block found @{lc}: '{l}'"
                                      .format(lc=enddoclineno, l=line.strip()))
                        break
                num += 1

            # If we reach here and couldn't find either classname, enddocline number, etc.
            # This file might not have the standard documentation
            if classname is None or classlineno < 0 or enddoclineno < 0:
                logging.error("Could not process documentation block in file {f}"
                              .format(f=headerfile.name))
                continue
            elif labellineno > -1:
                # This file already contains the examples block
                logging.warning("Header {f} already contains the '@par {l}:' documentation block"
                                .format(f=headerfile.name, l=label))
                continue
        logging.debug("Parsed header {f} in {t:.2f}s"
                      .format(f=headerfile.name, t=(time.time() - header_time)))
        logging.debug("Collected info for {f}: classname={c}, classlineno={lc},"
                      "labellineno={ll}, enddoclineno={le}"
                      .format(f=headerfile.name, c=classname,
                              lc=classlineno, ll=labellineno, le=enddoclineno))
        logging.debug("Fetching examples for class '{c}'".format(c=classname))
        t, examples = selectExamples.get_examples(
            selectExamples.get_crossref_dict(tmpdir), classname,
            'Cxx', all_values=False, number=max_num)
        if not examples or examples is None:
            logging.debug(
                "No examples found for class: {c}".format(c=classname))
            continue
        logging.debug("Fetched {n} examples for class '{c}'"
                      .format(c=classname, n=len(examples)))
        doxblock = preamble_for_label(label)
        # Add a new grid div
        doxblock += " "*4 + "<div class=\"examplegrid\">\n"
        for e in examples:
            d = docblock_from_example(e, label)
            doxblock += d + "\n" if d is not None else ""
        # Close the existing grid div
        doxblock += " "*4 + "</div>\n"
        doxblock += conclusion_block()

        # Finally, write the new documentation block to the header file
        with open(headerfile, "r") as f:
            contents = f.readlines()
        contents.insert(enddoclineno, doxblock)
        with open(headerfile, "w") as f:
            contents = "".join(contents)
            f.write(contents)
        logging.debug("Added documentation block for found examples")
    logging.info("Done processing {n} collected files in {t:.2f}s."
                 .format(n=num_headers, t=(time.time() - intermediate_time)))


# -------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    args = parse_arguments()
    logging.basicConfig(level=args.log_level)

    prog_name = basename(__file__)
    logging.info(prog_name)

    start_time = time.time()
    main(args.dir, args.tmpdir, args.label, args.number)
    logging.info("Finished in {t:.2f}s.".format(t=(time.time() - start_time)))
