// VTK doxygen layout tweaks.
//
// Turns the (often large) inheritance and collaboration diagrams on each
// class/struct reference page into self-contained collapsible sections that
// are collapsed by default, keeping the machine-generated graphs out of the
// way until requested.
//
// The other page-layout changes -- hiding the redundant one-line brief
// description (with its "More..." link) and hoisting the Detailed Description
// above the member tables -- are handled natively by the vendored
// DoxygenLayout.xml (LAYOUT_FILE), so no DOM reordering is needed here.
//
// The collapsing is done in JS because doxygen (as of 1.13) does not make the
// graph .dynheader/.dyncontent blocks collapsible on its own -- they carry no
// toggle -- and the layout file has no attribute to express it. The control
// added below does not depend on doxygen's dynsections.js.

(function () {
  'use strict';

  function onReady(fn) {
    if (document.readyState !== 'loading') {
      fn();
    } else {
      document.addEventListener('DOMContentLoaded', fn);
    }
  }

  var CARET_COLLAPSED = '▶'; // right-pointing triangle
  var CARET_EXPANDED = '▼'; // down-pointing triangle

  // Turn a single .dynheader/.dyncontent pair into a collapsible section that
  // starts collapsed.
  function wireCollapsible(header, content) {
    if (header.getAttribute('data-vtk-collapsible')) {
      return;
    }
    header.setAttribute('data-vtk-collapsible', '1');

    var caret = document.createElement('span');
    caret.className = 'vtk-collapse-caret';
    caret.textContent = CARET_COLLAPSED;
    caret.style.marginRight = '0.4em';
    caret.style.display = 'inline-block';
    caret.style.fontSize = '0.8em';
    header.insertBefore(caret, header.firstChild);

    header.style.cursor = 'pointer';
    header.setAttribute('role', 'button');
    header.setAttribute('tabindex', '0');

    var collapsed = true;
    content.style.display = 'none';

    function toggle() {
      collapsed = !collapsed;
      content.style.display = collapsed ? 'none' : '';
      caret.textContent = collapsed ? CARET_COLLAPSED : CARET_EXPANDED;
    }

    header.addEventListener('click', toggle);
    header.addEventListener('keydown', function (event) {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        toggle();
      }
    });
  }

  // Make the inheritance and collaboration diagrams collapsible (collapsed by
  // default).
  function collapseDiagrams(contents) {
    var headers = contents.querySelectorAll('div.dynheader');
    for (var i = 0; i < headers.length; i++) {
      if (!/(Inheritance|Collaboration) diagram/i.test(headers[i].textContent)) {
        continue;
      }
      var content = headers[i].nextElementSibling;
      if (!content || !content.classList.contains('dyncontent')) {
        continue;
      }
      wireCollapsible(headers[i], content);
    }
  }

  onReady(function () {
    var contents = document.querySelector('div.contents');
    if (!contents) {
      return;
    }
    collapseDiagrams(contents);
  });
})();
