'use strict';
'require view';
'require fs';
'require ui';

return view.extend({
	load: function() {
		return Promise.all([
			L.resolveDefault(fs.stat('/sbin/logread'), null),
			L.resolveDefault(fs.stat('/usr/sbin/logread'), null)
		]).then(function(stat) {
			var logger = stat[0] ? stat[0].path : stat[1] ? stat[1].path : null;

			return fs.exec_direct(logger, [ '-e', '^' ]).catch(function(err) {
				ui.addNotification(null, E('p', {}, _('Unable to load log data: ' + err.message)));
				return '';
			});
		});
	},

	handleDownload: function(ev) {
		var logdata = document.getElementById('syslog').value;
		var filename = 'System_Log.txt';
		var blob = new Blob([logdata], { type: 'text/plain' });
		var url = URL.createObjectURL(blob);
		var a = document.createElement("a");
		a.href = url;
		a.download = filename;
		a.click();
		URL.revokeObjectURL(url);
	},

	render: function(logdata) {
		var loglines = logdata.trim().split(/\n/);

		return E([], [
			E('h2', {}, [ _('System Log') ]),
			E('button', {
				'class': 'btn cbi-button-action important',
				'click': L.bind(this.handleDownload, this)
			}, [ _('Save Log') ]),
			E('div', { 'id': 'content_syslog' }, [
				E('textarea', {
					'id': 'syslog',
					'style': 'font-size:12px',
					'readonly': 'readonly',
					'wrap': 'off',
					'rows': loglines.length + 1
				}, [ loglines.join('\n') ])
			])
		]);
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
