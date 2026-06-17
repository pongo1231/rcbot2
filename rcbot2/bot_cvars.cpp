
#include "icvar.h"
// #include "iconvar.h"
#include "convar.h"

#include "bot_cvars.h"

static ICvar *s_pCVar;

#pragma GCC visibility push(default)
ConVar *rcbot_tf2_debug_spies_cloakdisguise = nullptr;
ConVar *rcbot_tf2_medic_letgotime = nullptr;
ConVar *rcbot_tf2_pyro_airblast = nullptr;
ConVar *rcbot_const_point_master_offset = nullptr;
ConVar *rcbot_projectile_tweak = nullptr;
ConVar *bot_cmd_enable_wpt_sounds = nullptr;
ConVar *bot_general_difficulty = nullptr;
ConVar *bot_visrevs_clients = nullptr;
ConVar *bot_spyknifefov = nullptr;
ConVar *bot_visrevs = nullptr;
ConVar *bot_pathrevs = nullptr;
ConVar *bot_command = nullptr;
ConVar *bot_attack = nullptr;
ConVar *bot_scoutdj = nullptr;
ConVar *bot_anglespeed = nullptr;
ConVar *bot_stop = nullptr;
ConVar *bot_waypointpathdist = nullptr;
ConVar *bot_rj = nullptr;
ConVar *bot_defrate = nullptr;
ConVar *bot_beliefmulti = nullptr;
ConVar *bot_belief_fade = nullptr;
ConVar *bot_change_class = nullptr;
ConVar *bot_use_vc_commands = nullptr;
ConVar *bot_use_disp_dist = nullptr;
ConVar *bot_max_cc_time = nullptr;
ConVar *bot_min_cc_time = nullptr;
ConVar *bot_avoid_radius = nullptr;
ConVar *bot_avoid_strength = nullptr;
ConVar *bot_messaround = nullptr;
ConVar *bot_heavyaimoffset = nullptr;
ConVar *bot_aimsmoothing = nullptr;
ConVar *bot_bossattackfactor = nullptr;
ConVar *rcbot_enemyshootfov = nullptr;
ConVar *rcbot_enemyshoot_gravgun_fov = nullptr;
ConVar *rcbot_wptplace_width = nullptr;
ConVar *rcbot_wpt_autoradius = nullptr;
ConVar *rcbot_wpt_autotype = nullptr;
ConVar *rcbot_move_sentry_time = nullptr;
ConVar *rcbot_move_sentry_kpm = nullptr;
ConVar *rcbot_smoke_time = nullptr;
ConVar *rcbot_move_disp_time = nullptr;
ConVar *rcbot_move_disp_healamount = nullptr;
ConVar *rcbot_demo_runup_dist = nullptr;
ConVar *rcbot_demo_jump = nullptr;
ConVar *rcbot_move_tele_time = nullptr;
ConVar *rcbot_move_tele_tpm = nullptr;
ConVar *rcbot_tf2_protect_cap_time = nullptr;
ConVar *rcbot_tf2_protect_cap_percent = nullptr;
ConVar *rcbot_tf2_spy_kill_on_cap_dist = nullptr;
ConVar *rcbot_move_dist = nullptr;
ConVar *rcbot_shoot_breakables = nullptr;
ConVar *rcbot_shoot_breakable_dist = nullptr;
ConVar *rcbot_shoot_breakable_cos = nullptr;
ConVar *rcbot_move_obj = nullptr;
ConVar *rcbot_taunt = nullptr;
ConVar *bot_highfive = nullptr;
ConVar *rcbot_notarget = nullptr;
ConVar *rcbot_nocapturing = nullptr;
ConVar *rcbot_jump_obst_dist = nullptr;
ConVar *rcbot_jump_obst_speed = nullptr;
ConVar *rcbot_speed_boost = nullptr;
ConVar *rcbot_melee_only = nullptr;
ConVar *rcbot_debug_iglev = nullptr;
ConVar *rcbot_dont_move = nullptr;
ConVar *rcbot_runplayercmd_dods = nullptr;
ConVar *rcbot_ladder_offs = nullptr;
ConVar *rcbot_ffa = nullptr;
ConVar *rcbot_prone_enemy_only = nullptr;
ConVar *rcbot_menu_update_time1 = nullptr;
ConVar *rcbot_menu_update_time2 = nullptr;
ConVar *rcbot_autowaypoint_dist = nullptr;
ConVar *rcbot_stats_inrange_dist = nullptr;
ConVar *rcbot_squad_idle_time = nullptr;
ConVar *rcbot_bots_form_squads = nullptr;
ConVar *rcbot_listen_dist = nullptr;
ConVar *rcbot_footstep_speed = nullptr;
ConVar *rcbot_bot_squads_percent = nullptr;
ConVar *rcbot_tooltips = nullptr;
ConVar *rcbot_debug_notasks = nullptr;
ConVar *rcbot_debug_dont_shoot = nullptr;
ConVar *rcbot_debug_show_route = nullptr;
ConVar *rcbot_tf2_autoupdate_point_time = nullptr;
ConVar *rcbot_tf2_payload_dist_retreat = nullptr;
ConVar *rcbot_spy_runaway_health = nullptr;
ConVar *rcbot_supermode = nullptr;
ConVar *rcbot_addbottime = nullptr;
ConVar *rcbot_hijack_afk_time = nullptr;
ConVar *rcbot_gamerules_offset = nullptr;
ConVar *rcbot_bot_quota_interval = nullptr;
ConVar *rcbot_show_welcome_msg = nullptr;
ConVar *rcbot_force_class = nullptr;
ConVar *rcbot_process_usercmds_offset = nullptr;

ConVarRef sv_gravity("sv_gravity");
ConVarRef mp_teamplay("mp_teamplay");
ConVarRef sv_tags("sv_tags");
ConVarRef mp_friendlyfire("mp_friendlyfire");
ConVarRef mp_stalemate_enable("mp_stalemate_enable");
ConVarRef mp_stalemate_meleeonly("mp_stalemate_meleeonly");

void RCBOT2_Cvar_setup(ICvar *cvar)
{
	if (sv_tags.IsValid())
	{
		char sv_tags_str[512];

		strcpy(sv_tags_str, sv_tags.GetString());

		// fix
		if (strstr(sv_tags_str, "rcbot2") == nullptr)
		{

			if (sv_tags_str[0] == 0)
				strcat(sv_tags_str, "rcbot2");
			else
				strcat(sv_tags_str, ",rcbot2");

			sv_tags.SetValue(sv_tags_str);
		}
	}
}

ConVar *rcbot_mvm_revive_markers = nullptr;
#pragma GCC visibility pop
