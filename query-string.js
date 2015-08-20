

$(document).ready (function () {
    var qs = (function () {
	// http://stackoverflow.com/a/979995
	var query_string = {};
	var query = window.location.search.substring(1);
	var vars = query.split("&");
	for (var i=0;i<vars.length;i++) {
	    var pair = vars[i].split("=");
	    if (typeof query_string[pair[0]] === "undefined") {
		query_string[pair[0]] = pair[1];
	    } else if (typeof query_string[pair[0]] === "string") {
		var arr = [ query_string[pair[0]], pair[1] ];
		query_string[pair[0]] = arr;
	    } else {
		query_string[pair[0]].push(pair[1]);
	    }
	}
	return query_string;
    })();

    if (qs.email != undefined) {
	$("#announce-email").val (qs.email);
    }
});
