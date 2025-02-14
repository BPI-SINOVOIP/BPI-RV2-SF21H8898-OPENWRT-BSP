/*
 * rpcd - UBUS RPC server
 *
 *   Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define _GNU_SOURCE /* crypt() */

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <signal.h>
#include <glob.h>
#include <libubox/blobmsg_json.h>
#include <libubox/avl-cmp.h>
#include <libubus.h>
#include <uci.h>

#include <rpcd/plugin.h>
#include <stdio.h>

static const struct rpc_daemon_ops *ops;

static struct blob_buf buf;
static struct uci_context *cursor;
enum {
	RPC_D_DATA,
	__RPC_D_MAX
};

static const struct blobmsg_policy rpc_data_policy[__RPC_D_MAX] = {
	[RPC_D_DATA]   = { .name = "data",  .type = BLOBMSG_TYPE_STRING },
};

static int
rpc_web_get_test(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	char lan_ip[16] = "";
	struct uci_package *p;
	struct uci_ptr ptr = {
		.package = "network",
		.section = "lan",
		.option  = "ipaddr"
	};

	if (uci_load(cursor, ptr.package, &p) || !p)
		goto out;

	uci_lookup_ptr(cursor, &ptr, NULL, true);

	if (ptr.o && ptr.o->type == UCI_TYPE_STRING)
		strcpy(lan_ip, ptr.o->v.string);

	uci_unload(cursor, p);

out:
	blob_buf_init(&buf, 0);
	blobmsg_add_string(&buf, "ipaddr", lan_ip);
	ubus_send_reply(ctx, req, buf.head);
	return 0;
}

static int
rpc_web_set_test(struct ubus_context *ctx, struct ubus_object *obj,
                   struct ubus_request_data *req, const char *method,
                   struct blob_attr *msg)
{
	struct blob_attr *tb[__RPC_D_MAX];

	struct uci_package *p;
	struct uci_ptr ptr = {
		.package = "network",
		.section = "lan",
		.option  = "ipaddr",
		.value  = "192.168.6.1"
	};

	printf("====value1===%s\n", ptr.value);
	blobmsg_parse(rpc_data_policy, __RPC_D_MAX, tb,
	              blob_data(msg), blob_len(msg));

	if (!tb[RPC_D_DATA] || blobmsg_data_len(tb[RPC_D_DATA]) >= 128)
		return UBUS_STATUS_INVALID_ARGUMENT;

	ptr.value = blobmsg_data(tb[RPC_D_DATA]);
	printf("====value2===%s\n", ptr.value);

	uci_load(cursor, ptr.package, &p);
	uci_set(cursor, &ptr);
	uci_save(cursor, p);
	uci_commit(cursor, &p, true);
	uci_unload(cursor, p);

	return 0;
}

enum {
	RPC_S_DEVICE,
	RPC_S_SSID,
	RPC_S_BSSID,
	RPC_S_ENCRYPTION,
	RPC_S_KEY,
	RPC_S_CHANNEL,
	RPC_S_OC,
	__RPC_S_MAX
};

static const struct blobmsg_policy rpc_wificonn_policy[__RPC_S_MAX] = {
	[RPC_S_DEVICE]     = {.name = "device",     .type = BLOBMSG_TYPE_STRING},
	[RPC_S_SSID]       = {.name = "ssid",       .type = BLOBMSG_TYPE_STRING},
	[RPC_S_BSSID]      = {.name = "bssid",      .type = BLOBMSG_TYPE_STRING},
	[RPC_S_ENCRYPTION] = {.name = "encryption", .type = BLOBMSG_TYPE_STRING},
	[RPC_S_KEY]        = {.name = "key",        .type = BLOBMSG_TYPE_STRING},
	[RPC_S_CHANNEL]    = {.name = "channel",    .type = BLOBMSG_TYPE_STRING},
	[RPC_S_OC]         = {.name = "oc",         .type = BLOBMSG_TYPE_STRING}
};

enum {
	RPC_SN_SSID,
	RPC_SN_ENCRYPTION,
	RPC_SN_KEY,
    RPC_SN_FLAG,
	__RPC_SN_MAX
};

