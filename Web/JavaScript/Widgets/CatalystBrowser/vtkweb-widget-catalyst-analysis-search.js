(function ($, GLOBAL) {
    var SEARCH_TEMPLATE = '<div class="search-toolbar"><b>Query</b><input type="text" class="query-expression"/><input type="range" min="10" max="100" value="10" class="zoom-level"/><span class="result-count"></span><i>HELP</i></div><div class="query-results"></div>';

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
        width = $('.zoom-level', container).val(),
        imgStr = '<img class="query-result" src="URL" width="WIDTH%" title="ALT" alt="ALT"/>'.replace(/WIDTH/g, width);

        while(count--) {
            if(results[count]['keep']) {
                imageList.push(imgStr.replace(/URL/g, results[count]['url']).replace(/ALT/g, JSON.stringify(results[count]["args"]).replace(/["{}]/g,'').replace(/,/g,'\n').replace(/:/g,' : ')));
            }
        }

        resultContainer.empty()[0].innerHTML = imageList.join('');
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        var query = $('.query-expression', container),
        zoom = $('.zoom-level', container),
        resultCountTxt = $('.result-count', container),
        resultCount = 0;

        query.change(function(){
            resultCount = filterBy(container, query.val());
            if(resultCount < 500) {
                showResults(container);
            }
            resultCountTxt.html("Found VAL results.".replace(/VAL/g, resultCount));
        });

        zoom.bind('change mousemove keyup', function(){
            $('.query-result', container).css('width', $(this).val() +'%');
        })

    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisSearch = function(dataBasePath) {
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

    }(jQuery, window));