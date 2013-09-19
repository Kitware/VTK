/**
 * vtkWebLoader JavaScript Library.
 *
 * vtkWebLoader use the vtkWeb namespace to manage JavaScript dependency and more specifically
 * vtkWeb dependencies.
 *
 * @class vtkWebLoader
 *
 * @singleton
 */
(function (GLOBAL) {

    var vtkWebLibs = {
        "core-min" : [
        "ext/js-core/jquery-1.8.3.min.js",
        "ext/js-core/autobahn.min.js",
        "lib/js/vtkweb-all.min.js"
        ],
        "core" : [
        "ext/js-core/jquery-1.8.3.min.js",
        "ext/js-core/autobahn.js",
        "lib/js/vtkweb-all.js"
        ],
        "webgl-min" : [
        "ext/js-core/gl-matrix-min.js"
        ],
        "webgl" : [
        "ext/js-core/gl-matrix.js"
        ],
        "mobile-min" : [
        "ext/js-core/jquery.hammer.min.js"
        ],
        "mobile" : [
        "ext/js-core/jquery.hammer.js"
        ],
        "all" : [
        "ext/js-core/jquery-1.8.3.min.js",
        "ext/js-core/autobahn.js",
        "lib/js/vtkweb-all.js",
        "ext/js-core/gl-matrix.js",
        "ext/js-core/jquery.hammer.js"
        ],
        "all-min": [
        "ext/js-core/jquery-1.8.3.min.js",
        "ext/js-core/autobahn.min.js",
        "lib/js/vtkweb-all.min.js",
        "ext/js-core/gl-matrix-min.js",
        "ext/js-core/jquery.hammer.min.js"
        ]
    },
    modules = [],
    script = document.getElementsByTagName("script")[document.getElementsByTagName("script").length - 1],
    basePath = "",
    extraScripts = [];

    // Extract modules to load
    try {
        modules = script.getAttribute("load").split(",");
        for(var j in modules) {
            modules[j] = modules[j].replace(/^\s+|\s+$/g, ''); // Trim
        }
    } catch(e) {
    // We don't care we will use the default setup
    }

    // Extract extra script to load
    try {
        extraScripts = script.getAttribute("extra").split(",");
        for(var j in extraScripts) {
            extraScripts[j] = extraScripts[j].replace(/^\s+|\s+$/g, ''); // Trim
        }
    } catch(e) {
    // We don't care we will use the default setup
    }

    // If no modules have been defined, just pick the default
    if(modules.length == 0) {
        modules = [ "all-min" ];
    }

    // Extract basePath
    var lastSlashIndex = script.getAttribute("src").lastIndexOf('lib/js/vtkweb-loader');
    if(lastSlashIndex != -1) {
        basePath = script.getAttribute("src").substr(0, lastSlashIndex);
    }

    // Add missing libs
    for(var i in modules) {
        for(var j in vtkWebLibs[modules[i]]) {
            document.write('<script src="' + basePath + vtkWebLibs[modules[i]][j] + '"></script>');
        }
    }

    // Add extra libs
    for(var i in extraScripts) {
        document.write('<script src="' + extraScripts[i] + '"></script>');
    }

    // Remove loader
    script.parentNode.removeChild(script);
}(window));
