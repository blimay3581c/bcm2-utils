/**
 * bcm2-utils
 * Copyright (C) 2016 Joseph Lehner <joseph.c.lehner@gmail.com>
 *
 * bcm2-utils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bcm2-utils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bcm2-utils.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "nonvoldef.h"
#include "util.h"
using namespace std;

namespace bcm2cfg {
namespace {

template<class T> const sp<T>& nv_val_disable(const sp<T>& val, bool disable)
{
	val->disable(disable);
	return val;
}

template<class T> const sp<T>& nv_compound_rename(const sp<T>& val, const std::string& name)
{
	val->rename(name);
	return val;
}

bool is_zero_mac(const csp<nv_mac>& mac)
{
	return mac->to_str() == "00:00:00:00:00:00";
}

bool is_empty_string(const csp<nv_string>& str)
{
	return str->to_str().empty();
}

class nv_timestamp : public nv_u32
{
	public:
	virtual string to_string(unsigned, bool) const override
	{
		char buf[128];
		time_t time = num();
		strftime(buf, sizeof(buf) - 1, "%F %R", localtime(&time));
		return buf;
	}
};

class nv_time_period : public nv_compound_def
{
	public:
	nv_time_period()
	: nv_compound_def("time-period", {
			NV_VAR(nv_u8_m<23>, "beg_hrs"),
			NV_VAR(nv_u8_m<23>, "end_hrs"),
			NV_VAR(nv_u8_m<59>, "beg_min"),
			NV_VAR(nv_u8_m<59>, "end_min"),
	}) {}

	string to_string(unsigned level, bool pretty) const override
	{
		if (!pretty) {
			return nv_compound_def::to_string(level, pretty);
		}

		return num("beg_hrs") + ":" + num("beg_min") + "-" +
				num("end_hrs") + ":" + num("end_min");
	}

	private:
	string num(const string& name) const
	{
		string str = get(name)->to_pretty();
		return (str.size() == 2 ? "" : "0") + str;
	}
};

class nv_group_mlog : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_mlog, "MLog", "userif");

	protected:
	class nv_ipstacks : public nv_bitmask<nv_u8>
	{
		public:
		nv_ipstacks() : nv_bitmask("ipstacks", {
				"IP1", "IP2", "IP3", "IP4", "IP5", "IP6", "IP7", "IP8"
		}) {}
	};

	class nv_remote_acc_methods : public nv_bitmask<nv_u8>
	{
		public:
		nv_remote_acc_methods() : nv_bitmask("remote_acc_methods", {
				"telnet", "http", "ssh"
		}) {}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return {
				NV_VAR2(nv_bitmask<nv_u32>, "log_severities", {
						"fatal",
						"error",
						"warning",
						"fn_entry_exit",
						"trace",
						"info",
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "log_fields", {
						"severity",
						"instance",
						"function",
						"module",
						"timestamp",
						"thread",
						"milliseconds"
				}),
				NV_VAR(nv_data, "", 33),
				NV_VAR(nv_ip4, "remote_acc_ip"),
				NV_VAR(nv_ip4, "remote_acc_subnet"),
				NV_VAR(nv_ip4, "remote_acc_router"),
				NV_VAR(nv_u8, "remote_acc_ipstack"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "virtual_enet_ipstack"),
			};
		}

		if (profile() && profile()->cfg_flags() & BCM2_CFG_DATA_USERIF_ALT) {
			return {
				NV_VAR(nv_p16string, "http_user", 32),
				NV_VAR(nv_p16string, "http_pass", 32),
				NV_VAR(nv_p16string, "http_admin_user", 32),
				NV_VAR(nv_p16string, "http_admin_pass", 32),
				NV_VAR(nv_p16string, "http_local_user", 16),
				NV_VAR(nv_p16string, "http_local_pass", 16),
				NV_VAR(nv_p16string, "http_default_pass"),
				NV_VAR(nv_p16string, "http_erouter_user"),
				NV_VAR(nv_p16string, "http_erouter_pass"),
				NV_VAR2(nv_remote_acc_methods, "remote_acc_methods"),
				NV_VAR(nv_fzstring<16>, "remote_acc_user"),
				NV_VAR(nv_fzstring<16>, "remote_acc_pass"),
				NV_VAR(nv_data, "", 112),
				NV_VAR2(nv_ipstacks, "telnet_ipstacks"),
				NV_VAR2(nv_ipstacks, "ssh_ipstacks"),
				NV_VAR2(nv_u32, "remote_acc_timeout"),
				NV_VAR2(nv_ipstacks, "http_ipstacks"),
				NV_VAR2(nv_ipstacks, "http_adv_ipstacks"),
				NV_VAR2(nv_p16string, "http_seed"),
				NV_VAR2(nv_p16data, "http_acl_hosts"),
				NV_VAR2(nv_u32, "http_idle_timeout"),
				NV_VAR2(nv_bool, "log_exceptions"),
			};
		} else {
			return {
				NV_VAR(nv_p16string, "http_user", 32),
				NV_VAR(nv_p16string, "http_pass", 32),
				NV_VAR(nv_p16string, "http_admin_user", 32),
				NV_VAR(nv_p16string, "http_admin_pass", 32),
				NV_VAR(nv_remote_acc_methods, "remote_acc_methods"),
				NV_VAR(nv_fzstring<16>, "remote_acc_user"),
				NV_VAR(nv_fzstring<16>, "remote_acc_pass"),
				NV_VAR(nv_ipstacks, "telnet_ipstacks"),
				NV_VAR(nv_ipstacks, "ssh_ipstacks"),
				NV_VAR(nv_u32, "remote_acc_timeout"),
				NV_VAR(nv_ipstacks, "http_ipstacks"),
				NV_VAR(nv_ipstacks, "http_adv_ipstacks"),
				NV_VAR(nv_p16string, "http_seed"),
				NV_VAR(nv_p16data, "http_acl_hosts"),
				NV_VAR(nv_u32, "http_idle_timeout"),
				NV_VAR(nv_bool, "log_exceptions"),
				NV_VAR(nv_u32, "ssh_inactivity_timeout"),
			};
		}
	}
};

class nv_group_cmap : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cmap, "CMAp", "bfc")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return {
				NV_VAR2(nv_bitmask<nv_u8>, "console_features", {
						"stop_at_console",
						"skip_driver_init_prompt",
						"stop_at_console_prompt"
				}),
			};
		} else {
			return {
				NV_VAR2(nv_enum<nv_u32>, "serial_console_mode", "serial_console_mode",
						{ "disabled", "ro", "rw", "factory" }),
			};
		};
	}
};

class nv_group_thom : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_thom, "THOM", "thombfc")

	protected:
	class nv_eth_ports : public nv_bitmask<nv_u32>
	{
		public:
		nv_eth_ports()
		: nv_bitmask<nv_u32>("eth-ports", {
			{ 0x02000, "eth1" },
			{ 0x04000, "eth2" },
			{ 0x08000, "eth3" },
			{ 0x10000, "eth4" },
		}) {}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			// 0x6 = rw (0x4 = write, 0x2 = read)
			// 0x6 = rw (0x4 = rw, 0x2 = enable (?), 0x1 = factory (?))
			NV_VAR2(nv_bitmask<nv_u8>, "serial_console_mode", {
				"", "read", "write", "factory"
			}),
#if 0
			NV_VAR(nv_bool, "early_console_enable"), // ?
			NV_VAR(nv_u16, "early_console_bufsize", true),
			NV_VAR(nv_u8, "", true), // maybe this is early_console_enable?
			NV_VAR(nv_data, "", 3),
			// 1 = lan_access
			NV_VAR(nv_bitmask<nv_u8>, "features"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_eth_ports, "pt_interfacemask"),
			NV_VAR(nv_eth_ports, "pt_interfaces"),
#endif
		};
	}

};

class nv_group_8021 : public nv_group
{
	public:
	nv_group_8021(bool card2)
	: nv_group(card2 ? "8022" : "8021", "bcmwifi"s + (card2 ? "2" : ""))
	{}

	NV_GROUP_DEF_CLONE(nv_group_8021);

	protected:
	template<size_t N> class nv_cdata : public nv_data
	{
		public:
		nv_cdata() : nv_data(N) {}
	};

	class nv_wmm : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm, "wmm");

		protected:
		class nv_wmm_block : public nv_compound
		{
			public:
			NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm_block, "wmm-block");

			protected:
			class nv_wmm_params : public nv_compound
			{
				public:
				NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm_params, "wmm-params");

				protected:
				virtual list definition() const override
				{
					typedef nv_u16_r<0, 15> cwminaifs;
					typedef nv_u16_r<0, 1024> cwmax;
					typedef nv_u16_r<0, 8192> txop;

					return {
						NV_VAR(cwminaifs, "cwmin"),
						NV_VAR(cwmax, "cwmax"),
						NV_VAR(cwminaifs, "aifsn"),
						NV_VAR(txop, "txop_b"),
						NV_VAR(txop, "txop_ag")
					};
				}
			};

			virtual list definition() const override
			{
				return {
					NV_VARN(nv_wmm_params, "sta"),
					NV_VARN(nv_wmm_params, "ap"),
					NV_VAR(nv_bool, "ap_adm_control"),
					NV_VAR(nv_bool, "ap_oldest_first"),
				};
			}
		};

		virtual list definition() const override
		{
			return {
				NV_VARN(nv_wmm_block, "ac_be"),
				NV_VARN(nv_wmm_block, "ac_bk"),
				NV_VARN(nv_wmm_block, "ac_vi"),
				NV_VARN(nv_wmm_block, "ac_vo"),
			};
		}

	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return nv_group::definition(type, ver);
		}

		// known versions
		// 0x0015: TWG850
		// 0x001d: TWG870
		// 0x0021: TCW770
		// 0x0024: TC7200
		//
		// in its current form, only 0x0015 needs special care, whereas
		// 0x001d-0x0024 appear to be the same, at least until
		// in_network_radar_check

		typedef nv_u16_r<20, 1024> beacon_interval;
		typedef nv_u16_r<1, 255> dtim_interval;
		typedef nv_u16_r<256, 2346> frag_threshold;
		typedef nv_u16_r<1, 2347> rts_threshold;

		// XXX quite possibly an nv_u16
		class nv_wifi_rate_mbps : public nv_enum<nv_u8>
		{
			public:
			nv_wifi_rate_mbps() : nv_enum<nv_u8>("rate-mbps", nv_enum<nv_u8>::valmap {
				{ 0x00, "auto" },
				{ 0x02, "1" },
				{ 0x04, "2" },
				{ 0x0b, "5.5" },
				{ 0x0c, "6" },
				{ 0x12, "9" },
				{ 0x16, "11" },
				{ 0x18, "12" },
				{ 0x24, "18" },
				{ 0x30, "24" },
				{ 0x48, "36" },
				{ 0x60, "48" },
				{ 0x6c, "54" }
			})
			{}
		};

		const nv_enum<nv_u8>::valvec offauto = { "off", "auto" };

		if (type != fmt_perm) {
			return {
				NV_VAR(nv_zstring, "ssid", 33),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "b_channel"),
				NV_VAR(nv_u8, "", true),
				// 0x0f = all
				NV_VAR(nv_u8, "basic_rates", true), // XXX u16?
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_u8, "supported_rates", true), // 0x03 = min, 0x0f = all (1..11Mbps)
				NV_VAR(nv_data, "", 1),
				NV_VAR2(nv_enum<nv_u8>, "encryption", "encryption", {
						"none", "wep64", "wep128", "tkip", "aes", "tkip_aes",
						"tkip_wep64", "aes_wep64", "tkip_aes_wep64", "tkip_wep128",
						"aes_wep128", "tkip_aes_wep128"
				}),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_u8_r<1 COMMA() 3>, "authentication"), // 1 = open, 2 = shared key, 3 = both
				NV_VAR(nv_array<nv_cdata<5>>, "wep64_keys", 4),
				NV_VAR(nv_u8, "wep_key_num"),
				NV_VAR(nv_cdata<13>, "wep128_key_1"),
				NV_VAR(beacon_interval, "beacon_interval"),
				NV_VAR(dtim_interval, "dtim_interval"),
				NV_VAR(frag_threshold, "frag_threshold"),
				NV_VAR(rts_threshold, "rts_threshold"),
				NV_VAR(nv_array<nv_cdata<13>>, "wep128_keys", 3),
				NV_VAR2(nv_enum<nv_u8>, "mac_policy", "mac_policy", { "disabled", "allow", "deny" }),
				NV_VARN(nv_array<nv_mac>, "mac_table", 32, &is_zero_mac),
				NV_VAR(nv_bool, "preamble_long"),
				NV_VAR(nv_bool, "hide_ssid"),
				NV_VAR(nv_u8_r<1 COMMA() 8>, "txpower_level"),
				NV_VAR(nv_data, "", 0x20),
				NV_VAR(nv_u8, "short_retry_limit"),
				NV_VAR(nv_u8, "long_retry_limit"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "a_channel"),
				// 1 = auto, 4 = performance, 5 = lrs
				NV_VAR2(nv_enum<nv_u8>, "g_mode", "g_mode", { "b", "auto", "g", "", "", "performance", "lrs" }),
				NV_VAR(nv_bool, "radio_enabled"),
				NV_VAR(nv_bool, "g_protection"),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_wifi_rate_mbps, "g_rate"),
				NV_VAR(nv_u8_m<100>, "tx_power"),
				NV_VAR(nv_p16string, "wpa_psk"),
				NV_VAR(nv_data, "", 0x2),
				// wpa_rekey
				NV_VAR(nv_u16, "wpa_rekey_interval"),
				NV_VAR(nv_ip4, "radius_ip"),
				NV_VAR(nv_u16, "radius_port"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_p8string, "radius_key"),
				// [size-5]: nv_bool frame_burst_enabled
				// [size-6]: nv_bool shared_key_auth_required
				NV_VAR(nv_data, "", (ver.num() <= 0x0015 ? 0x56 : 0x2a) - 0x1d),
				NV_VAR(nv_bool, "wds_enabled"),
				NV_VAR(nv_array<nv_mac>, "wds_list", 4),
				NV_VAR(nv_bool, "enable_afterburner"),
				NV_VAR(nv_data, "", 3),
				NV_VAR2(nv_bitmask<nv_u8>, "wpa_auth", "wpa_auth", { "802.1x", "wpa1", "psk1", "wpa2", "psk2" }),
				NV_VAR(nv_data, "", 2),
				NV_VAR(nv_u16, "wpa_reauth_interval"),
				NV_VAR(nv_bool, "wpa2_preauth_enabled"),
				NV_VAR(nv_data, "", 3),
				NV_VAR(nv_bool, "wmm_enabled"),
				NV_VAR(nv_bool, "wmm_nak"),
				NV_VAR(nv_bool, "wmm_powersave"),
				// nv_i32(?) wmm_vlan_mode: -1 = auto, 0 = off, 1 = on
				NV_VAR(nv_data, "", 4),
				NV_VARN(nv_wmm, "wmm"),
				NV_VAR2(nv_enum<nv_u8>, "n_band", "band", { "", "2.4Ghz", "5Ghz" }),
				NV_VAR(nv_u8, "n_control_channel"),
				NV_VAR2(nv_enum<nv_u8>, "n_mode", "off_auto", offauto), // 0 = off, 1 = auto
				NV_VAR2(nv_enum<nv_u8>, "n_bandwidth", "n_bandwidth", nv_enum<nv_u8>::valmap {
					{ 10, "10MHz" },
					{ 20, "20MHz" },
					{ 40, "40MHz" }
				}),
				NV_VAR2(nv_enum<nv_i8>, "n_sideband", "sideband", nv_enum<nv_i8>::valmap {
					{ -1, "lower" },
					{ 0, "none" },
					{ 1, "upper"}
				}),
				NV_VAR2(nv_enum<nv_i8>, "n_rate", "n_rate", nv_enum<nv_i8>::valmap {
						{ -2, "legacy" },
						{ -1, "auto" },
						{ 0, "0" },
						{ 1, "1" },
						{ 2, "2" },
						{ 3, "3" },
						{ 4, "4" },
						{ 5, "5" },
						{ 6, "6" },
						{ 7, "7" },
						{ 8, "8" },
						{ 9, "9" },
						{ 10, "10" },
						{ 11, "11" },
						{ 12, "12" },
						{ 13, "13" },
						{ 14, "14" },
						{ 15, "15" },
						{ 32, "mcs_index" },
				}),
				NV_VAR2(nv_enum<nv_u8>, "n_protection", "off_auto", offauto), // 0 = off, 1 = auto
				NV_VAR(nv_bool, "wps_enabled"),
				NV_VAR(nv_bool, "wps_configured"),
				// these appear to be p8string on some devices
				NV_VAR(nv_p8string, "wps_device_pin"),
				NV_VAR(nv_p8zstring, "wps_model"),
				NV_VAR(nv_p8zstring, "wps_manufacturer"),
				NV_VAR(nv_p8zstring, "wps_device_name"),
				// ?? nv_bool wps_external ??
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_p8zstring, "wps_sta_pin"),
				NV_VAR(nv_p8zstring, "wps_model_num"),
				NV_VAR(nv_bool, "wps_timeout"),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_p8zstring, "wps_uuid"),
				NV_VAR(nv_p8zstring, "wps_board_num"),
				// ?? stays 0 when setting to "configed" (?wps_sta_mac_restrict?)
				NV_VAR(nv_bool, "wps_configured2"),
				NV_VAR(nv_p8zstring, "country"),
				NV_VAR(nv_bool, "primary_network_enabled"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_wifi_rate_mbps, "a_mcast_rate"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_wifi_rate_mbps, "bg_mcast_rate"),
				NV_VAR2(nv_enum<nv_u8>, "reg_mode", "reg_mode", {
					"off", "802.11d", "802.11h"
				}),
				NV_VAR(nv_u8_m<99>, "pre_network_radar_check"),
				NV_VAR(nv_u8_m<99>, "in_network_radar_check"),
				// XXX enum: 0 (off), 2, 3, 4 dB
				NV_VAR2(nv_enum<nv_u8>, "tpc_mitigation", "", {
						"off", "", "2dB", "3dB", "4dB"
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "features", nv_bitmask<nv_u8>::valmap {
					{ 0x01, "obss_coex" },
					{ 0x04, "ap_isolate" },
					{ 0x10, "stbc_tx_on" },
					{ 0x20, "stbc_tx_off" },
					{ 0x40, "sgi_on" },
					{ 0x80, "sgi_off" },
				}),
				NV_VAR(nv_data, "", 1),
				// 0 = default, 1 = legacy, 2 = intf, 3 = intf-busy, 4 = optimized,
				// 5 = optimized1, 6 = optimized2, 7 = user
				NV_VAR(nv_u8_m<7>, "acs_policy_index"),
				NV_VAR(nv_data, "", 3),
				NV_VAR(nv_u8, "acs_flags"),
				NV_VAR(nv_p8string, "acs_user_policy"),
				NV_VAR(nv_p8list<nv_u32>, ""),
				NV_VAR(nv_u8_m<128>, "bss_max_assoc"),
				NV_VAR2(nv_enum<nv_u8>, "bandwidth_cap", "", nv_enum<nv_u8>::valmap {
					{ 0x01, "20MHz" },
					{ 0x03, "40MHz" },
					{ 0x07, "80MHz" }
				}),
				// this is the same as n_control_channel, but as text (could also be a p8zstring)
				NV_VAR(nv_p8istring, ""),
				NV_VAR(nv_data, "", 0x13),
				// 0x01 = txchain(1), 0x03 = txchain(2), 0x07 = txchain(3)
				NV_VAR(nv_u8, "txchain", true),
				// same as above
				NV_VAR(nv_u8, "rxchain", true),
				NV_VAR2(nv_enum<nv_u8>, "mimo_preamble", "", {
						"mixed", "greenfield", "greenfield_broadcom"
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "features2", nv_bitmask<nv_u8>::valvec {
						"ampdu", "amsdu",
				}),
				NV_VAR(nv_u32, "acs_cs_scan_timer"),
				NV_VAR(nv_u8_m<3>, "bss_mode_reqd", true),
				NV_VAR(nv_data, "", 3),
				NV_VAR2(nv_bitmask<nv_u8>, "features3", nv_bitmask<nv_u8>::valmap {
					{ 0x01, "band_steering" },
					{ 0x02, "airtime_fairness" },
					{ 0x04, "traffic_scheduler" },
					{ 0x08, "exhausted_buf_order_sched" },
				}),
			};
		}

		return nv_group::definition(type, ver);
	}
};

class nv_group_t802 : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_t802, "T802", "tmmwifi")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return nv_group::definition(type, ver);
		}

		return {
#if 0
			NV_VAR(nv_array<nv_u8 COMMA() 10>, "wifi_sleep"),
#else
			NV_VAR(nv_bool, "sleep_breaking_time"),
			NV_VAR(nv_bool, "sleep_every_day"),
			NV_VAR(nv_bitmask<nv_u8>, "sleep_days"),
			NV_VAR(nv_bool, "sleep_all_day"),
#if 0
			NV_VAR(nv_u8_m<23>, "sleep_begin_h"),
			NV_VAR(nv_u8_m<23>, "sleep_end_h"),
			NV_VAR(nv_u8_m<59>, "sleep_begin_m"),
			NV_VAR(nv_u8_m<59>, "sleep_end_m"),
#else
			NV_VAR(nv_time_period, "sleep_time"),
#endif
			NV_VAR(nv_bool, "sleep_enabled"),
			NV_VAR(nv_bool, "sleep_page_visible"),
#endif
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_fzstring<33>, "ssid_24"),
			NV_VAR(nv_fzstring<33>, "ssid_50"),
			NV_VAR(nv_u8, "", true),
			NV_VAR(nv_p8string, "wpa_psk_24"),
			NV_VAR(nv_u8, "", true),
			NV_VAR(nv_p8string, "wpa_psk_50"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_fzstring<33>, "wifi_opt60_replace"),
			NV_VAR(nv_data, "", 8),
			NV_VAR(nv_fstring<33>, "card1_prefix"),
			// the firmware refers to this as "Card-1 Ramdon"
			NV_VAR(nv_fzstring<33>, "card1_random"),
			NV_VAR(nv_fzstring<33>, "card2_prefix"),
			NV_VAR(nv_fzstring<33>, "card2_random"),
			NV_VAR(nv_u8_m<99>, "card1_regul_rev"),
			NV_VAR(nv_u8_m<99>, "card2_regul_rev"),
		};
	}
};

class nv_group_rg : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_rg, "RG..", "rg")

	protected:
	template<int N> class nv_ip_range : public nv_compound
	{
		public:
		nv_ip_range() : nv_compound(false) {}

		virtual string type() const override
		{ return "ip" + ::to_string(N) + "_range"; }

		virtual string to_string(unsigned level, bool pretty) const override
		{
			return get("start")->to_string(level, pretty) + "," + get("end")->to_string(level, pretty);
		}


		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip<N>, "start"),
				NV_VAR(nv_ip<N>, "end")
			};
		}
	};

	class nv_ip4_range : public nv_ip_range<4>
	{
		public:
		static bool is_end(const csp<nv_ip4_range>& r)
		{
			return r->get("start")->to_str() == "0.0.0.0" && r->get("end")->to_str() == "0.0.0.0";
		}
	};

	class nv_port_range : public nv_compound
	{
		public:
		nv_port_range() : nv_compound(false) {}

		virtual string type() const override
		{ return "port-range"; }

		static bool is_range(const csp<nv_port_range>& r, uint16_t start, uint16_t end)
		{
			return r->get_as<nv_u16>("start")->num() == start && r->get_as<nv_u16>("end")->num() == end;
		}

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u16, "start"),
				NV_VAR(nv_u16, "end")
			};
		}

	};

	class nv_proto : public nv_enum<nv_u8>
	{
		public:
		// FIXME in RG 0x0016, TCP and UDP are reversed!
		nv_proto(bool reversed = false) : nv_enum<nv_u8>("protocol", {
			{ 0x3, reversed ? "UDP" : "TCP" },
			{ 0x4, reversed ? "TCP" : "UDP" },
			{ 0xfe, "TCP+UDP" }
		}) {}
	};

	class nv_port_forward : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_forward, "port-forward");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip4, "dest"),
				NV_VAR(nv_port_range, "ports"),
				NV_VAR(nv_proto, "type"),
			};
		}
	};

	class nv_port_forward_dport : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_forward_dport, "port-forward-dport");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_port_range, "ports"),
				NV_VAR(nv_data, "data", 4),
			};
		}
	};

	class nv_port_trigger : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_trigger, "port-trigger");

		static bool is_end(const csp<nv_port_trigger>& pt)
		{
			return nv_port_range::is_range(pt->get_as<nv_port_range>("trigger"), 0, 0);
		}

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_port_range, "trigger"),
				NV_VAR(nv_port_range, "target"),
			};
		}
	};

	template<bool ROUTE1> class nv_route : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_route, "route");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip4, ROUTE1 ? "netmask" : "network"),
				NV_VAR(nv_ip4, ROUTE1 ? "network" : "gateway"),
				NV_VAR(nv_ip4, ROUTE1 ? "gateway" : "netmask"),
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		typedef nv_i32_r<-45000, 45000> timezone_offset;

		// TWG870: version 0x0016 (0.22)

		return {
			NV_VAR(nv_bool, "router_mode"),
			NV_VAR(nv_zstring, "http_pass", 9),
			NV_VAR(nv_zstring, "http_realm", 256),
			NV_VAR(nv_mac, "spoofed_mac"),
			NV_VAR2(nv_bitmask<nv_u32>, "features1", nv_bitmask<nv_u32>::valvec {
				"wan_conn_pppoe",
				"", // 0x02 (unset by default, automatically removed if set)
				"feature_ip_filters",
				"feature_port_filters",
				"wan_block_pings",
				"feature_ipsec_passthrough",
				"feature_pptp_passthrough",
				"wan_remote_cfg_mgmt",
				"feature_ip_forwarding", // 0x0100 (unset by default)
				"feature_dmz",
				"wan_conn_static",
				"feature_nat_debug",
				"lan_dhcp_server",
				"lan_http_server",
				"primary_default_override",
				"feature_mac_filters",
				"feature_port_triggers",
				"feature_multicast",
				"wan_rip",
				"", // 0x080000 (unset by default)
				"feature_dmz_by_hostname",
				"lan_upnp",
				"lan_routed_subnet",
				"lan_routed_subnet_dhcp",
				"wan_passthrough_skip_dhcp",
				"lan_routed_subnet_nat",
				"", // 0x04000000
				"wan_sntp",
				"wan_conn_pptp",
				"wan_pptp_server",
				"feature_ddns",
				"", // 0x80000000
			}),
			NV_VAR(nv_ip4, "dmz_ip"),
			NV_VAR(nv_ip4, "wan_ip"),
			NV_VAR(nv_ip4, "wan_mask"),
			NV_VAR(nv_ip4, "wan_gateway"),
			NV_VAR(nv_fzstring<0x100>, "wan_dhcp_hostname"),
			NV_VAR(nv_fzstring<0x100>, "syslog_email"),
			NV_VAR(nv_fzstring<0x100>, "syslog_smtp"),
			NV_VARN(nv_array<nv_ip4_range>, "ip_filters", 10 , &nv_ip4_range::is_end),
			NV_VARN(nv_array<nv_port_range>, "port_filters", 10, [] (const csp<nv_port_range>& range) {
				return nv_port_range::is_range(range, 1, 0xffff);
			}),
			NV_VARN(nv_array<nv_port_forward>, "port_forwards", 10, [] (const csp<nv_port_forward>& fwd) {
				return fwd->get("dest")->to_str() == "0.0.0.0";
			}),
			NV_VARN(nv_array<nv_mac>, "mac_filters", 20, &is_zero_mac),
			NV_VARN(nv_array<nv_port_trigger>, "port_triggers", 10, &nv_port_trigger::is_end),
			NV_VAR(nv_data, "", 0x15),
			NV_VARN(nv_array<nv_proto>, "port_filter_protocols", 10),
			NV_VAR(nv_data, "", 0xaa),
			NV_VARN(nv_array<nv_proto>, "port_trigger_protocols", 10),
			NV_VAR(nv_data, "", 0x443),
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_p8string, "rip_key"),
			NV_VAR(nv_u16, "rip_reporting_interval"),
			NV_VAR(nv_data, "", 0xa),
			NV_VAR(nv_route<true>, "route1"),
			NV_VAR(nv_route<false>, "route2"),
			NV_VAR(nv_route<false>, "route3"),
			NV_VAR(nv_ip4, "nat_route_gateway"),
			NV_VAR(nv_array<nv_ip4>, "nat_route_dns", 3),
			NV_VAR(nv_p8string, "l2tp_username"),
			NV_VAR(nv_p8string, "l2tp_password"),
			NV_VAR(nv_data, "", 5),
			NV_VARN(nv_p8list<nv_p8string>, "timeservers"),
			NV_VAR(timezone_offset, "timezone_offset"),
			NV_VARN3(ver.num() > 0x0016, nv_array<nv_port_forward_dport>, "port_forward_dports", 10, [] (const csp<nv_port_forward_dport>& range) {
				return nv_port_range::is_range(range->get_as<nv_port_range>("ports"), 0, 0);
			}),
			NV_VAR(nv_p16string, "ddns_username"),
			NV_VAR(nv_p16string, "ddns_password"),
			NV_VAR(nv_p16string, "ddns_hostname"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_u16, "mtu"),
			NV_VAR(nv_data, "", 3),
			// 0x01 = wan_l2tp_server , 0x02 = wan_conn_l2tp (?)
			NV_VAR2(nv_bitmask<nv_u8>, "features2", nv_bitmask<nv_u8>::valvec {
				"wan_l2tp_server",
				"wan_conn_l2tp"
			}),
			NV_VAR(nv_ip4, "l2tp_server_ip"),
			NV_VAR(nv_p8string, "l2tp_server_name"), // could also be a p8zstring

		};
	}
};

class nv_group_cdp : public nv_group
{
	public:

	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cdp, "CDP.", "dhcp")

	protected:
	class nv_ip4_typed : public nv_compound
	{
		public:
		nv_ip4_typed() : nv_compound(false) {}

		virtual string type() const override
		{ return "typed_ip"; }

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u32, "type"),
				NV_VAR(nv_ip4, "ip")
			};
		}
	};

	class nv_lan_addr_entry : public nv_compound
	{
		public:
		nv_lan_addr_entry() : nv_compound(false) {}

		string type() const override
		{ return "lan_addr"; }

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u16, "num_1"),
				NV_VAR(nv_u16, "create_time"),
				NV_VAR(nv_u16, "num_2"),
				NV_VAR(nv_u16, "expire_time"),
				NV_VAR(nv_u8, "ip_type"),
				NV_VAR(nv_ip4, "ip"),
				NV_VAR(nv_data, "ip_data", 3),
				NV_VAR(nv_u8, "method"),
				NV_VAR(nv_p8data, "client_id"),
				NV_VAR(nv_p8string, "hostname"),
				NV_VAR(nv_mac, "mac"),
			};
		}
	};

	class nv_wan_dns_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wan_dns_entry, "wan-dns-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_ip4, "ip")
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 7),
			NV_VAR(nv_u8, "lan_trans_threshold"),
			NV_VAR(nv_data, "", 8),
			NV_VARN(nv_ip4_typed, "dhcp_pool_start"),
			NV_VARN(nv_ip4_typed, "dhcp_pool_end"),
			NV_VARN(nv_ip4_typed, "dhcp_subnet_mask"),
			NV_VAR(nv_data, "", 4),
			NV_VARN(nv_ip4_typed, "router_ip"),
			NV_VARN(nv_ip4_typed, "dns_ip"),
			NV_VARN(nv_ip4_typed, "syslog_ip"),
			NV_VAR(nv_u32, "ttl"),
			NV_VAR(nv_data, "", 4),
			NV_VARN(nv_ip4_typed, "ip_2"),
			NV_VAR(nv_p8string, "domain"),
			NV_VAR(nv_data, "", 7),
			//NV_VARN(nv_ip4_typed, "ip_3"),
			NV_VARN(nv_array<nv_lan_addr_entry>, "lan_addrs", 16),
			//NV_VAR(nv_lan_addr_entry, "lan_addr_1"), // XXX make this an array
			NV_VAR(nv_data, "", 0x37a),
			NV_VAR(nv_array<nv_wan_dns_entry>, "wan_dns", 3),
		};
	}
};


class nv_group_fire : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_fire, "FIRE", "firewall")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 2),
			NV_VAR2(nv_bitmask<nv_u16>, "features", nv_bitmask<nv_u16>::valvec {
				"keyword_blocking",
				"domain_blocking",
				"http_proxy_blocking",
				"disable_cookies",
				"disable_java_applets",
				"disable_activex_ctrl",
				"disable_popups",
				"mac_tod_filtering",
				"email_alerts",
				"",
				"",
				"",
				""
				"block_fragmented_ip",
				"port_scan_detection",
				"syn_flood_detection"
			}),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_u8, "word_filter_count"),
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_u8, "domain_filter_count"),
			NV_VAR2(nv_array<nv_fstring<0x20>>, "word_filters", 16, &is_empty_string),
			NV_VAR2(nv_array<nv_fstring<0x40>>, "domain_filters", 16, &is_empty_string),
			NV_VAR(nv_data, "", 0x2d4), // this theoretically gives room for 11 more domain filters
			NV_VAR(nv_data, "", 0xc),
			// 0x00 = all (!)
			// 0x01 = sunday
			// 0x40 = saturday
			NV_VAR(nv_bitmask<nv_u8>, "tod_filter_days"),
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_time_period, "tod_filter_time"),
			NV_VAR(nv_data, "", 0x2a80),
			NV_VAR(nv_ip4, "syslog_ip"),
			NV_VAR(nv_data, "", 2),
			// or nv_u8?
			// 0x08 = product config events
			// 0x04 = "known internet attacks"
			// 0x02 = blocked connections
			// 0x01 = permitted connections
			NV_VAR(nv_bitmask<nv_u16>, "syslog_events"),

		};
	}
};

class nv_group_cmev : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cmev, "CMEV", "cmlog")

	protected:
	class nv_log_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_log_entry, "log-entry")

		static bool is_end(const csp<nv_log_entry>& log_entry)
		{
			return log_entry->get("msg")->bytes() == 0;
		}

		virtual list definition() const override
		{
			return {
				NV_VAR(nv_data, "data", 8),
				NV_VAR(nv_timestamp, "time1"),
				NV_VAR(nv_timestamp, "time2"),
				NV_VAR(nv_p16string, "msg")
			};
		}
	};


	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_u8, "", true), // maybe a p16list??
			NV_VARN(nv_p8list<nv_log_entry>, "log"),
		};
	}
};

class nv_group_xxxl : public nv_group
{
	public:
	nv_group_xxxl(const string& magic)
	: nv_group(magic, bcm2dump::transform(magic, ::tolower))
	{}

	NV_GROUP_DEF_CLONE(nv_group_xxxl)

	protected:
	class nv_log_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_log_entry, "log-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_timestamp, "time"),
				NV_VAR(nv_p16istring, "msg"),
				NV_VAR(nv_data, "", 2)
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 1),
			NV_VARN(nv_p8list<nv_log_entry>, "log")
		};
	}
};

class nv_group_upc : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_upc, "UPC.", "upc")

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_bool, "parental_enable"),
			NV_VAR(nv_data, "", 6),
			NV_VAR(nv_u16, "parental_activity_time_enable"),
			NV_VAR(nv_zstring, "parental_password", 10),
			NV_VAR(nv_data, "", 0x2237),
			NV_VAR(nv_u8, "web_country"),
			NV_VAR(nv_u8, "web_language"),
			NV_VAR(nv_bool, "web_syslog_enable"),
			NV_VAR2(nv_bitmask<nv_u8>, "web_syslog_level", "", {
					"critical",
					"major",
					"minor",
					"warning",
					"inform"
			}),
			NV_VAR2(nv_array<nv_mac>, "trusted_macs", 10, &is_zero_mac),
			NV_VAR(nv_data, "", 0xd8),
			NV_VAR(nv_array<nv_ip4>, "lan_dns4_list", 3),
			NV_VAR(nv_array<nv_ip6>, "lan_dns6_list", 3),
		};
	}
};

class nv_group_xbpi : public nv_group
{
	public:
	nv_group_xbpi(bool ebpi)
	: nv_group(ebpi ? "Ebpi" : "bpi ", ebpi ? "ebpi" : "bpi")
	{}

	NV_GROUP_DEF_CLONE(nv_group_xbpi)

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_dyn) {
			return {
				NV_VAR(nv_data, "code_access_start", 12),
				NV_VAR(nv_data, "cvc_access_start", 12),
				NV_VAR(nv_u8, "", true),
			};
			//return nv_group::definition(format, ver);
		}

		return {
#if 0
			NV_VAR3(format == fmt_dyn, nv_data, "code_access_start", 12),
			NV_VAR3(format == fmt_dyn, nv_data, "cvc_access_start", 12),
			NV_VAR3(format == fmt_dyn, nv_u8, "", true),
#endif
			NV_VAR(nv_p16data, "bpi_public_key"),
			NV_VAR(nv_p16data, "bpi_private_key"),
			NV_VAR(nv_p16data, "bpiplus_root_public_key"),
			NV_VAR(nv_p16data, "bpiplus_cm_certificate"),
			NV_VAR(nv_p16data, "bpiplus_ca_certificate"),
			NV_VAR(nv_p16data, "bpiplus_cvc_root_certificate"),
			NV_VAR(nv_p16data, "bpiplus_cvc_ca_certificate"),
		};
	}
};

class nv_group_rca : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_rca, "RCA ", "thomcm")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_data, "", 2),
			NV_VAR(nv_p8list<nv_u32>, "scan_list_freqs"),
			NV_VAR(nv_u32, "us_alert_poll_period"),
			//NV_VAR(nv_data, "", 1),
			NV_VAR(nv_u32, "us_alert_thresh_dbmv_min"),
			NV_VAR(nv_u32, "us_alert_thresh_dbmv_max"),
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_p8data, "html_bitstring"),
			NV_VAR2(nv_enum<nv_u8>, "message_led", "", {
					"",
					"off",
					"on",
					"flashing"
			}),
			NV_VAR(nv_data, "", 5),
			NV_VAR(nv_p8list<nv_u32>, "last_known_freqs"),
			NV_VAR(nv_data, "", 2),

		};
	}
};

class nv_group_halif : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_halif, 0xf2a1f61f, "halif")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_dyn) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR2(nv_bitmask<nv_u32>, "interfaces", "interfaces", {
					"docsis",
					"ethernet",
					"hpna",
					"usb",
					"IP1",
					"IP2",
					"IP3",
					"IP4",
					"davic",
					"pci",
					"bluetooh",
					"802.11",
					"packet_generator",
					"IP5",
					"IP6",
					"IP7",
					"IP8",
					"wan_ethernet",
					"scb",
					"itc",
					"moca"

			}),
			NV_VAR(nv_array<nv_mac COMMA() 4>, "mac_addrs_1"),
			NV_VAR(nv_data, "", 5),
			NV_VAR(nv_u8, "board_rev"),
			NV_VAR(nv_u16, "usb_vid", true),
			NV_VAR(nv_u16, "usb_pid", true),
			NV_VAR(nv_mac, "usb_mac"),
			//NV_VAR(nv_bool, "usb_rndis"),
			NV_VAR(nv_data, "", 0x19),
			NV_VAR(nv_mac, "bluetooth_local_mac"),
			NV_VAR(nv_mac, "bluetooth_remote_mac"),
			//NV_VAR(nv_bool, "bluetooth_master"),
			NV_VAR(nv_data, "", 0x15),
			NV_VAR(nv_array<nv_mac COMMA() 4>, "mac_addrs_2"),
		};
	}
};

class nv_group_msc : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_msc, "MSC.", "msc")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_p8istring, "server_name"),
			NV_VAR(nv_data, "", 6),
			NV_VAR(nv_data, "", 2),
		};
	}
};

struct registrar {
	registrar()
	{
		vector<sp<nv_group>> groups = {
			NV_GROUP(nv_group_cmap),
			NV_GROUP(nv_group_mlog),
			NV_GROUP(nv_group_8021, false),
			NV_GROUP(nv_group_8021, true),
			NV_GROUP(nv_group_t802),
			NV_GROUP(nv_group_rg),
			NV_GROUP(nv_group_cdp),
			NV_GROUP(nv_group_fire),
			NV_GROUP(nv_group_cmev),
			NV_GROUP(nv_group_upc),
			NV_GROUP(nv_group_xxxl, "RSTL"),
			NV_GROUP(nv_group_xxxl, "CMBL"),
			NV_GROUP(nv_group_xxxl, "EMBL"),
			NV_GROUP(nv_group_thom),
			NV_GROUP(nv_group_xbpi, false),
			NV_GROUP(nv_group_xbpi, true),
			NV_GROUP(nv_group_halif),
			NV_GROUP(nv_group_rca),
			NV_GROUP(nv_group_msc),
		};

		for (auto g : groups) {
			nv_group::registry_add(g);
		}
	}
} instance;
}
}
