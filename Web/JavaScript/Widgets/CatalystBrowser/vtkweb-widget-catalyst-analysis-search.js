(function ($, GLOBAL) {
    var SEARCH_TEMPLATE = '<div class="search-toolbar"><b>Query</b><input type="text" class="query-expression"/><b>Sort&nbsp;by</b><input type="text" class="sort-expression"/><input type="range" min="10" max="100" value="10" class="zoom-level"/><span class="result-count"></span><span class="vtk-icon-chart-area toggle-stats stats action search-button"></span><span class="vtk-icon-picture-1 render-all action search-button" title="Render all images" alt="Render all images"></span><i>HELP</i></div><div class="query-results"></div>',
    TOOLBAR_TEMPLATE = '<div class=sub-menu><ul class="menu left"><li class="vtk-icon-list-add sub action" data-type="composite-image-stack"><ul></ul></li><li class="vtk-icon-chart-line sub action" data-type="catalyst-resample-viewer"><ul></ul></li><li class="vtk-icon-loop-alt sub action" data-type="catalyst-viewer"><ul></ul></li></ul><ul class="menu right"><li class="search-title"/></ul></div><div class="search-panel"></div>',
    ENTRY_TEMPLATE = '<li class="create-search" data-path="PATH" data-title="TITLE">TITLE<i class=help>DESCRIPTION</i></li>',
    SEARCH_FACTORY = {
        "catalyst-viewer": function(domToFill, path) {
            domToFill.vtkCatalystAnalysisGenericSearch(path);
        },
        "catalyst-resample-viewer" : function(domToFill, path) {
            domToFill.vtkCatalystAnalysisGenericSearch(path);
        },
        "composite-image-stack" : function(domToFill, path) {
            domToFill.vtkCatalystCompositeSearch(path);
        },
        "catalyst-pvweb" : function(domToFill, path) {
            domToFill.empty().html("<p style='padding: 20px;font-weight: bold;'>This type of data is not searchable.</p>");
        }
    };

    // ------------------------------------------------------------------------

    function getFileName(filePattern, args) {
        var fileName = filePattern;
        for(key in args) {
            fileName = fileName.replace('{'+key+'}', args[key]);
        }
        return fileName;
    }

    // ------------------------------------------------------------------------

    function buildFileNames(info, basePath) {
        var results = [],
        args = info['arguments'],
        pattern = info['name_pattern'],
        keyNames = [],
        valueCounts = [],
        valueIndexes = [];

        // Fill args infos
        for(var key in args) {
            keyNames.push(key);
            valueCounts.push(args[key]['values'].length);
            valueIndexes.push(0);
        }

        function keepGoing() {
            var count = valueCounts.length;
            while(count--) {
                if(valueCounts[count] != valueIndexes[count] + 1) {
                    return true;
                }
            }
            return false;
        }

        function increment() {
            var idx = 0;
            valueIndexes[idx]++;

            while(valueIndexes[idx] % valueCounts[idx] === 0) {
                valueIndexes[idx] = 0;
                idx++;
                valueIndexes[idx]++;
            }
        }

        function getCurrentArgs() {
            var result = {},
            count = keyNames.length;
            while(count--) {
                result[keyNames[count]] = args[keyNames[count]]["values"][valueIndexes[count]];
            }

            return result;
        }

        while(keepGoing()) {
            var currentArgs = getCurrentArgs(),
            url = basePath + '/' + getFileName(pattern, currentArgs);

            results.push( { "args": currentArgs, "url": url, "keep": true } );

            // Move to next possibility
            increment();
        }

        return results;
    }

    // ------------------------------------------------------------------------

    function extractQueryDocumentation(info) {
         var txtBuffer = [],
         args = info["arguments"],
         template = "<b>KEY</b>: [VALUES]<br/>",
         values = null;

         for(var key in args) {
            values = args[key]["values"];
            if(args[key]['type'] === 'range') {
               var txt = "";
               txt += values[0];
               txt += " to ";
               txt += values[values.length - 1];
               txt += " % ";
               txt += (Number(values[1]) - Number(values[0]));
               values = txt;
            }
            txtBuffer.push(template.replace(/KEY/g, key).replace(/VALUES/g, values));
         }

         return txtBuffer.join('');
    }

    // ------------------------------------------------------------------------

    function filterBy(container, expression) {
        var functionStr = 'var LOCAL_VARS; return (EXP);',
        template = 'ARG = args["ARG"]',
        localVarsStr = [],
        validator = null,
        all = container.data('data-list'),
        firstArgs = all[0]['args'],
        count = all.length,
        nbValidResults = 0;

        // Generate filter function
        for(var key in firstArgs) {
            localVarsStr.push(template.replace(/ARG/g, key));
        }
        functionStr = functionStr.replace(/LOCAL_VARS/g, localVarsStr.join(',')).replace(/EXP/g, expression);
        validator = new Function('args', functionStr);

        // Apply function
        while(count--) {
            all[count]['keep'] = validator(all[count]["args"]);
            if(all[count]['keep']) {
                nbValidResults++;
            }
        }

        return nbValidResults;
    }

    // ------------------------------------------------------------------------

    function showResults(container) {
        var results = container.data('data-list'),
        resultContainer = $('.query-results', container),
        imageList = [],
        count = results.length,
        sortQuery = $('.sort-expression', container).val(),
        imgStr = '<div class="query-result" data-url="URL"><div class="query-stats">STATS</div><img class="image-result"/></div>';

        // Sort results if possible
        if(sortQuery.trim().length > 0) {
            var exposeVars = "var noop = 0";
            for(var key in results[0]['args']) {
                exposeVars += ', ' + key + ' = obj.args["' + key + '"]';
            }
            exposeVars += ';';

            var sortFunctionSTR = "function extractValue(obj) {" + exposeVars + "return " + sortQuery + ";}; return extractValue(a) - extractValue(b);",
            sortFunc = new Function(["a","b"], sortFunctionSTR);
            results.sort(sortFunc);
        }

        while(count--) {
            if(results[count]['keep']) {
                imageList.push(imgStr.replace(/URL/g, results[count]['url']).replace(/STATS/g, JSON.stringify(results[count]["args"]).replace(/["{}]/g,'').replace(/,/g,'<br/>').replace(/:/g,' : ')));
            }
        }

        resultContainer.empty()[0].innerHTML = imageList.join('');
        $('.toggle-stats', container).addClass('stats');
        $('.zoom-level', container).trigger('change');
        $('.query-result', container).click(function(){
            var me = $(this), img = $('img', me), url = me.attr('data-url');
            img.attr('src', url);
        });
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        var query = $('.query-expression', container),
        zoom = $('.zoom-level', container),
        sort = $('.sort-expression', container),
        resultCountTxt = $('.result-count', container),
        toggleStats = $('.toggle-stats', container),
        renderAll = $('.render-all', container),
        resultCount = 0;

        query.change(function(){
            resultCount = filterBy(container, query.val());
            if(resultCount < 500) {
                showResults(container);
            }
            resultCountTxt.html("Found VAL results.".replace(/VAL/g, resultCount));
        });

        zoom.bind('change mousemove keyup', function(){
            var widthRef = $(window).width() * Number($(this).val()) / 100.0;
            $('.query-result', container).css('width', widthRef).css('height', widthRef);
        })

        sort.bind('change keyup', function(e){
            // Apply search
            if(e.type === 'keyup' && e.keyCode !== 13) {
                return;
            }
            query.trigger('change');
        });

        toggleStats.click(function(){
            var me = $(this).toggleClass('stats'), isActive = me.hasClass('stats');
            if(isActive) {
                $('.query-stats').show();
            } else {
                $('.query-stats').hide();
            }
        });

        renderAll.click(function(){
            $('.query-result', container).each(function(){
                var me = $(this), img = $('img', me), url = me.attr('data-url');
                img.attr('src', url);
            });
        });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisGenericSearch = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-analysis-search');

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);

                    // Build file list
                    me.data('data-list', buildFileNames(data, dataBasePath));

                    // Build UI
                    me.html(SEARCH_TEMPLATE.replace(/HELP/g, extractQueryDocumentation(data)));

                    // Attach interaction listeners
                    initializeListeners(me);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisSearch = function(project, dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().html(TOOLBAR_TEMPLATE),
            menu = $('.menu.left', me),
            buffer = [],
            analysis = project.analysis,
            count = analysis.length,
            containers = {
                "composite-image-stack" : $('.menu.left > li[data-type="composite-image-stack"] > ul', me),
                "catalyst-resample-viewer" : $('.menu.left > li[data-type="catalyst-resample-viewer"] > ul', me),
                "catalyst-viewer" : $('.menu.left > li[data-type="catalyst-viewer"] > ul', me)
            },
            buffers = { "composite-image-stack" : [], "catalyst-resample-viewer" : [], "catalyst-viewer" : [], "catalyst-pvweb" : [] },
            rootContainer = me;

            // Fill buffers
            while(count--) {
                var item = analysis[count];
                buffers[item.type].push(ENTRY_TEMPLATE.replace(/PATH/g, dataBasePath + '/' + item.id).replace(/TITLE/g, item.title).replace(/DESCRIPTION/g, item.description));
            }

            // Update UI
            for(var key in containers) {
                containers[key].html(buffers[key].join(''));
            }

            // Attach search query listeners
            $('.create-search', me).addClass('action').click(function(){
                var me = $(this),
                path = me.attr('data-path'),
                type = me.parent().parent().attr('data-type'),
                title = me.attr('data-title'),
                searchPanel = $('.search-panel', rootContainer).removeClass().addClass('search-panel').unbind().empty();

                $('.search-title', rootContainer).html(title);
                SEARCH_FACTORY[type](searchPanel, path);
            });
        });
    }

    }(jQuery, window));