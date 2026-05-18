/**
 * VTK Documentation Version Selector
 *
 * Adds a version dropdown near the VTK logo in Doxygen-generated pages.
 * Reads available versions from vtk_versions.json and navigates to the
 * corresponding page on version change.
 *
 * Fetch order:
 *   1. Shared server-level JSON (maintained by CI across all releases).
 *   2. Local vtk_versions.json bundled with this documentation build.
 */
(function () {
  "use strict";

  // Replaced by CMake's configure_file(@ONLY).
  var defined_current_version = "@VTK_CURRENT_VERSION@";

  // The shared JSON lives two levels up from a versioned doc root,
  // e.g. /doc/vtk_versions.json when docs are at /doc/release/9.6/html/.
  // For nightly docs at /doc/nightly/html/ it is at /doc/vtk_versions.json.
  // We also accept a base URL override so deployers can customise.
  var defined_shared_json_url = "@VTK_DOC_VERSIONS_URL@";

  // Local fallback bundled with the build.
  var defined_local_json_url = "vtk_versions.json";

  // ------------------------------------------------------------------
  // Helpers
  // ------------------------------------------------------------------

  /**
   * Try to figure out the shared JSON URL from the page location when the
   * CMake variable was not set (local / non-deployed builds).
   */
  function resolveSharedJsonUrl() {
    if (defined_shared_json_url &&
        defined_shared_json_url.indexOf("@") !== 0) {
      return defined_shared_json_url;
    }
    // Heuristic: walk up from .../html/<page> to the doc root.
    // /doc/release/9.6/html/index.html  →  /doc/vtk_versions.json
    // /doc/nightly/html/index.html      →  /doc/vtk_versions.json
    var parts = location.pathname.split("/");
    var htmlIdx = parts.lastIndexOf("html");
    if (htmlIdx > 0) {
      return parts.slice(0, htmlIdx - 1).join("/") + "/vtk_versions.json";
    }
    return null;
  }

  /**
   * Fetch JSON from *url*. Calls cb(data) on success, errCb() on failure.
   */
  function fetchJson(url, cb, errCb) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    xhr.onreadystatechange = function () {
      if (xhr.readyState !== 4) return;
      if (xhr.status === 200) {
        try {
          cb(JSON.parse(xhr.responseText));
        } catch (e) {
          (errCb || function () {})();
        }
      } else {
        (errCb || function () {})();
      }
    };
    xhr.send();
  }

  // ------------------------------------------------------------------
  // UI
  // ------------------------------------------------------------------

  function createSelector(versions, currentVersion) {
    var select = document.createElement("select");
    select.id = "vtk-version-selector";
    select.title = "Switch VTK documentation version";

    versions.forEach(function (v) {
      var opt = document.createElement("option");
      opt.value = v.baseUrl || "";
      opt.textContent = v.name;
      if (v.version === currentVersion) {
        opt.selected = true;
      }
      select.appendChild(opt);
    });

    select.addEventListener("change", function () {
      var url = select.value;
      if (url) {
        var currentPage = location.pathname.split("/").pop() || "index.html";
        var target = url.replace(/\/$/, "") + "/" + currentPage;
        window.location.href = target;
      }
    });

    return select;
  }

  function injectSelector(select) {
    var projectName = document.getElementById("projectname");
    if (projectName) {
      var wrapper = document.createElement("div");
      wrapper.id = "vtk-version-selector-wrapper";
      wrapper.appendChild(select);
      projectName.parentNode.insertBefore(wrapper, projectName.nextSibling);
      return;
    }

    var titleArea = document.getElementById("titlearea");
    if (titleArea) {
      var td = document.createElement("td");
      td.style.paddingLeft = "1em";
      td.style.verticalAlign = "middle";
      td.appendChild(select);
      var row = titleArea.querySelector("tr");
      if (row) {
        row.appendChild(td);
      }
    }
  }

  function buildUI(data) {
    var versions = data.versions || [];
    if (versions.length > 0) {
      injectSelector(createSelector(versions, defined_current_version));
    }
  }

  // ------------------------------------------------------------------
  // Init
  // ------------------------------------------------------------------

  function init() {
    var sharedUrl = resolveSharedJsonUrl();
    if (sharedUrl) {
      // Try the shared server-level JSON first.
      fetchJson(sharedUrl, buildUI, function () {
        // Fall back to the local copy bundled with this build.
        fetchJson(defined_local_json_url, buildUI);
      });
    } else {
      fetchJson(defined_local_json_url, buildUI);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
