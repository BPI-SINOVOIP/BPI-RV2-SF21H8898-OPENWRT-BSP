'use strict';
'require ui';
'require rpc';
'require uci';
'require view';
'require form';
'require view_iface';
'require dom';
'require poll';
'require fs';
'require network';
'require firewall';

return view.extend({
    load: function () {
        return Promise.all([
            uci.callLoad('easymesh'),
            uci.callLoad('wireless'),
        ]);
    },

    updateConfig: async function() {
        var easymeshPromise = uci.callLoad('easymesh');
        var wirelessPromise = uci.callLoad('wireless');

        try {
            var PromiseResult = await Promise.all([easymeshPromise, wirelessPromise]);
            return PromiseResult;
        } catch (error) {
            console.error(error);
            return null;
        }
    },

    updateCanWps: async function() {
        fs.exec('/bin/sf_easymesh', ["check_wps_enable"])
        var PromiseResult = await this.updateConfig();
        var can_enable_wps = PromiseResult[0]['status'].can_enable_wps;
        if(can_enable_wps == 'false') {
            wpsButton.setAttribute('disabled', 'true');
        } else if(can_enable_wps == 'true') {
            wpsButton.removeAttribute('disabled');
        }
    },

    updateCanSet: async function() {
        var PromiseResult = await this.updateConfig();
        var is_first_load = PromiseResult[0]['status'].is_first_load;
        if(is_first_load == 'false') {
            backhaulSsidInput.setAttribute('disabled', 'true');
            backhaulKeyInput.setAttribute('disabled', 'true');
            deviceRoleSelect.setAttribute('disabled', 'true');
            backhaulFormSelect.setAttribute('disabled', 'true');
        } else if(is_first_load == 'true') {
            backhaulSsidInput.removeAttribute('disabled');
            backhaulKeyInput.removeAttribute('disabled');
            deviceRoleSelect.removeAttribute('disabled');
            backhaulFormSelect.removeAttribute('disabled');
        }
    },

    // updateStatus: async function() {
    //     fs.exec('/bin/sf_easymesh', ["check_wps_enable"])
    //     fs.exec('/bin/sf_easymesh', ["check_conn_enable"])
    //     fs.exec('/bin/sf_easymesh', ["check_can_update_wifi_config"])
    //     var PromiseResult = await this.updateConfig();
    //     var can_enable_wps = PromiseResult[0]['status'].can_enable_wps;
    //     if(can_enable_wps == 'false') {
    //         wpsButton.setAttribute('disabled', 'true');
    //     } else if(can_enable_wps == 'true') {
    //         wpsButton.removeAttribute('disabled');
    //     }
    // },

    bytelen: function(x) {
        return new Blob([x]).size;
    },

    checkInput: async function() {
        var fhslen = this.bytelen(fronthaulSsidInput.value);
        var fhklen = this.bytelen(fronthaulKeyInput.value);
        var bhblen = this.bytelen(backhaulSsidInput.value);
        var bhklen = this.bytelen(backhaulKeyInput.value);
        if(fhslen > 32 || bhblen > 32) {
            ui.addNotification(null, E('p', [ _('Ssid is more than 32 chars') ]));
            return -1
        }
        if(fhklen < 8 || fhklen > 63 || bhklen < 8 || bhklen > 63) {
            ui.addNotification(null, E('p', [ _('Key must between 8 and 63 chars') ]));
            return -1
        }
        return 0;
    },

    forbidOpt: function() {
        enableEasymeshCheckbox.setAttribute('disabled', 'true');
        setButton.setAttribute('disabled', 'true');
        wpsButton.setAttribute('disabled', 'true');
    },

    allowOpt: function() {
        enableEasymeshCheckbox.removeAttribute('disabled');
        setButton.removeAttribute('disabled');
        this.updateCanWps();
    },

    handleEnableEasymeshChange: async function (event) {
        this.forbidOpt();
        var opt = enableEasymeshCheckbox.checked ? "enable" : "disable"
        fs.exec('/bin/sf_easymesh', [opt])
        if (opt == 'enable') {
            var status = await this.timerWaitStatus(20, 'enable', '1');
        } else {
            var status = await this.timerWaitStatus(20, 'enable', '0');
        }
        if(!status) {
            console.log(opt + ' fail, please retry')
            this.allowOpt();
        } else {
            var PromiseResult = await this.updateConfig();
            var easymesh = PromiseResult[0]
            var wireless = PromiseResult[1]['default_radio0']
            fronthaulSsidInput.value = wireless['ssid']
            fronthaulKeyInput.value = wireless['key']
            encryptionSelect.value = wireless['encryption']
            backhaulSsidInput.value = wireless['multi_ap_backhaul_ssid']
            backhaulKeyInput.value = wireless['multi_ap_backhaul_key']
            deviceRoleSelect.value = easymesh['status']['management_mode']

            if (opt == "enable") {
                fronthaulSsid.style.display = 'flex';
                fronthaulKey.style.display = 'flex';
                encryption.style.display = 'flex';
                deviceRole.style.display = 'flex';
                if (deviceRoleSelect.value === 'controller') {
                    backhaulForm.style.display = 'none';
                    backhaulSsid.style.display = 'flex';
                    backhaulKey.style.display = 'flex';
                } else if (deviceRoleSelect.value === 'agent') {
                    backhaulForm.style.display = 'flex';
                    backhaulSsid.style.display = 'none';
                    backhaulKey.style.display = 'none';
                }
            } else {
                fronthaulSsid.style.display = 'none';
                fronthaulKey.style.display = 'none';
                encryption.style.display = 'none';
                backhaulSsid.style.display = 'none';
                backhaulKey.style.display = 'none';;
                deviceRole.style.display = 'none';
                backhaulForm.style.display = 'none';
            }
            this.updateCanSet();
            this.allowOpt();
        }
    },

    handledeviceRoleSelectChange: function (event) {
        if (deviceRoleSelect.value === 'controller') {
            fronthaulSsidInput.disabled = false;
            fronthaulKeyInput.disabled = false;
            encryptionSelect.disabled = false;
            backhaulSsid.style.display = 'flex';
            backhaulKey.style.display = 'flex';
            backhaulForm.style.display = 'none';
        } else if (deviceRoleSelect.value === 'agent') {
            fronthaulSsidInput.disabled = true;
            fronthaulKeyInput.disabled = true;
            encryptionSelect.disabled = true;
            backhaulSsid.style.display = 'none';
            backhaulKey.style.display = 'none';
            backhaulForm.style.display = 'flex';
        }
    },

    handleWPS: async function () {
        this.forbidOpt();
        var PromiseResult = await this.updateConfig();
        var enbale = PromiseResult[0]['status'].enable;
        if(enbale == '0') {
            ui.addNotification(null, E('p', [ _('Easymesh not enable') ]));
            this.allowOpt();
            return;
        }

        var can_enable_conn = PromiseResult[0]['status'].can_enable_conn;
        if(can_enable_conn == 'false') {
            ui.addNotification(null, E('p', [ _('Config not load') ]));
            this.allowOpt();
            return;
        }

        var can_enable_wps = await this.timerWaitStatus(20, 'can_enable_wps', 'true');
        if(!can_enable_wps) {
            ui.addNotification(null, E('p', [ _('Can not wps') ]));
            return;
        } else {
            fs.exec('/bin/sf_easymesh', ["enable_wps"])
        }
        ui.addNotification(null, E('p', [ _('WPSing, please wait 2 minutes') ]));
        await this.timerWaitStatus(120, 'null', 'null');
        this.allowOpt();
    },

    handleSetMode: async function () {
        var PromiseResult = await this.updateConfig();
        var enbale = PromiseResult[0]['status'].enable;
        var is_set_mode = PromiseResult[0]['status'].is_set_mode;
        if(enbale == '0') {
            ui.addNotification(null, E('p', [ _('Easymesh not enable') ]));
            return -1;
        } else if(is_set_mode == 'true') {
            return;
        } else {
            var role = document.getElementById('deviceRoleSelect').value;
            fs.exec('/bin/sf_easymesh', ["set_management_mode", role])
        }
    },

    handleSetControllerConfig: async function () {
        var PromiseResult = await this.updateConfig();
        var role = document.getElementById('deviceRoleSelect').value;

        if(PromiseResult[0]['status'].enable == '0') {
            return;
        }

        var role = document.getElementById('deviceRoleSelect').value;
        if(role == "controller") {
            var fhs = document.getElementById('fronthaulSsidInput').value;
            var fhk = document.getElementById('fronthaulKeyInput').value;
            var bhb = document.getElementById('backhaulSsidInput').value;
            var bhk = document.getElementById('backhaulKeyInput').value;
            var encr = document.getElementById('encryptionSelect').value;
            var is_first_load = PromiseResult[0]['status'].is_first_load;
            if(is_first_load == 'true') {
                fs.exec('/bin/sf_easymesh', ["set_controller_config", fhs, fhk, encr, bhb, bhk]);
                fs.exec('/bin/sf_easymesh', ["load_for_web"]);
            } else {
                var can_update_wifi_config = await this.timerWaitStatus(20, 'can_update_wifi_config', 'true');
                if(can_update_wifi_config) {
                    fs.exec('/bin/sf_easymesh', ["set_wifi_config", fhs, fhk, encr])
                    fs.exec('/bin/sf_easymesh', ["update_wifi_config"])
                }
            }
        } else {
            var form = document.getElementById('backhaulFormSelect').value;
            fs.exec('/bin/sf_easymesh', ["set_backhaul_form", form]);
            fs.exec('/bin/sf_easymesh', ["load_for_web"])
        }
    },

    handleSet: async function () {
        this.forbidOpt()
        if(await this.handleSetMode() == -1){
            this.allowOpt();
            return;
        }

        if(await this.checkInput() == -1){
            this.allowOpt();
            return;
        }

        var is_set_mode = await this.timerWaitStatus(20, 'is_set_mode', 'true');
        if(!is_set_mode) {
            ui.addNotification(null, E('p', [ _('Set mode fail') ]));
        } else {
            await this.handleSetControllerConfig();
        }
        var can_enable_conn = await this.timerWaitStatus(20, 'can_enable_conn', 'true');
        if(!can_enable_conn) {
            ui.addNotification(null, E('p', [ _('Easymesh driver load fail') ]));
        }
        this.updateCanSet();
        this.allowOpt()
    },

    timerWaitStatus : async function(interaval, status, flag) {
        try {
            var detectionResult = await new Promise(function(resolve, reject) {
                var timerCount = 0;
                var intervalId = setInterval(async function() {
                    if(status == 'null') {
                        timerCount += 2;
                        if (timerCount >= interaval) {
                            console.log('Timer reached');
                            clearInterval(intervalId);
                            resolve(1);
                        }
                    } else {
                        var easymeshPromise = uci.callLoad('easymesh');
                        var wirelessPromise = uci.callLoad('wireless');
                        var promiseList = [easymeshPromise, wirelessPromise]
                        if(status == 'can_enable_conn') {
                            var check_conn_enable = fs.exec('/bin/sf_easymesh', ["check_conn_enable"])
                            promiseList.push(check_conn_enable)
                        } else if(status == 'can_enable_wps') {
                            var check_wps_enable = fs.exec('/bin/sf_easymesh', ["check_wps_enable"])
                            promiseList.push(check_wps_enable)
                        } else if(status == 'can_update_wifi_config') {
                            var check_can_update_wifi_config = fs.exec('/bin/sf_easymesh', ["check_can_update_wifi_config"])
                            promiseList.push(check_can_update_wifi_config)
                        }
                        var PromiseResult = await Promise.all(promiseList);
                        var result = PromiseResult[0]['status'][status]
                        if (result == flag) {
                            clearInterval(intervalId);
                            console.log('Detected ' + status);
                            resolve(1);
                        }

                        timerCount += 2;
                        if (timerCount >= interaval) {
                            console.log('Timer reached');
                            clearInterval(intervalId);
                            resolve(0);
                        }
                    }
                }, 2000);
            });
            return detectionResult;
        } catch (error) {
            console.log(error)
            return null;
        }
    },

    render: async function (data) {
        var config = data[0]['config'];
        var status = data[0]['status'];
        var wireless = data[1]['default_radio0']

        // var statusLabel = E('label', { 'id': 'statusLabel' }, _('Easymesh status'));
        // var statusInfo = E('label', { 'id': 'statusInfo' }, _('Has not enable'));

        var enableEasymeshCheckbox = E('input', {
            'type': 'checkbox',
            'checked': status['enable'] == "1" ? '' : null,
            'id': 'enableEasymeshCheckbox',
            change: this.handleEnableEasymeshChange.bind(this)
        });

        var enableEasymeshlabel = E('label', { 'id': 'enableEasymeshlabel' }, _('Enable Easymesh'));

        var fronthaulSsidInput = E('input', {
            'type': 'text',
            'value': 'SiWiFi-EasyMesh-F',
            'id': 'fronthaulSsidInput',
        }, _('Fronthaul Ssid'));

        var fronthaulSsidlabel = E('label', { 'id': 'fronthaulSsidlabel' }, _('Fronthaul Ssid'));

        var fronthaulKeyInput = E('input', {
            'type': 'text',
            'value': '12345678',
            'id': 'fronthaulKeyInput',
        }, _('Fronthaul Key'));
        var fronthaulKeylabel = E('label', { 'id': 'fronthaulKeylabel' }, _('Fronthaul Key'));

        var encryptionSelect = E('select', {
            'id': 'encryptionSelect',
        },  [
                E('option', { 'value': 'psk2'       }, [_('WPA2-PSK')]),
                E('option', { 'value': 'psk-mixed'  }, [_('WPA-PSK/WPA2-PSK Mixed')]),
                E('option', { 'value': 'sae'        }, [_('WPA3-SAE')]),
                E('option', { 'value': 'sae-mixed', selected: true }, [_('WPA2-PSK/WPA3-SAE Mixed')]),
                E('option', { 'value': 'none'       }, [_('No Encryption')]),
            ]);
        var encryptionlabel = E('label', { 'id': 'encryptionlabel' }, _('encryption'));

        var backhaulSsidInput = E('input', {
            'type': 'text',
            'value': 'SiWiFi-EasyMesh-B',
            'id': 'backhaulSsidInput',
        }, _('Backhaul Ssid'));
        var backhaulSsidlabel = E('label', { 'id': 'backhaulSsidlabel' }, _('Backhaul Ssid'));

        var backhaulKeyInput = E('input', {
            'type': 'text',
            'value': '12345678',
            'id': 'backhaulKeyInput',
        }, _('Backhaul Key'));
        var backhaulKeylabel = E('label', { 'id': 'backhaulKeylabel' }, _('Backhaul Key'));

        var deviceRoleSelect = E('select', {
            'id': 'deviceRoleSelect',
            change: this.handledeviceRoleSelectChange.bind(this)
        },  [
                E('option', { 'value': 'controller', selected: true }, [_('Controller')]),
                E('option', { 'value': 'agent' }, [_('Agent')])
            ]);
        var deviceRolelabel = E('label', { 'id': 'deviceRolelabel' }, _('DeviceRole'));

        var backhaulFormSelect = E('select', { 'id': 'backhaulFormSelect' }, [
            E('option', { 'value': 'wireless-24' }, ['wireless-24']),
            E('option', { 'value': 'wireless-5' }, ['wireless-5']),
            E('option', { 'value': 'wired' }, [_('Wired')])
        ]);

        var backhaulFormlabel = E('label', { 'id': 'backhaulFormlabel' }, _('Backhaul Form'))

        var wpsButton = E('button', {
            'class': 'btn cbi-button-action',
            'id': 'wpsButton',
            'click': this.handleWPS.bind(this)
        }, _('WPS'));

        var setButton = E('button', {
            'class': 'btn cbi-button-action',
            'id': 'setButton',
            'click': this.handleSet.bind(this)
        }, _('Load Config'));

        var fronthaulSsid = E('div', { id: 'fronthaulSsid', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [fronthaulSsidlabel]), fronthaulSsidInput])

        var fronthaulKey = E('div', { id: 'fronthaulKey', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [fronthaulKeylabel]), fronthaulKeyInput])

        var encryption = E('div', { id: 'encryption', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [encryptionlabel]), encryptionSelect])

        var deviceRole = E('div', { id: 'deviceRole', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [deviceRolelabel]), deviceRoleSelect])

        var backhaulSsid = E('div', { id: 'backhaulSsid', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [backhaulSsidlabel]), backhaulSsidInput])

        var backhaulKey = E('div', { id: 'backhaulKey', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [backhaulKeylabel]), backhaulKeyInput])

        var backhaulForm = E('div', { id: 'backhaulForm', style: 'align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                                [E('div', { style: 'width: 200px' }, [backhaulFormlabel]), backhaulFormSelect])

        var component = E([], [
            E('h2', {}, [_('Easymesh Basic')]),
            // E('div', { id: 'easymeshStatus', style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
            //     [E('div', { style: 'width: 200px' }, [statusLabel]), statusInfo]
            // ),
            E('div', { id: 'enableEasymesh', style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                [E('div', { style: 'width: 200px' }, [enableEasymeshlabel]), enableEasymeshCheckbox]
            ),
            fronthaulSsid,
            fronthaulKey,
            encryption,
            deviceRole,
            backhaulSsid,
            backhaulKey,
            backhaulForm,
            E('div', { style: 'display: flex; align-items: center; margin-bottom: 10px; margin-top: 10px; width: 400px;' },
                [wpsButton, setButton]
            )
        ]);

        fronthaulSsid.style.display = status['enable'] == "1" ? 'flex' : 'none';
        fronthaulKey.style.display = status['enable'] == "1" ? 'flex' : 'none';
        encryption.style.display = status['enable'] == "1" ? 'flex' : 'none';
        deviceRole.style.display = status['enable'] == "1" ? 'flex' : 'none';
        if (status['enable'] == "1" && status['management_mode'] == 'controller') {
            backhaulForm.style.display = 'none';
            backhaulSsid.style.display = 'flex';
            backhaulKey.style.display = 'flex';
        } else if (status['enable'] == "1" && status['management_mode'] == 'agent') {
            backhaulForm.style.display = 'flex';
            backhaulSsid.style.display = 'none';
            backhaulKey.style.display = 'none';
        } else {
            backhaulForm.style.display = 'none';
            backhaulSsid.style.display = 'none';
            backhaulKey.style.display = 'none';
        }

        var PromiseResult = await this.updateConfig();
        var easymesh = PromiseResult[0]
        var wireless = PromiseResult[1]['default_radio0']
        fronthaulSsidInput.value = wireless['ssid']
        fronthaulKeyInput.value = wireless['key']
        encryptionSelect.value = wireless['encryption']
        backhaulSsidInput.value = wireless['multi_ap_backhaul_ssid']
        backhaulKeyInput.value = wireless['multi_ap_backhaul_key']
        deviceRoleSelect.value = easymesh['status']['management_mode']

        this.updateCanSet();
        this.updateCanWps();
        return component;
    },

    handleSaveApply: null,
	handleSave: null,
	handleReset: null
});