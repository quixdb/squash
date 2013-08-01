if (!Array.prototype.forEach) {
  Array.prototype.forEach = function (fn, scope) {
    'use strict';
    var i, len;
    for (i = 0, len = this.length; i < len; ++i) {
      if (i in this) {
        fn.call(scope, this[i], i, this);
      }
    }
  };
}

google.load('visualization', '1', {packages:['table', 'corechart']});

var benchmark = new function () {
  var _this = this;

  _this.data = [];

  $.getJSON('data.json', function(json) {
    google.setOnLoadCallback(function () {
      Object.getOwnPropertyNames (json).forEach (function (dataset, i, a) {
        var data = new google.visualization.DataTable ();

        data.addColumn('string', 'Plugin');
        data.addColumn('string', 'Codec');
        data.addColumn('number', 'Compression Ratio');
        data.addColumn('number', 'Compress CPU Time');
        data.addColumn('number', 'Decompress CPU Time');

        data.addRows (json[dataset].map (function (e) {
          return [e.plugin, e.codec, e.ratio, e.compress_cpu, e.decompress_cpu];
        }));

        _this.data[dataset] = data;
      });
    });
  });

  this.load = function (name) {
    var visualizations = [];
    var result = $('<div id="' + name + '"></div>');

    $("#results").empty ().append (result);
    result.append ("<h2>" + name + " Results</h2>");

    {
      var result_table = $("<div></div>");
      result.append (result_table);
      var table = new google.visualization.Table (result_table.get (0));
      visualizations.push (table);

      table.draw (_this.data[name]);
    }

    var compression_ratio_view = new google.visualization.DataView (_this.data[name]);
    compression_ratio_view.setColumns ([1, 2]);

    var compression_ratio_container = $("<div></div>");
    result.append (compression_ratio_container);
    var compression_ratio = new google.visualization.BarChart (compression_ratio_container.get (0));
    compression_ratio.draw (compression_ratio_view, {
      legend: 'none',
      title: 'Compression Ratio',
      vAxis: { title: 'Codecs' },
      height: ($("#results").innerWidth () * (9 / 16))
    });
    visualizations.push (compression_ratio);

    var compression_ratio_vs_time_view = new google.visualization.DataView (_this.data[name]);
    compression_ratio_vs_time_view.setColumns ([2, 3]);

    var container = $("<div></div>");
    result.append (container);
    var compression_ratio_vs_time = new google.visualization.ScatterChart (container.get (0));
    compression_ratio_vs_time.draw (compression_ratio_vs_time_view, {
      legend: 'none',
      title: 'Compression Ratio vs. Time',
      hAxis: { title: 'Ratio', minValue: 0 },
      vAxis: { title: 'Time (logarithmic)', minValue: 0, logScale: true },
      height: ($("#results").innerWidth () * (9 / 16))
    });
    visualizations.push (compression_ratio_vs_time);

    visualizations.forEach (function (visualization) {
      google.visualization.events.addListener(visualization, 'select', function() {
        var selection = visualization.getSelection ();
        visualizations.forEach (function (v) {
          if (v !== visualization) {
            v.setSelection (selection);
          }
        });
      });
    });
  };
} ();

function drawTable() {
  $.getJSON('data.json', function(json) {
    // data.addColumn('number', 'Compress Wall Clock Time');
    // data.addColumn('number', 'Decompress Wall Clock Time');

    var results = $(document.getElementById ('results'));

    Object.getOwnPropertyNames (json).forEach (function (dataset, i, a) {
      var result = $('<div id="' + dataset + '"></div>');
      result.append ("<h3>" + dataset + "</h3>");

      results.append (result);
    });

    // data.addRows (json.data.map (function (e) {
    //   return [e.plugin, e.codec, e.ratio, e.compress_cpu, e.decompress_cpu];
    // }));

    var table = new google.visualization.Table(document.getElementById('results-table'));

    table.draw(data, {showRowNumber: true});
  });
}