static const struct blobmsg_policy rpc_wificonnnew_policy[__RPC_SN_MAX] = {
	[RPC_SN_SSID]       = {.name = "ssid",       .type = BLOBMSG_TYPE_STRING},
	[RPC_SN_ENCRYPTION] = {.name = "encryption", .type = BLOBMSG_TYPE_STRING},
	[RPC_SN_KEY]        = {.name = "key",        .type = BLOBMSG_TYPE_STRING},
    [RPC_SN_FLAG]        = {.name = "flag",        .type = BLOBMSG_TYPE_STRING},
};

enum {
	RPC_I_DEVICE,
	RPC_I_SSID,
	RPC_I_ENCRYPTION,
	RPC_I_KEY,
	RPC_I_STATUS,
	__RPC_I_MAX
};

static const struct blobmsg_policy rpc_wifiself_policy[__RPC_I_MAX] = {
	[RPC_I_DEVICE]     = {.name = "device",     .type = BLOBMSG_TYPE_STRING},
	[RPC_I_SSID]       = {.name = "ssid",       .type = BLOBMSG_TYPE_STRING},
	[RPC_I_ENCRYPTION] = {.name = "encryption", .type = BLOBMSG_TYPE_STRING},
	[RPC_I_KEY]        = {.name = "key",        .type = BLOBMSG_TYPE_STRING},
	[RPC_I_STATUS]     = {.name = "status",     .type = BLOBMSG_TYPE_STRING}
};
enum {
	RPC_T_RADIO,
	__RPC_T_MAX
};

static const struct blobmsg_policy rpc_status_policy[__RPC_T_MAX] = {
	[RPC_T_RADIO] = {.name = "radio", .type = BLOBMSG_TYPE_STRING},
};


enum {
	RPC_W_IFNAME,
	RPC_W_IP,
	__RPC_W_MAX
};

static const struct blobmsg_policy rpc_wds_policy[__RPC_W_MAX] = {
	[RPC_W_IFNAME] = {.name = "ifname", .type = BLOBMSG_TYPE_STRING},
	[RPC_W_IP] = {.name = "ip", .type = BLOBMSG_TYPE_STRING},
};

enum {
	RPC_O_DEV,
	RPC_O_OC,
	__RPC_O_MAX
};

static const struct blobmsg_policy rpc_offwds_policy[__RPC_O_MAX] = {
	[RPC_O_DEV] = {.name = "device", .type = BLOBMSG_TYPE_STRING},
	[RPC_O_OC]  = {.name = "oc",     .type = BLOBMSG_TYPE_STRING},

};

static char* get(char* package, char* section, char* option) {
	char *value = (char*)malloc(100*sizeof(char));
	struct uci_context *now = uci_alloc_context();
	struct uci_package *pack = NULL;
	struct uci_ptr ptr = {
		.package = package,
		.section = section,
		.option = option
	};
	uci_load(now, ptr.package, &pack);
	uci_lookup_ptr(cursor, &ptr, NULL, true);
	if (ptr.o && ptr.o->type == UCI_TYPE_STRING)
		strcpy(value, ptr.o->v.string);
	uci_unload(now, pack);
	uci_free_context(now);
	now = NULL;
	return value;
}

/*static int set_ano(char* package,char* option,char* value,int pos){
	struct uci_context *now = uci_alloc_context();
	struct uci_package *pack = NULL;
	struct uci_element *e;
	struct uci_ptr ptr = {
		.package = package,
		.option = option,
		.value = value
	};
	uci_load(now, ptr.package, &pack);
	int i = 1;
	uci_foreach_element(&pack->sections, e) {
		struct uci_section *s = uci_to_section(e);
		ptr.section = s->e.name;
		if (i == pos)
			break;
		else
			i++;
	}
	uci_set(now, &ptr);
	uci_save(now, pack);
	uci_commit(now, &pack, true);
	uci_unload(now, pack);
	uci_free_context(now);
	now = NULL;
	return 0;
}*/

