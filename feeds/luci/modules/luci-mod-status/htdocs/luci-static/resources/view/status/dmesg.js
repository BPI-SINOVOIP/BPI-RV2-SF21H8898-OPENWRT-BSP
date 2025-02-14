'use strict';
'require view';
'require fs';
'require ui';

return view.extend({
	load: function() {
		return fs.exec_direct('/bin/dmesg', [ '-r' ]).catch(function(err) {
			ui.addNotification(null, E('p', {}, _('Unable to load log data: ' + err.message)));
			return '';
		});
	},

	handleDownload: function(ev) {
		var logdata = document.getElementById('syslog').value;
		var filename = 'Kernel_Log.txt';
		var blob = new Blob([logdata], { type: 'text/plain' });
		var url = URL.createObjectURL(blob);
		var a = document.createElement("a");
		a.href = url;
		a.download = filename;
		a.click();
		URL.revokeObjectURL(url);
	},

	render: function(logdata) {
		var loglines = logdata.trim().split(/\n/).map(function(line) {
			return line.replace(/^<\d+>/, '');
		});

		return E([], [
			E('h2', {}, [ _('Kernel Log') ]),
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
