/**
 * VTK-Web Widget Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to 2D chart visualization. This widget depends on D3 and Rickshaw.
 *
 * @class jQuery.vtk.ui.Chart
 */
(function ($) {

    // =======================================================================
    // ==== Defaults constant values =========================================
    // =======================================================================
    var GRAPH_HTML_TEMPLATE = [
        "<div class='vtk-legend'></div>",
        "<div class='vtk-top' style='left: AXIS_SIZE px; top: 0px; height: AXIS_SIZE px; position: absolute;'></div>",
        "<div class='vtk-left' style='left: 0 px; top: AXIS_SIZE px; width: AXIS_SIZE px; position: absolute;'></div>",
        "<div class='vtk-center' style='left: AXIS_SIZE px; top: AXIS_SIZE px; position: relative;'></div>",
        "<div class='vtk-right' style='right: 0px; top: AXIS_SIZE px; position: absolute;'></div>",
        "<div class='vtk-bottom' style='left: AXIS_SIZE px; bottom: 0px; position: absolute;'></div>",
        "<div class='vtk-annotation' style='left: AXIS_SIZE px; bottom: 0px; position: absolute;'></div>"
    ];

    // =======================================================================

    function toNumber(str) {
        return Number(str.replace(/^\s+|\s+$/g, ''));
    }

    // =======================================================================

    function extractColumnHeaderMap(headerLine) {
        var header = headerLine.split(','),
        colIdxMap = {};
        for(var idx in header) {
            colIdxMap[header[idx]] = idx;
        }
        return colIdxMap;
    }

    // =======================================================================

    function singleDataCSVConverter(inputString, outputSeries, options) {
        var lines = inputString.split('\n'),
        data = [],
        serie = $.extend({data:data}, options),
        nbLines = lines.length;

        // Process data
        for(var i = 1; i < nbLines; ++i) {
            var values = lines[i].split(',');
            if(values.length === 2) {
                item = { x: toNumber(values[0]), y: toNumber(values[1]) };
                data.push(item);
            }
        }
        outputSeries.push(serie);
    }

    // =======================================================================

    function multiDataCSVConverter(inputString, outputSeries, options) {
        var lines = inputString.split('\n'),
        header = lines[0].split(','),
        headerMap = extractColumnHeaderMap(lines[0]),
        nbLines = lines.length,
        nbValuesByLines = header.length,
        xHeaderName = options['x'],
        xIdx = headerMap[xHeaderName],
        series = [],
        palette = new Rickshaw.Color.Palette();

        if(options.hasOwnProperty('palette') && options['palette'] !== null) {
            palette = options['palette'];
        }

        // Remove time field
        header.splice(header.indexOf(xHeaderName), 1);

        // Create series
        for(var idx in header) {
            var serie = {
                data: [],
                color: palette.color(),
                name: header[idx]
            };

           series.push(serie);
           outputSeries.push(serie);
        }

        // Process data
        for(var i = 1; i < nbLines; ++i) {
            var values = lines[i].split(',');
            if(values.length === nbValuesByLines) {
                xValue =  toNumber(values[xIdx]);
                for(var idx in header) {
                    var item = { x: xValue, y: toNumber(values[headerMap[header[idx]]])};
                    if(item.y === NaN) {
                        item.y = null;
                    }
                    if(item.x !== NaN) {
                        series[idx].data.push(item);
                    }
                }
            }
        }
    }

    // =======================================================================

    function updateLegend(container) {
        var legendContainer = $('.vtk-legend', container),
        chart = container.data('chart'),
        legend = chart['legends'].basic;

        // Empty UI
        legendContainer.children("ul").empty();

        // Update model
        if(legend !== undefined) {
            legend.lines = [];
            var series = chart.graph.series.map( function(s) { return s } )
            series.forEach(function(s) {
                legend.addLine(s);
            });
        }
    }

    // =======================================================================

    /**
     * Method used to create a 2D chart based on some available data.
     *
     * @member jQuery.vtk.ui.Chart
     * @method vtkChart
     * @param {Object} configuration
     *
     * Usage:
     *      var options = {
     *         'renderer': 'line',  // Type of chart [line, area, bar, scatterplot]
     *         'stacked' : false,
     *         'series': [
     *            {
     *                data: [ { x:0, y:0 }, { x:100, y:10 }, { x:200, y:5 }, { x:300, y:20 }, { x:400, y:25 }, { x:1000, y:-10 } ],
     *                color: 'steelblue',
     *                name: 'field 0'
     *            },{
     *                data: [ { x:0, y:20 }, { x:100, y:30 }, { x:200, y:25 }, { x:300, y:40 }, { x:400, y:55 }, { x:1000, y:-10 } ],
     *                color: 'lightblue',
     *                name: 'field 1'
     *            }
     *        ],
     *        'axes': [ "bottom", "left", "top"], // Draw axis on border with scale
     *        'chart-padding': [0, 150, 50, 0],   // Graph padding [top, right, bottom, left] in px. Useful to save space for legend
     *      };
     *
     *      $('.chart-container-div').vtkChart(options);
     */

    $.fn.vtkChart = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.vtkChart.defaults, options);

        return this.each(function() {
            var me = $(this).empty().addClass('vtk-chart'),
            container = $("<div/>", {
                html: GRAPH_HTML_TEMPLATE.join('').replace(/AXIS_SIZE /g, opts.axisThickness)
            }),
            chartContainer = $('.vtk-center', container),
            legendContainer = $('.vtk-legend', container),
            axisContainer = {
                bottom: $('.vtk-bottom', container)[0],
                top: $('.vtk-top', container)[0],
                left: $('.vtk-left', container)[0],
                right: $('.vtk-right', container)[0]
            },
            annotationContainer = $('.vtk-annotation', container);
            me.append(container);
            // container.css('width', (opts['width']+(2*opts.axisThickness)) + 'px');

            var graphOptions = {
                element: chartContainer[0],
                width: opts['width'],
                height: opts['height'],
                renderer: opts['renderer'],
                min: 'auto',
                stroke: true,
                series: opts['series']
            },
            graph = new Rickshaw.Graph(graphOptions),
            axes = [],
            legends = {},
            annotator = null,
            data = {
                configuration: graphOptions,
                options: opts,
                palette: new Rickshaw.Color.Palette(),
                graph: graph,
                axes: axes,
                legends: legends
            };

            graph.renderer.unstack = !opts.stacked;
            graph.render();

            // Complete graph accessories
            // => Axis
            for(var idx in opts.axes) {
                var orientation = opts.axes[idx], axis = null;
                if(orientation === 'top' || orientation === 'bottom') {
                    axis = new Rickshaw.Graph.Axis.X({graph: graph, orientation: orientation, element: axisContainer[orientation]});
                } else {
                    axis = new Rickshaw.Graph.Axis.Y({graph: graph, orientation: orientation, element: axisContainer[orientation]});
                }
                axes.push(axis);
            }
            // => Legend
            if(opts.legend.basic) {
                legends['basic'] = new Rickshaw.Graph.Legend({graph: graph, element: legendContainer[0]});
                // if(opts.legend.toggle) {
                //     legends['toggle'] = new Rickshaw.Graph.Behavior.Series.Toggle({graph: graph, legend: legends['basic']});
                // }
                // if(opts.legend.highlight) {
                //     legends['highlight'] = new Rickshaw.Graph.Behavior.Series.Highlight({graph: graph, legend: legends['basic']});
                // }
            }
            // => Hover
            if(opts.hover !== null) {
                data['hover'] = new Rickshaw.Graph.HoverDetail({
                    graph: graph,
                    xFormatter: opts.hover.xFormatter,
                    yFormatter: opts.hover.yFormatter
                });
            }
            // => Annotation
            data['annotator'] = new Rickshaw.Graph.Annotate({
                graph: graph,
                element: annotationContainer[0]
            });
            for(var idx in opts.annotations) {
                var annotation = opts.annotations[idx];
                data['annotator'].add(annotation['time'], annotation['message']);
            }

            // Handle auto-resize
            if(opts.autosize) {
                function autoResize() {
                    var w = $(window),
                    padding = opts['chart-padding'],
                    thickness = opts.axisThickness,
                    size = { width: me.width() - (2*thickness) - (padding[1] + padding[3]), height: me.height() - (2*thickness) - (padding[0] + padding[2])};
                    $('.vtk-bottom, .vtk-top, .vtk-annotation', me).css('height', thickness +'px').css('width', size['width'] +'px');
                    $('.vtk-left, .vtk-right', me).css('width', thickness +'px').css('height', size['height'] +'px');
                    $('.vtk-right', me).css('right', padding[1] + 'px').css('top', (padding[0] + thickness) + 'px');
                    $('.vtk-left', me).css('left', padding[3] + 'px').css('top', (padding[0] + thickness) + 'px');
                    $('.vtk-top', me).css('top', padding[0] + 'px').css('left', (thickness+padding[3]) + 'px');
                    $('.vtk-bottom, .vtk-annotation', me).css('left', (thickness+padding[3]) + 'px');
                    $('.vtk-bottom', me).css('bottom', padding[2] + 'px');
                    $('.vtk-center', me).css('width', (size['width'] - 2*thickness - (padding[1] + padding[3]))+'px').css('height', (size['height'] - 2*thickness - (padding[0] + padding[2]))+'px').css('left', (padding[3]+thickness) + 'px').css('top', (padding[0]+thickness) + 'px');

                    data.graph.configure(size);
                    data.graph.update();
                }

                $(window).resize(autoResize).trigger('resize');
            }

            // Save data
            me.data('chart', data);
            graph.render();
        });
    };

    // =======================================================================
    /**
     * Method used to update the data of the 2D chart.
     *
     * @member jQuery.vtk.ui.Chart
     * @method vtkChartUpdateData
     * @param {Array} series
     * @param {boolean} replace previous series
     *
     * Usage:
     *      var series: [
     *         {
     *            data: [ { x:0, y:0 }, { x:100, y:10 }, { x:200, y:5 }, { x:300, y:20 }, { x:400, y:25 }, { x:1000, y:-10 } ],
     *            color: 'steelblue',
     *            name: 'field 0'
     *         },{
     *            data: [ { x:0, y:20 }, { x:100, y:30 }, { x:200, y:25 }, { x:300, y:40 }, { x:400, y:55 }, { x:1000, y:-10 } ],
     *            color: 'lightblue',
     *            name: 'field 1'
     *         }
     *      ];
     *
     *      $('.chart-container-div').vtkChartUpdateData(series);
     */
    $.fn.vtkChartUpdateData = function(series, replace) {
        return this.each(function() {
            var me = $(this),
            data = me.data('chart'),
            dataset = data['configuration']['series'];
            if(replace) {
                while(dataset.length > 0) {
                    dataset.pop();
                }
            }
            for(var idx in series) {
                data.graph.series.push(series[idx]);
            }
            data.graph.validateSeries(data.graph.series);
            data.graph.update();
            updateLegend(me);
        });
    }

    // =======================================================================

    /**
     * Method used to update the data of the 2D chart.
     *
     * @member jQuery.vtk.ui.Chart
     * @method vtkChartFetchData
     * @param {Object} options
     *
     * Usage:
     *      var options_json = { replace: true, url: "data.json", type: 'json', converter: null };
     *      var options_csv_1 = { replace: true, url: "data1.csv", type: 'csv-xy', options: { name: 'Temperature', color: palette.color(), ... } };
     *      var options_csv_n = { replace: true, url: "data2.csv", type: 'csv-x*', options: { x: 'time', palette: null } };
     *
     *      $('.chart-container-div').vtkChartFetchData(options_*);
     *
     * Where data looks like:
     *
     *     data.json
     *       [
     *         {
     *            data: [ { x:0, y:0 }, { x:100, y:10 }, { x:200, y:5 }, { x:300, y:20 }, { x:400, y:25 }, { x:1000, y:-10 } ],
     *            color: 'steelblue',
     *            name: 'field 0'
     *         },{
     *            data: [ { x:0, y:20 }, { x:100, y:30 }, { x:200, y:25 }, { x:300, y:40 }, { x:400, y:55 }, { x:1000, y:-10 } ],
     *            color: 'lightblue',
     *            name: 'field 1'
     *         }
     *       ]
     *
     *
     *     data1.csv
     *     x,y
     *     0,0
     *     1,0.234
     *     2,0.5
     *     2.5,7
     *
     *
     *     data2.csv
     *     time,x,y,z
     *     0,0,0,0
     *     1,0.234,1.2,7.6
     *     2,0.5,3,6
     *     2.5,7,8,9
     */
    $.fn.vtkChartFetchData = function(info) {
        return this.each(function() {
            var me = $(this),
            data = me.data('chart'),
            options = info['options'];

            $.ajax({
                url: info.url,
                dataType: "text"
            }).done(function(data){
                var series = [];
                if (info.type === 'json') {
                    series = $.parseJSON(data);
                } else if(info.type === 'csv-xy') {
                    singleDataCSVConverter(data, series, options);
                } else if(info.type === 'csv-x*') {
                    multiDataCSVConverter(data, series, options);
                }
                me.vtkChartUpdateData(series, info['replace']);
            });
        });
    }

    // =======================================================================
    /**
     * Method used to update the data of the 2D chart.
     *
     * @member jQuery.vtk.ui.Chart
     * @method vtkChartConfigure
     * @param {Object} options
     *
     * Usage:
     *
     *     var options = {
     *        'renderer': 'line',  // Type of chart [line, area, bar, scatterplot]
     *        'stacked' : false,
     *        'axes': [ "bottom", "left", "top"], // Draw axis on border with scale
     *        'chart-padding': [0, 150, 50, 0],   // Graph padding [top, right, bottom, left] in px. Useful to save space for legend
     *      };
     *      $('.chart-container-div').vtkChartConfigure(options);
     */

    $.fn.vtkChartConfigure = function(conf) {
        return this.each(function() {
            var me = $(this),
            data = me.data('chart');
            var opts = $.extend(data['options']['configuration'], conf);
            $('.x_axis_d3', me).height(data.axisThickness + 'px').width(($(window).width()-(2*data.axisThickness)) + 'px');
            data.graph.configure(opts);
            data.graph.update();
        });
    };

    // =======================================================================

    $.fn.vtkChart.defaults = {
        width: 300,
        height: 200,
        axisThickness: 25,
        autosize: true,
        stacked: false,
        renderer: "line",
        interpolation: "linear",
        series: [],
        hover: { xFormatter: function(x) { return x; }, yFormatter: function(y) {return y;} },
        legend: { basic: true, toggle: true, highlight: true },
        annotations: [], // { time: 0, message: "Just a text" } ...
        axes: [ "bottom", "left" ],
        'chart-padding': [0, 0, 0, 0]
    };

    // =======================================================================

}(jQuery));