static int set(char* package, char* section, char* option, char* value) {
	struct uci_context *now = uci_alloc_context();
	struct uci_package *pack = NULL;
	struct uci_ptr ptr = {
		.package = package,
		.section = section,
		.option = option,
		.value = value
	};
	uci_load(now, ptr.package, &pack);
	uci_set(now, &ptr);
	uci_save(now, pack);
	uci_commit(now, &pack, true);
	uci_unload(now, pack);
	uci_free_context(now);
	now = NULL;
	return 0;
}

static int del(char* package, char* section, char* option) {
	struct uci_context *now = uci_alloc_context();
	struct uci_package *pack = NULL;
	struct uci_ptr ptr = {
		.package = package,
		.section = section,
		.option = option
	};
	uci_load(now, ptr.package, &pack);
	uci_delete(now, &ptr);
	uci_save(now, pack);
	uci_commit(now, &pack, false);
	uci_unload(now, pack);
	uci_free_context(now);
	now = NULL;
	return 0;
}

static int rpc_web_wireless_wds_wifi_self(struct ubus_context *ctx,
										  struct ubus_object *obj,
										  struct ubus_request_data *req,
										  const char *method,
										  struct blob_attr *msg) {
	struct blob_attr *tb[__RPC_I_MAX];
	blobmsg_parse(rpc_wifiself_policy, __RPC_I_MAX, tb, blob_data(msg),
				  blob_len(msg));

	char wl[] = "wireless", nw[] = "network", s[32];

	if (!strcmp(blobmsg_data(tb[RPC_I_STATUS]), "true")) {
		if (!strcmp(blobmsg_data(tb[RPC_I_DEVICE]), "wlan0-1")) {
			strcpy(s, "default_radio0");
		} else {
			strcpy(s, "default_radio1");
		}
		set(wl, s, "ssid", blobmsg_data(tb[RPC_I_SSID]));
		set(wl, s, "encryption", blobmsg_data(tb[RPC_I_ENCRYPTION]));
		if (strcmp(blobmsg_data(tb[RPC_I_ENCRYPTION]), "none")) {
			set(wl, s, "key", blobmsg_data(tb[RPC_I_KEY]));
		}
		system("wifi reload");
	} else if (!strcmp(blobmsg_data(tb[RPC_I_STATUS]), "false")) {
		del(nw, "wwan", NULL);
		del(wl, "wds", NULL);
		system("/etc/init.d/network restart");
	}
	return 0;
}

static int rpc_web_wireless_wds_wifi_connect(struct ubus_context *ctx,
											 struct ubus_object *obj,
											 struct ubus_request_data *req,
											 const char *method,
											 struct blob_attr *msg) {
	struct blob_attr *tb[__RPC_S_MAX];
	blobmsg_parse(rpc_wificonn_policy, __RPC_S_MAX, tb, blob_data(msg),
				  blob_len(msg));

	char wl[] = "wireless", s[32] = "wds", v[256] = "";
	set(wl, s, NULL, "wifi-iface");
	if (!strcmp(blobmsg_data(tb[RPC_S_DEVICE]), "radio0")) {
		set(wl, s, "device", "radio0");
		set(wl, s, "ifname", "wlan0-1");
		set(wl, "radio0", "channel", blobmsg_data(tb[RPC_S_CHANNEL]));
		set(wl, "radio0", "origin_channel", blobmsg_data(tb[RPC_S_OC]));

	} else if (!strcmp(blobmsg_data(tb[RPC_S_DEVICE]), "radio1")) {
		set(wl, s, "device", "radio1");
		set(wl, s, "ifname", "wlan1-1");
		set(wl, "radio1", "channel", blobmsg_data(tb[RPC_S_CHANNEL]));
		set(wl, "radio1", "origin_channel", blobmsg_data(tb[RPC_S_OC]));
	}
	set(wl, s, "ssid", blobmsg_data(tb[RPC_S_SSID]));
	set(wl, s, "bssid", blobmsg_data(tb[RPC_S_BSSID]));
	if (!strcmp(blobmsg_data(tb[RPC_S_ENCRYPTION]), "WPA2(PSK)")) {
		strcpy(v, "psk2+ccmp");
	} else if (!strcmp(blobmsg_data(tb[RPC_S_ENCRYPTION]), "Open")) {
		strcpy(v, "none");
	} else if (!strcmp(blobmsg_data(tb[RPC_S_ENCRYPTION]), "WPA(PSK)")) {
		strcpy(v, "psk");
	} else if (!strcmp(blobmsg_data(tb[RPC_S_ENCRYPTION]), "WPA/WPA2(PSK) MIXED")) {
		strcpy(v, "psk-mixed");
	}
	set(wl, s, "encryption", v);
	if (strcmp(v, "none")) {
		set(wl, s, "key", blobmsg_data(tb[RPC_S_KEY]));
	}
	set(wl, s, "mode", "sta");
	set(wl, s, "network", "lan");
	system("wifi");

	return 0;
}

