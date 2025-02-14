L.ui.view.extend({

	testUpgrade: L.rpc.declare({
		object: 'luci2.system',
		method: 'upgrade_test',
		expect: { '': { } }
	}),

	startUpgrade: L.rpc.declare({
		object: 'luci2.system',
		method: 'upgrade_start',
		params: [ 'keep' ]
	}),

	cleanUpgrade: L.rpc.declare({
		object: 'luci2.system',
		method: 'upgrade_clean'
	}),

	restoreBackup: L.rpc.declare({
		object: 'luci2.system',
		method: 'backup_restore'
	}),

	cleanBackup: L.rpc.declare({
		object: 'luci2.system',
		method: 'backup_clean'
	}),

	getBackupConfig: L.rpc.declare({
		object: 'luci2.system',
		method: 'backup_config_get',
		expect: { config: '' }
	}),

	setBackupConfig: L.rpc.declare({
		object: 'luci2.system',
		method: 'backup_config_set',
		params: [ 'data' ]
	}),

	listBackup: L.rpc.declare({
		object: 'luci2.system',
		method: 'backup_list',
		expect: { files: [ ] }
	}),

	testReset: L.rpc.declare({
		object: 'luci2.system',
		method: 'reset_test',
		expect: { supported: false }
	}),

	startReset: L.rpc.declare({
		object: 'luci2.system',
		method: 'reset_start'
	}),

	getVersion: L.rpc.declare({
		object: 'system',
		method: 'board',
	}),

	callSet: L.rpc.declare({
		object: 'uci',
		method: 'set',
		params: ['config', 'section', 'values']
	}),

	callLoad: L.rpc.declare({
		object: 'uci',
		method: 'get',
		params: ['config', 'section', 'option'],
		expect: { value: ' ' }
	}),

	callCommit: L.rpc.declare({
		object: 'uci',
		method: 'commit',
		params: ['config']
	}),

	get_server_addr: L.rpc.declare({
		object: 'web.system',
		method: 'get_server_addr',
	}),

	write: L.rpc.declare({
		object: 'file',
		method: 'write',
		params: ['path', 'data']
	}),

	log_write: L.rpc.declare({
		object: 'log',
		method: 'write',
		params: ['event']
	}),

	exec: L.rpc.declare({
		object: 'file',
		method: 'exec',
		params: ['command']
	}),

	stat: L.rpc.declare({
		object: 'file',
		mathod: 'stat',
		params: ['path']
	}),

	md5sum: L.rpc.declare({
		object: 'file',
		method: 'md5',
		params: ['path']
	}),

	startOtaUpgrade: L.rpc.declare({
		object: 'luci2.system',
		method: 'upgrade_start',
	}),

	handleFlashUpload: function() {
		var self = this;
		L.ui.upload(
			L.tr('Firmware upload'),
			L.tr('Select the sysupgrade image to flash and click "%s" to proceed.').format(L.tr('Ok')), {
				filename: '/tmp/firmware.bin',
				success: function(info) {
					self.handleFlashVerify(info);
				}
			}
		);
	},

	handleFlashVerify: function(info) {
		var self = this;
		self.testUpgrade().then(function(res) {
			if (res.code == 0) {
				var form = [
						$('<p />').text(L.tr('The firmware image was uploaded completely. Please verify the checksum and file size below, then click "%s" to start the flash procedure.').format(L.tr('Ok'))),
						$('<ul />')
							.append($('<li />')
								.append($('<strong />').text(L.tr('Checksum') + ': '))
								.append(info.checksum))
							.append($('<li />')
								.append($('<strong />').text(L.tr('Size') + ': '))
								.append('%1024mB'.format(info.size))),
						$('<label />')
							.append($('<input />')
								.attr('type', 'checkbox')
								.attr('name', 'keep')
								.prop('checked', true))
							.append(' ')
							.append(L.tr('Keep configuration when reflashing'))
					];

				L.ui.dialog(
					L.tr('Verify firmware'), form, {
						style: 'confirm',
						confirm: function() {
							var keep = form[2].find("input[name='keep']").prop("checked");
							self.startUpgrade(keep).then(function() {
								L.session.stopHeartbeat();
								L.ui.reconnect();
							});
						}
					}
				);
			} else {
				L.ui.dialog(
					L.tr('Invalid image'), [
						$('<p />').text(L.tr('The uploaded image file does not contain a supported format. Make sure that you choose the generic image format for your platform.'))
					], {
						style: 'close',
						close: function() {
							self.cleanUpgrade().then(function() {
								L.ui.dialog(false);
							});
						}
					}
				);
			}
		});
	},

	handleBackupUpload: function() {
		var self = this;
		L.ui.upload(
			L.tr('Backup restore'),
			L.tr('Select the backup archive to restore and click "%s" to proceed.').format(L.tr('Ok')), {
				filename: '/tmp/backup.tar.gz',
				success: function(info) {
					self.handleBackupVerify(info);
				}
			}
		);
	},

	handleBackupVerify: function(info) {
		var self = this;
		L.ui.dialog(
			L.tr('Backup restore'), [
				$('<p />').text(L.tr('The backup archive was uploaded completely. Please verify the checksum and file size below, then click "%s" to restore the archive.').format(L.tr('Ok'))),
				$('<ul />')
					.append($('<li />')
						.append($('<strong />').text(L.tr('Checksum') + ': '))
						.append(info.checksum))
					.append($('<li />')
						.append($('<strong />').text(L.tr('Size') + ': '))
						.append('%1024mB'.format(info.size)))
			], {
				style: 'confirm',
				confirm: function() {
					self.handleBackupRestore();
				}
			}
		);
	},

	handleBackupRestore: function() {
		var self = this;
		self.restoreBackup().then(function(res) {
			if (res.code == 0) {
				L.ui.dialog(
					L.tr('Backup restore'), [
						$('<p />').text(L.tr('The backup was successfully restored, it is advised to reboot the system now in order to apply all configuration changes.')),
						$('<input />')
							.addClass('cbi-button')
							.attr('type', 'button')
							.attr('value', L.tr('Reboot system'))
							.click(function() {
								L.system.performReboot().then(function(){
									L.ui.reconnect(L.tr('Device rebooting...'));
								});
							})
					], {
						style: 'close',
						close: function() {
							self.cleanBackup().then(function() {
								L.ui.dialog(false);
							});
						}
					}
				);
			} else {
				L.ui.dialog(
					L.tr('Backup restore'), [
						$('<p />').text(L.tr('Backup restoration failed, Make sure that you choose the file format for your platform.')),
					], {
						style: 'close',
						close: function() {
							self.cleanBackup().then(function() {
								L.ui.dialog(false);
							});
						}
					}
				);
			}
		});
	},

	handleBackupDownload: function() {
		var form = $('#btn_backup').parent();

		form.find('[name=sessionid]').val(L.globals.sid);
		form.submit();
	},

	handleReset: function() {
		var self = this;
		L.ui.dialog(L.tr('Really reset all changes?'), L.tr('This will reset the system to its initial configuration, all changes made since the initial flash will be lost!'), {
			style: 'confirm',
			confirm: function() {
				self.startReset().then(L.ui.reconnect);
			}
		});
	},

	ota_upgrade: function () {
		var self = this;
		self.getVersion().then(function (results) {
			var description = results.release.description;
			description = description.split(' ');
			var version = description[2];
			self.callSet('basic_setting', 'ota', { 'version': version });
			self.callCommit('basic_setting');
			self.get_server_addr().then(function (value) {
				L.file.exec("cat",["/sys/devices/platform/factory-read/rom_type"]).then(function(result){
					var romtype_tmp = result.stdout.split("");
					var romtype = romtype_tmp[2] + romtype_tmp[3];
					var data = { };
					self.callSet('basic_setting', 'ota', { 'romtype': romtype});
					self.callCommit('basic_setting');
					if (value.cloudtype == '0')
						var cloud_code_url = "https://" + value.server_addr + "/lookImgVersion";
					else
						var cloud_code_url = "https://192.168.1.12:8090/v1/lookImgVersion";
					data.env = value.env;
					data.version = version;
					data.imagetype = value.imagetype;
					data.romtype = romtype;
					data.chip = value.chip;
					console.log(data);
					var data_json = JSON.stringify(data);
					$.ajax({
						method: "POST",
						url: cloud_code_url,
						contentType: "application/json",
						data: data_json,
						processData: false,
						success: function (data_get) {
							setTimeout(function (data_get) {
								var data_parse = JSON.parse(data_get);
								if (data_parse.data != "null") {
									var remote_info = data_parse.data.updateToVersion;
									var force = remote_info.firce;
									var otaversion = remote_info.version;
									var download_type = "&downloaderType=1";
									var download_id = "&sn=ffffffffff";
									var info = {};
									info.size = remote_info.size;
									info.url = remote_info.path + download_type + download_id;
									info.checksum = remote_info.checksum;
									info.otaversion = otaversion;
									var info_json = JSON.stringify(info);
									self.write("/tmp/ota_info", info_json);
									self.ota_download();
								}
								else {
									self.log_write('The version or the romtype is wrong');
									L.ui.dialog(
										L.tr(''), [
										$('<p />').text(L.tr('ota version get failed.')),
									],
										{
											style: 'close',
											close: function () {
												self.cleanBackup().then(function () {
													L.ui.dialog(false);
												});
											}
										}
									);
								}
							}, 25000);
						},
						error: function () {
							self.log_write('Failed to connect to cloud.siflower.cn port 443: Connection refused');
							L.ui.dialog(
								L.tr(''), [
								$('<p />').text(L.tr('ota version get failed.')),
							],
								{
									style: 'close',
									close: function () {
										self.cleanBackup().then(function () {
											L.ui.dialog(false);
										});
									}
								}
							);
						}
					});
				});
			});
		})
	},

	ota_download:function () {
		var self = this;
		var image_tmp = "/tmp/firmware.bin"
		L.file.read('/tmp/ota_info').then(function(data){
			if(data != undefined){
				self.log_write('ota file is downloading now');
				var remote_info = JSON.parse(data);
				var url = remote_info.url;
				var ota_checksum = remote_info.checksum;
				L.file.exec('curl',['-k','-m','120','-o',image_tmp,url]).then(function(data){
					if(data.code != 0)
					{
						self.log_write('ota file download failed');
						L.ui.dialog(
							L.tr(''), [
								$('<p />').text(L.tr('ota file download failed.')),
							],
							{
									style: 'close',
									close: function () {
										self.cleanBackup().then(function () {
											L.ui.dialog(false);
										});
									}
							}
						);
					}
					else
					{
						self.ota_upgrade_start(ota_checksum);
					}
				});
			}
		});
	},

	ota_upgrade_start: function(ota_checksum) {
		var self = this;
		var image_tmp = "/tmp/firmware.bin";
		self.md5sum(image_tmp).then(function(data){
			L.file.exec('ls',['-l', image_tmp]).then(function(ret){
				var size_tmp =ret.stdout.split('      ');
				size_tmp = size_tmp[1].split(' ');
				var size = Number(size_tmp[1]);
				if(ota_checksum == data.md5)
				{
					var form = [
						$('<p />').text(L.tr('The firmware image was uploaded completely. Please verify the checksum and file size below, then click "%s" to start the flash procedure.').format(L.tr('Ok'))),
						$('<ul />')
							.append($('<li />')
							.append($('<strong />').text(L.tr('Checksum') + ': '))
							.append(ota_checksum))
						.append($('<li />')
							.append($('<strong />').text(L.tr('Size') + ': '))
							.append('%1024mB'.format(size))),
					];
					L.ui.dialog(
						L.tr('Verify firmware'), form, {
							style: 'confirm',
							confirm: function () {
								self.startOtaUpgrade().then(function () {
									L.session.stopHeartbeat();
									L.ui.reconnect();
								});
							}
						}
					);
				}
				else{
					L.ui.dialog(
						L.tr(''), [
						$('<p />').text(L.tr('ota file download failed.')),
					],
						{
							style: 'close',
							close: function () {
								self.cleanBackup().then(function () {
									L.ui.dialog(false);
								});
							}
						}
					);
					self.log_write("the checksum is wrong");
				}
			});
		});
	},

	execute: function() {
		var self = this;

		self.testReset().then(function(reset_avail) {
			if (!reset_avail) {
				$('#btn_reset').prop('disabled', true);
			}

			if (!self.options.acls.backup) {
				$('#btn_restore, #btn_save, textarea').prop('disabled', true);
			}
			else {
				$('#btn_backup').click(function() { self.handleBackupDownload(); });
				$('#btn_restore').click(function() { self.handleBackupUpload(); });
			}

			if (!self.options.acls.upgrade) {
				$('#btn_flash, #btn_reset').prop('disabled', true);
			}
			else {
				$('#btn_flash').click(function() { self.handleFlashUpload(); });
				$('#btn_reset').click(function() { self.handleReset(); });
			}

			$('#btn_ota').click(function(){
				L.ui.dialog(
					L.tr(''), [
					$('<p />').text(L.tr('Are you sure you want to upgrade online? The upgrade process may take three to five minutes, please ensure that the machine will not be powered off.')),
					], {
						style: 'confirm',
						confirm: function () {
							self.cleanBackup().then(function () {
								L.ui.dialog(
									L.tr(''), [
										$('<p />').text(L.tr('ota checking software version'))
									]
								);
								self.ota_upgrade();
							});
						}
					}
				);
			});

			self.callLoad('basic_setting', 'auto_ota', 'enable').then(function (value) {
				if (value == '1') {
					$('#check_ota').prop('checked', 'true');
					$('#checkbox_ota').prop('class', 'checkbox_press').css('margin-top', '-7px');
				}
			});
			$('#check_ota').click(function () {
				if ($('#check_ota').prop('checked')) {
					$('#checkbox_ota').prop('class', 'checkbox_press').css('margin-top', '-7px');
					self.callSet('basic_setting', 'auto_ota', { 'enable': '1' });
					self.callCommit('basic_setting');
				}
				else {
					$('#checkbox_ota').prop('class', 'checkbox').css('margin-top', '20px');
					self.callSet('basic_setting', 'auto_ota', { 'enable': '0' });
					self.callCommit('basic_setting');
				}
				L.ui.dialog(
					L.tr(''), [
						$('<p />').text(L.tr('Set successfully')),
					],
					{
							style: 'close',
							close: function () {
								self.cleanBackup().then(function () {
									L.ui.dialog(false);
								});
							}
					}
				);
			});

			return self.getBackupConfig();
		})
	}
});
