L.ui.view.extend({
	refresh: 5000,

	getSystemLog: L.rpc.declare({
		object: 'luci2.system',
		method: 'syslog',
		expect: { log: '' }
	}),

	execute: function() {
		return this.getSystemLog().then(function(log) {
			var ta = document.getElementById('syslog');
			var lines = log.replace(/\n+$/, '').split(/\n/);

			ta.rows = lines.length;
			ta.value = lines.reverse().join("\n");
			$(ta).prop('wrap', 'on');
			$(ta).css({
				border: 'none',
				background: 'white',
				'box-shadow': 'none',
				'width': '730px',
				'height': '1200px',
				'resize': 'none',
				cursor: 'auto',
			});
		});
	}
});