static int rpc_web_wireless_wds_wifi_connect_new(struct ubus_context *ctx,
											 struct ubus_object *obj,
											 struct ubus_request_data *req,
											 const char *method,
											 struct blob_attr *msg) {
	struct blob_attr *tb[__RPC_SN_MAX];
	blobmsg_parse(rpc_wificonnnew_policy, __RPC_SN_MAX, tb, blob_data(msg),
				  blob_len(msg));

	char wl[] = "wireless", s[32] = "wds", v[256] = "";
	set(wl, s, NULL, "wifi-iface");
	if (!strcmp(blobmsg_data(tb[RPC_SN_FLAG]), "2.4G")) {
		set(wl, s, "device", "radio0");
		set(wl, s, "ifname", "wlan0-1");

	} else if (!strcmp(blobmsg_data(tb[RPC_SN_FLAG]), "5G")) {
		set(wl, s, "device", "radio1");
		set(wl, s, "ifname", "wlan1-1");
	}
	set(wl, s, "ssid", blobmsg_data(tb[RPC_SN_SSID]));

	if (!strcmp(blobmsg_data(tb[RPC_SN_ENCRYPTION]), "WPA2(PSK)")) {
		strcpy(v, "psk2+ccmp");
	} else if (!strcmp(blobmsg_data(tb[RPC_SN_ENCRYPTION]), "Open")) {
		strcpy(v, "none");
	} else if (!strcmp(blobmsg_data(tb[RPC_SN_ENCRYPTION]), "WPA(PSK)")) {
		strcpy(v, "psk");
	} else if (!strcmp(blobmsg_data(tb[RPC_SN_ENCRYPTION]), "WPA/WPA2(PSK) MIXED")) {
		strcpy(v, "psk-mixed");
	}
	set(wl, s, "encryption", v);
	if (strcmp(v, "none")) {
		set(wl, s, "key", blobmsg_data(tb[RPC_SN_KEY]));
	}
	set(wl, s, "mode", "sta");
	set(wl, s, "network", "lan");
	system("wifi");

	return 0;
}

static int rpc_web_wireless_wds_wifi_connect_status(struct ubus_context *ctx,
									   struct ubus_object *obj,
									   struct ubus_request_data *req,
									   const char *method,
									   struct blob_attr *msg) {
	FILE *p_file = NULL;
	struct blob_attr *tb[__RPC_T_MAX];
	blobmsg_parse(rpc_status_policy, __RPC_T_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[RPC_T_RADIO] || blobmsg_data_len(tb[RPC_T_RADIO]) >= 128)
		return UBUS_STATUS_INVALID_ARGUMENT;
	char *ifname = blobmsg_data(tb[RPC_T_RADIO]);
	char cmd[100];
	char ssid[64];
	memset(cmd, 0, sizeof(cmd));
	memset(ssid, 0, sizeof(ssid));
	sprintf(cmd, "iwinfo | grep %s%s", ifname, " -C 3 | grep 'Mode' | awk -F ' ' '{print$2}' | tr -d '\n'");
	p_file = popen(cmd, "r");
	if (p_file) {
		while (fgets(ssid, 64, p_file) != NULL) {}
		pclose(p_file);
	}

	blob_buf_init(&buf, 0);
	blobmsg_add_string(&buf, "mode", ssid);
	blobmsg_add_string(&buf, "ifname", ifname);
	ubus_send_reply(ctx, req, buf.head);

	return 0;
}

static int rpc_web_wireless_wds_enable(struct ubus_context *ctx,
									   struct ubus_object *obj,
									   struct ubus_request_data *req,
									   const char *method,
									   struct blob_attr *msg) {
	struct blob_attr *tb[__RPC_W_MAX];
	blobmsg_parse(rpc_wds_policy, __RPC_W_MAX, tb, blob_data(msg), blob_len(msg));

	char *ifname = blobmsg_data(tb[RPC_W_IFNAME]);
	char *ip = blobmsg_data(tb[RPC_W_IP]);
	char cmd[64];

	set("network", "lan", "proto", "dhcp");
	set("network", "lan", "oip", ip);
	del("network", "lan", "ipaddr");
	del("network", "lan", "netmask");
	del("network", "lan","ip6assign");

	system("/etc/init.d/network restart");

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "sh /www/luci2/scripts/wds_status.sh %s", ifname);
	system(cmd);

	return 0;
}

static int rpc_web_wireless_wds_disable(struct ubus_context *ctx,
										struct ubus_object *obj,
										struct ubus_request_data *req,
										const char *method,
										struct blob_attr *msg) {
	struct blob_attr *tb[__RPC_O_MAX];
	blobmsg_parse(rpc_offwds_policy, __RPC_O_MAX, tb, blob_data(msg),
				  blob_len(msg));

	//make sure htmode without "+-"
	char *htmode = get("wireless", "radio0", "htmode");
	char *oip = get("network", "lan", "oip");
	htmode[4] = '\0';
	set("wireless", "radio0", "htmode", htmode);

	set("wireless", blobmsg_data(tb[RPC_O_DEV]), "channel", blobmsg_data(tb[RPC_O_OC]));
	del("wireless", blobmsg_data(tb[RPC_O_DEV]), "origin_channel");
	del("wireless", "wds", NULL);
	set("network", "lan", "proto", "static");
	set("network", "lan", "ipaddr", oip);
	set("network", "lan", "netmask", "255.255.255.0");
	set("network", "lan", " ip6assign", "60");

	system("/etc/init.d/dnsmasq start");
	system("/etc/init.d/network restart");

	return 0;
}

static int
rpc_web_api_init(const struct rpc_daemon_ops *o, struct ubus_context *ctx)
{
	int rv = 0;

	static const struct ubus_method web_wireless_methods[] = {
		UBUS_METHOD_NOARG("get_test",         rpc_web_get_test),
		UBUS_METHOD("set_test",               rpc_web_set_test, rpc_data_policy),
		UBUS_METHOD("wds_wifi_self",          rpc_web_wireless_wds_wifi_self, rpc_wifiself_policy),
		UBUS_METHOD("wds_wifi_connect",       rpc_web_wireless_wds_wifi_connect, rpc_wificonn_policy),
		UBUS_METHOD("wds_wifi_connect_new",       rpc_web_wireless_wds_wifi_connect_new, rpc_wificonnnew_policy),
		UBUS_METHOD("wds_wifi_connect_status",       rpc_web_wireless_wds_wifi_connect_status, rpc_status_policy),
		UBUS_METHOD("wds_enable",             rpc_web_wireless_wds_enable, rpc_wds_policy),
		UBUS_METHOD("wds_disable",            rpc_web_wireless_wds_disable, rpc_offwds_policy)
	};

	static struct ubus_object_type web_wireless_type =
		UBUS_OBJECT_TYPE("siflower-rpc-web-wireless", web_wireless_methods);

	static struct ubus_object wireless_obj = {
		.name = "web.wireless",
		.type = &web_wireless_type,
		.methods = web_wireless_methods,
		.n_methods = ARRAY_SIZE(web_wireless_methods),
	};

	cursor = uci_alloc_context();

	if (!cursor)
		return UBUS_STATUS_UNKNOWN_ERROR;

	ops = o;

	rv |= ubus_add_object(ctx, &wireless_obj);

	return rv;
}

struct rpc_plugin rpc_plugin = {
	.init = rpc_web_api_init
};

