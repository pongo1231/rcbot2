// AUTO-GENERATED — do not edit manually
#include "icvar.h"
#include "convar.h"
#include "bot_cvars.h"

#pragma GCC visibility push(default)
bool g_bCvarsReady = false;

void RCBOT2_Cvar_Register(ICvar *cvar)
{
	rcbot_tf2_debug_spies_cloakdisguise = new ConVar("rcbot_tf2_debug_spies_cloakdisguise", "1", 0, "");

	rcbot_tf2_medic_letgotime = new ConVar("rcbot_tf2_medic_letgotime", "0.4", 0, "");

	rcbot_tf2_pyro_airblast = new ConVar("rcbot_tf2_pyro_airblast_ammo", "50", 0, "");

	rcbot_projectile_tweak = new ConVar("rcbot_projtweak", "0.05", 0, "");

	bot_cmd_enable_wpt_sounds = new ConVar("rcbot_enable_wpt_sounds", "1", 0, "");

	bot_general_difficulty = new ConVar("rcbot_skill", "0.6", 0, "");

	bot_visrevs_clients = new ConVar("rcbot_visrevs_clients", "4", 0, "");

	bot_spyknifefov = new ConVar("rcbot_spyknifefov", "80", 0, "");

	bot_visrevs = new ConVar("rcbot_visrevs", "9", 0, "");

	bot_pathrevs = new ConVar("rcbot_pathrevs", "40", 0, "");

	bot_command = new ConVar("rcbot_cmd", "", 0, "");

	bot_attack = new ConVar("rcbot_flipout", "0", 0, "");

	bot_scoutdj = new ConVar("rcbot_scoutdj", "0.28", 0, "");

	bot_anglespeed = new ConVar("rcbot_anglespeed", "0.21", 0, "");

	bot_stop = new ConVar("rcbot_stop", "0", 0, "");

	bot_waypointpathdist = new ConVar("rcbot_wpt_pathdist", "400", 0, "");

	bot_rj = new ConVar("rcbot_rj", "0.01", 0, "");

	bot_defrate = new ConVar("rcbot_defrate", "0.24", 0, "");

	bot_beliefmulti = new ConVar("rcbot_beliefmulti", "20.0", 0, "");

	bot_belief_fade = new ConVar("rcbot_belief_fade", "0.75", 0, "");

	bot_change_class = new ConVar("rcbot_change_classes", "0", 0, "");

	bot_use_vc_commands = new ConVar("rcbot_voice_cmds", "1", 0, "");

	bot_use_disp_dist = new ConVar("rcbot_disp_dist", "800.0", 0, "");

	bot_max_cc_time = new ConVar("rcbot_max_cc_time", "240", 0, "");

	bot_min_cc_time = new ConVar("rcbot_min_cc_time", "60", 0, "");

	bot_avoid_radius = new ConVar("rcbot_avoid_radius", "80", 0, "");

	bot_avoid_strength = new ConVar("rcbot_avoid_strength", "100", 0, "");

	bot_messaround = new ConVar("rcbot_messaround", "1", 0, "");

	bot_heavyaimoffset = new ConVar("rcbot_heavyaimoffset", "0.1", 0, "");

	bot_aimsmoothing = new ConVar("rcbot_aimsmoothing", "1", 0, "");

	bot_bossattackfactor = new ConVar("rcbot_bossattackfactor", "1.0", 0, "");

	rcbot_enemyshootfov = new ConVar("rcbot_enemyshootfov", "0.97", 0, "");

	rcbot_enemyshoot_gravgun_fov = new ConVar("rcbot_enemyshoot_gravgun_fov", "0.98", 0, "");

	rcbot_wpt_autoradius = new ConVar("rcbot_wpt_autoradius", "0", 0, "");

	rcbot_move_sentry_time = new ConVar("rcbot_move_sentry_time", "120", 0, "");

	rcbot_move_sentry_kpm = new ConVar("rcbot_move_sentry_kpm", "1", 0, "");

	rcbot_smoke_time = new ConVar("rcbot_smoke_time", "10", 0, "");

	rcbot_move_disp_time = new ConVar("rcbot_move_disp_time", "120", 0, "");

	rcbot_move_disp_healamount = new ConVar("rcbot_move_disp_healamount", "100", 0, "");

	rcbot_demo_runup_dist = new ConVar("rcbot_demo_runup", "99.0", 0, "");

	rcbot_demo_jump = new ConVar("rcbot_enable_pipejump", "1", 0, "");

	rcbot_move_tele_time = new ConVar("rcbot_move_tele_time", "120", 0, "");

	rcbot_move_tele_tpm = new ConVar("rcbot_move_tele_tpm", "1", 0, "");

	rcbot_tf2_protect_cap_time = new ConVar("rcbot_tf2_prot_cap_time", "12.5", 0, "");

	rcbot_tf2_protect_cap_percent = new ConVar("rcbot_tf2_protect_cap_percent", "0.25", 0, "");

	rcbot_tf2_spy_kill_on_cap_dist = new ConVar("rcbot_tf2_spy_kill_on_cap_dist", "200.0", 0, "");

	rcbot_move_dist = new ConVar("rcbot_move_dist", "800", 0, "");

	rcbot_shoot_breakables = new ConVar("rcbot_shoot_breakables", "1", 0, "");

	rcbot_shoot_breakable_dist = new ConVar("rcbot_shoot_breakable_dist", "128.0", 0, "");

	rcbot_move_obj = new ConVar("rcbot_move_obj", "1", 0, "");

	rcbot_taunt = new ConVar("rcbot_taunt", "0", 0, "");

	bot_highfive = new ConVar("rcbot_highfive", "1", 0, "");

	rcbot_notarget = new ConVar("rcbot_notarget", "0", 0, "");

	rcbot_nocapturing = new ConVar("rcbot_dontcapture", "0", 0, "");

	rcbot_jump_obst_dist = new ConVar("rcbot_jump_obst_dist", "80", 0, "");

	rcbot_jump_obst_speed = new ConVar("rcbot_jump_obst_speed", "100", 0, "");

	rcbot_speed_boost = new ConVar("rcbot_speed_boost", "1", 0, "");

	rcbot_melee_only = new ConVar("rcbot_melee_only", "0", 0, "");

	rcbot_debug_iglev = new ConVar("rcbot_debug_iglev", "0", 0, "");

	rcbot_dont_move = new ConVar("rcbot_dontmove", "0", 0, "");

	rcbot_runplayercmd_dods = new ConVar("rcbot_runplayer_cmd_dods", "417", 0, "");

	rcbot_ladder_offs = new ConVar("rcbot_ladder_offs", "42", 0, "");

	rcbot_ffa = new ConVar("rcbot_ffa", "0", 0, "");

	rcbot_prone_enemy_only = new ConVar("rcbot_prone_enemy_only", "1", 0, "");

	rcbot_menu_update_time1 = new ConVar("rcbot_menu_update_time1", "0.04", 0, "");

	rcbot_menu_update_time2 = new ConVar("rcbot_menu_update_time2", "0.2", 0, "");

	rcbot_autowaypoint_dist = new ConVar("rcbot_autowpt_dist", "150.0", 0, "");

	rcbot_squad_idle_time = new ConVar("rcbot_squad_idle_time", "3.0", 0, "");

	rcbot_bots_form_squads = new ConVar("rcbot_bots_form_squads", "1", 0, "");

	rcbot_listen_dist = new ConVar("rcbot_listen_dist", "512", 0, "");

	rcbot_footstep_speed = new ConVar("rcbot_footstep_speed", "250", 0, "");

	rcbot_bot_squads_percent = new ConVar("rcbot_bot_squads_percent", "50", 0, "");

	rcbot_tooltips = new ConVar("rcbot_tooltips", "1", 0, "");

	rcbot_debug_notasks = new ConVar("rcbot_debug_notasks", "0", 0, "");

	rcbot_debug_dont_shoot = new ConVar("rcbot_debug_dont_shoot", "0", 0, "");

	rcbot_debug_show_route = new ConVar("rcbot_debug_show_route", "0", 0, "");

	rcbot_tf2_autoupdate_point_time = new ConVar("rcbot_tf2_autoupdate_point_time", "60", 0, "");

	rcbot_spy_runaway_health = new ConVar("rcbot_spy_runaway_health", "70", 0, "");

	rcbot_supermode = new ConVar("rcbot_supermode", "0", 0, "");

	rcbot_addbottime = new ConVar("rcbot_addbottime", "0", 0, "");

	rcbot_hijack_afk_time = new ConVar("rcbot_hijack_afk_time", "0", 0, "");

	rcbot_gamerules_offset = new ConVar("rcbot_gamerules_offset", "5", 0, "");

	rcbot_bot_quota_interval = new ConVar("rcbot_bot_quota_interval", "10", 0, "");

	rcbot_show_welcome_msg = new ConVar("rcbot_show_welcome_msg", "1", 0, "");

	rcbot_force_class = new ConVar("rcbot_force_class", "0", 0, "");

	rcbot_process_usercmds_offset = new ConVar("rcbot_process_usercmds_offset", "429", 0, "");

	rcbot_mvm_revive_markers = new ConVar("rcbot_mvm_revive_markers", "1", 0, "");

	rcbot_loglevel = new ConVar("rcbot_loglevel", "0", 0, "");

	rcbot_const_point_master_offset = new ConVar("rcbot_const_point_master_offset", "0", 0, "");

	rcbot_shoot_breakable_cos = new ConVar("rcbot_shoot_breakable_cos", "0", 0, "");

	rcbot_stats_inrange_dist = new ConVar("rcbot_stats_inrange_dist", "0", 0, "");

	rcbot_tf2_payload_dist_retreat = new ConVar("rcbot_tf2_payload_dist_retreat", "0", 0, "");

	rcbot_wpt_autotype = new ConVar("rcbot_wpt_autotype", "0", 0, "");

	rcbot_wptplace_width = new ConVar("rcbot_wptplace_width", "0", 0, "");

	g_bCvarsReady = true;
}

void RCBOT2_Cvar_Unlink(ICvar *cvar)
{
	if (rcbot_tf2_debug_spies_cloakdisguise)
		cvar->UnregisterConCommand(rcbot_tf2_debug_spies_cloakdisguise);
	if (rcbot_tf2_medic_letgotime)
		cvar->UnregisterConCommand(rcbot_tf2_medic_letgotime);
	if (rcbot_tf2_pyro_airblast)
		cvar->UnregisterConCommand(rcbot_tf2_pyro_airblast);
	if (rcbot_projectile_tweak)
		cvar->UnregisterConCommand(rcbot_projectile_tweak);
	if (bot_cmd_enable_wpt_sounds)
		cvar->UnregisterConCommand(bot_cmd_enable_wpt_sounds);
	if (bot_general_difficulty)
		cvar->UnregisterConCommand(bot_general_difficulty);
	if (bot_visrevs_clients)
		cvar->UnregisterConCommand(bot_visrevs_clients);
	if (bot_spyknifefov)
		cvar->UnregisterConCommand(bot_spyknifefov);
	if (bot_visrevs)
		cvar->UnregisterConCommand(bot_visrevs);
	if (bot_pathrevs)
		cvar->UnregisterConCommand(bot_pathrevs);
	if (bot_command)
		cvar->UnregisterConCommand(bot_command);
	if (bot_attack)
		cvar->UnregisterConCommand(bot_attack);
	if (bot_scoutdj)
		cvar->UnregisterConCommand(bot_scoutdj);
	if (bot_anglespeed)
		cvar->UnregisterConCommand(bot_anglespeed);
	if (bot_stop)
		cvar->UnregisterConCommand(bot_stop);
	if (bot_waypointpathdist)
		cvar->UnregisterConCommand(bot_waypointpathdist);
	if (bot_rj)
		cvar->UnregisterConCommand(bot_rj);
	if (bot_defrate)
		cvar->UnregisterConCommand(bot_defrate);
	if (bot_beliefmulti)
		cvar->UnregisterConCommand(bot_beliefmulti);
	if (bot_belief_fade)
		cvar->UnregisterConCommand(bot_belief_fade);
	if (bot_change_class)
		cvar->UnregisterConCommand(bot_change_class);
	if (bot_use_vc_commands)
		cvar->UnregisterConCommand(bot_use_vc_commands);
	if (bot_use_disp_dist)
		cvar->UnregisterConCommand(bot_use_disp_dist);
	if (bot_max_cc_time)
		cvar->UnregisterConCommand(bot_max_cc_time);
	if (bot_min_cc_time)
		cvar->UnregisterConCommand(bot_min_cc_time);
	if (bot_avoid_radius)
		cvar->UnregisterConCommand(bot_avoid_radius);
	if (bot_avoid_strength)
		cvar->UnregisterConCommand(bot_avoid_strength);
	if (bot_messaround)
		cvar->UnregisterConCommand(bot_messaround);
	if (bot_heavyaimoffset)
		cvar->UnregisterConCommand(bot_heavyaimoffset);
	if (bot_aimsmoothing)
		cvar->UnregisterConCommand(bot_aimsmoothing);
	if (bot_bossattackfactor)
		cvar->UnregisterConCommand(bot_bossattackfactor);
	if (rcbot_enemyshootfov)
		cvar->UnregisterConCommand(rcbot_enemyshootfov);
	if (rcbot_enemyshoot_gravgun_fov)
		cvar->UnregisterConCommand(rcbot_enemyshoot_gravgun_fov);
	if (rcbot_wpt_autoradius)
		cvar->UnregisterConCommand(rcbot_wpt_autoradius);
	if (rcbot_move_sentry_time)
		cvar->UnregisterConCommand(rcbot_move_sentry_time);
	if (rcbot_move_sentry_kpm)
		cvar->UnregisterConCommand(rcbot_move_sentry_kpm);
	if (rcbot_smoke_time)
		cvar->UnregisterConCommand(rcbot_smoke_time);
	if (rcbot_move_disp_time)
		cvar->UnregisterConCommand(rcbot_move_disp_time);
	if (rcbot_move_disp_healamount)
		cvar->UnregisterConCommand(rcbot_move_disp_healamount);
	if (rcbot_demo_runup_dist)
		cvar->UnregisterConCommand(rcbot_demo_runup_dist);
	if (rcbot_demo_jump)
		cvar->UnregisterConCommand(rcbot_demo_jump);
	if (rcbot_move_tele_time)
		cvar->UnregisterConCommand(rcbot_move_tele_time);
	if (rcbot_move_tele_tpm)
		cvar->UnregisterConCommand(rcbot_move_tele_tpm);
	if (rcbot_tf2_protect_cap_time)
		cvar->UnregisterConCommand(rcbot_tf2_protect_cap_time);
	if (rcbot_tf2_protect_cap_percent)
		cvar->UnregisterConCommand(rcbot_tf2_protect_cap_percent);
	if (rcbot_tf2_spy_kill_on_cap_dist)
		cvar->UnregisterConCommand(rcbot_tf2_spy_kill_on_cap_dist);
	if (rcbot_move_dist)
		cvar->UnregisterConCommand(rcbot_move_dist);
	if (rcbot_shoot_breakables)
		cvar->UnregisterConCommand(rcbot_shoot_breakables);
	if (rcbot_shoot_breakable_dist)
		cvar->UnregisterConCommand(rcbot_shoot_breakable_dist);
	if (rcbot_move_obj)
		cvar->UnregisterConCommand(rcbot_move_obj);
	if (rcbot_taunt)
		cvar->UnregisterConCommand(rcbot_taunt);
	if (bot_highfive)
		cvar->UnregisterConCommand(bot_highfive);
	if (rcbot_notarget)
		cvar->UnregisterConCommand(rcbot_notarget);
	if (rcbot_nocapturing)
		cvar->UnregisterConCommand(rcbot_nocapturing);
	if (rcbot_jump_obst_dist)
		cvar->UnregisterConCommand(rcbot_jump_obst_dist);
	if (rcbot_jump_obst_speed)
		cvar->UnregisterConCommand(rcbot_jump_obst_speed);
	if (rcbot_speed_boost)
		cvar->UnregisterConCommand(rcbot_speed_boost);
	if (rcbot_melee_only)
		cvar->UnregisterConCommand(rcbot_melee_only);
	if (rcbot_debug_iglev)
		cvar->UnregisterConCommand(rcbot_debug_iglev);
	if (rcbot_dont_move)
		cvar->UnregisterConCommand(rcbot_dont_move);
	if (rcbot_runplayercmd_dods)
		cvar->UnregisterConCommand(rcbot_runplayercmd_dods);
	if (rcbot_ladder_offs)
		cvar->UnregisterConCommand(rcbot_ladder_offs);
	if (rcbot_ffa)
		cvar->UnregisterConCommand(rcbot_ffa);
	if (rcbot_prone_enemy_only)
		cvar->UnregisterConCommand(rcbot_prone_enemy_only);
	if (rcbot_menu_update_time1)
		cvar->UnregisterConCommand(rcbot_menu_update_time1);
	if (rcbot_menu_update_time2)
		cvar->UnregisterConCommand(rcbot_menu_update_time2);
	if (rcbot_autowaypoint_dist)
		cvar->UnregisterConCommand(rcbot_autowaypoint_dist);
	if (rcbot_squad_idle_time)
		cvar->UnregisterConCommand(rcbot_squad_idle_time);
	if (rcbot_bots_form_squads)
		cvar->UnregisterConCommand(rcbot_bots_form_squads);
	if (rcbot_listen_dist)
		cvar->UnregisterConCommand(rcbot_listen_dist);
	if (rcbot_footstep_speed)
		cvar->UnregisterConCommand(rcbot_footstep_speed);
	if (rcbot_bot_squads_percent)
		cvar->UnregisterConCommand(rcbot_bot_squads_percent);
	if (rcbot_tooltips)
		cvar->UnregisterConCommand(rcbot_tooltips);
	if (rcbot_debug_notasks)
		cvar->UnregisterConCommand(rcbot_debug_notasks);
	if (rcbot_debug_dont_shoot)
		cvar->UnregisterConCommand(rcbot_debug_dont_shoot);
	if (rcbot_debug_show_route)
		cvar->UnregisterConCommand(rcbot_debug_show_route);
	if (rcbot_tf2_autoupdate_point_time)
		cvar->UnregisterConCommand(rcbot_tf2_autoupdate_point_time);
	if (rcbot_spy_runaway_health)
		cvar->UnregisterConCommand(rcbot_spy_runaway_health);
	if (rcbot_supermode)
		cvar->UnregisterConCommand(rcbot_supermode);
	if (rcbot_addbottime)
		cvar->UnregisterConCommand(rcbot_addbottime);
	if (rcbot_hijack_afk_time)
		cvar->UnregisterConCommand(rcbot_hijack_afk_time);
	if (rcbot_gamerules_offset)
		cvar->UnregisterConCommand(rcbot_gamerules_offset);
	if (rcbot_bot_quota_interval)
		cvar->UnregisterConCommand(rcbot_bot_quota_interval);
	if (rcbot_show_welcome_msg)
		cvar->UnregisterConCommand(rcbot_show_welcome_msg);
	if (rcbot_force_class)
		cvar->UnregisterConCommand(rcbot_force_class);
	if (rcbot_process_usercmds_offset)
		cvar->UnregisterConCommand(rcbot_process_usercmds_offset);
	if (rcbot_mvm_revive_markers)
		cvar->UnregisterConCommand(rcbot_mvm_revive_markers);
	if (rcbot_loglevel)
		cvar->UnregisterConCommand(rcbot_loglevel);
	if (rcbot_const_point_master_offset)
		cvar->UnregisterConCommand(rcbot_const_point_master_offset);
	if (rcbot_shoot_breakable_cos)
		cvar->UnregisterConCommand(rcbot_shoot_breakable_cos);
	if (rcbot_stats_inrange_dist)
		cvar->UnregisterConCommand(rcbot_stats_inrange_dist);
	if (rcbot_tf2_payload_dist_retreat)
		cvar->UnregisterConCommand(rcbot_tf2_payload_dist_retreat);
	if (rcbot_wpt_autotype)
		cvar->UnregisterConCommand(rcbot_wpt_autotype);
	if (rcbot_wptplace_width)
		cvar->UnregisterConCommand(rcbot_wptplace_width);
}



void RCBOT2_Cvar_Unregister()
{
	delete rcbot_tf2_debug_spies_cloakdisguise;
	rcbot_tf2_debug_spies_cloakdisguise = nullptr;
	delete rcbot_tf2_medic_letgotime;
	rcbot_tf2_medic_letgotime = nullptr;
	delete rcbot_tf2_pyro_airblast;
	rcbot_tf2_pyro_airblast = nullptr;
	delete rcbot_projectile_tweak;
	rcbot_projectile_tweak = nullptr;
	delete bot_cmd_enable_wpt_sounds;
	bot_cmd_enable_wpt_sounds = nullptr;
	delete bot_general_difficulty;
	bot_general_difficulty = nullptr;
	delete bot_visrevs_clients;
	bot_visrevs_clients = nullptr;
	delete bot_spyknifefov;
	bot_spyknifefov = nullptr;
	delete bot_visrevs;
	bot_visrevs = nullptr;
	delete bot_pathrevs;
	bot_pathrevs = nullptr;
	delete bot_command;
	bot_command = nullptr;
	delete bot_attack;
	bot_attack = nullptr;
	delete bot_scoutdj;
	bot_scoutdj = nullptr;
	delete bot_anglespeed;
	bot_anglespeed = nullptr;
	delete bot_stop;
	bot_stop = nullptr;
	delete bot_waypointpathdist;
	bot_waypointpathdist = nullptr;
	delete bot_rj;
	bot_rj = nullptr;
	delete bot_defrate;
	bot_defrate = nullptr;
	delete bot_beliefmulti;
	bot_beliefmulti = nullptr;
	delete bot_belief_fade;
	bot_belief_fade = nullptr;
	delete bot_change_class;
	bot_change_class = nullptr;
	delete bot_use_vc_commands;
	bot_use_vc_commands = nullptr;
	delete bot_use_disp_dist;
	bot_use_disp_dist = nullptr;
	delete bot_max_cc_time;
	bot_max_cc_time = nullptr;
	delete bot_min_cc_time;
	bot_min_cc_time = nullptr;
	delete bot_avoid_radius;
	bot_avoid_radius = nullptr;
	delete bot_avoid_strength;
	bot_avoid_strength = nullptr;
	delete bot_messaround;
	bot_messaround = nullptr;
	delete bot_heavyaimoffset;
	bot_heavyaimoffset = nullptr;
	delete bot_aimsmoothing;
	bot_aimsmoothing = nullptr;
	delete bot_bossattackfactor;
	bot_bossattackfactor = nullptr;
	delete rcbot_enemyshootfov;
	rcbot_enemyshootfov = nullptr;
	delete rcbot_enemyshoot_gravgun_fov;
	rcbot_enemyshoot_gravgun_fov = nullptr;
	delete rcbot_wpt_autoradius;
	rcbot_wpt_autoradius = nullptr;
	delete rcbot_move_sentry_time;
	rcbot_move_sentry_time = nullptr;
	delete rcbot_move_sentry_kpm;
	rcbot_move_sentry_kpm = nullptr;
	delete rcbot_smoke_time;
	rcbot_smoke_time = nullptr;
	delete rcbot_move_disp_time;
	rcbot_move_disp_time = nullptr;
	delete rcbot_move_disp_healamount;
	rcbot_move_disp_healamount = nullptr;
	delete rcbot_demo_runup_dist;
	rcbot_demo_runup_dist = nullptr;
	delete rcbot_demo_jump;
	rcbot_demo_jump = nullptr;
	delete rcbot_move_tele_time;
	rcbot_move_tele_time = nullptr;
	delete rcbot_move_tele_tpm;
	rcbot_move_tele_tpm = nullptr;
	delete rcbot_tf2_protect_cap_time;
	rcbot_tf2_protect_cap_time = nullptr;
	delete rcbot_tf2_protect_cap_percent;
	rcbot_tf2_protect_cap_percent = nullptr;
	delete rcbot_tf2_spy_kill_on_cap_dist;
	rcbot_tf2_spy_kill_on_cap_dist = nullptr;
	delete rcbot_move_dist;
	rcbot_move_dist = nullptr;
	delete rcbot_shoot_breakables;
	rcbot_shoot_breakables = nullptr;
	delete rcbot_shoot_breakable_dist;
	rcbot_shoot_breakable_dist = nullptr;
	delete rcbot_move_obj;
	rcbot_move_obj = nullptr;
	delete rcbot_taunt;
	rcbot_taunt = nullptr;
	delete bot_highfive;
	bot_highfive = nullptr;
	delete rcbot_notarget;
	rcbot_notarget = nullptr;
	delete rcbot_nocapturing;
	rcbot_nocapturing = nullptr;
	delete rcbot_jump_obst_dist;
	rcbot_jump_obst_dist = nullptr;
	delete rcbot_jump_obst_speed;
	rcbot_jump_obst_speed = nullptr;
	delete rcbot_speed_boost;
	rcbot_speed_boost = nullptr;
	delete rcbot_melee_only;
	rcbot_melee_only = nullptr;
	delete rcbot_debug_iglev;
	rcbot_debug_iglev = nullptr;
	delete rcbot_dont_move;
	rcbot_dont_move = nullptr;
	delete rcbot_runplayercmd_dods;
	rcbot_runplayercmd_dods = nullptr;
	delete rcbot_ladder_offs;
	rcbot_ladder_offs = nullptr;
	delete rcbot_ffa;
	rcbot_ffa = nullptr;
	delete rcbot_prone_enemy_only;
	rcbot_prone_enemy_only = nullptr;
	delete rcbot_menu_update_time1;
	rcbot_menu_update_time1 = nullptr;
	delete rcbot_menu_update_time2;
	rcbot_menu_update_time2 = nullptr;
	delete rcbot_autowaypoint_dist;
	rcbot_autowaypoint_dist = nullptr;
	delete rcbot_squad_idle_time;
	rcbot_squad_idle_time = nullptr;
	delete rcbot_bots_form_squads;
	rcbot_bots_form_squads = nullptr;
	delete rcbot_listen_dist;
	rcbot_listen_dist = nullptr;
	delete rcbot_footstep_speed;
	rcbot_footstep_speed = nullptr;
	delete rcbot_bot_squads_percent;
	rcbot_bot_squads_percent = nullptr;
	delete rcbot_tooltips;
	rcbot_tooltips = nullptr;
	delete rcbot_debug_notasks;
	rcbot_debug_notasks = nullptr;
	delete rcbot_debug_dont_shoot;
	rcbot_debug_dont_shoot = nullptr;
	delete rcbot_debug_show_route;
	rcbot_debug_show_route = nullptr;
	delete rcbot_tf2_autoupdate_point_time;
	rcbot_tf2_autoupdate_point_time = nullptr;
	delete rcbot_spy_runaway_health;
	rcbot_spy_runaway_health = nullptr;
	delete rcbot_supermode;
	rcbot_supermode = nullptr;
	delete rcbot_addbottime;
	rcbot_addbottime = nullptr;
	delete rcbot_hijack_afk_time;
	rcbot_hijack_afk_time = nullptr;
	delete rcbot_gamerules_offset;
	rcbot_gamerules_offset = nullptr;
	delete rcbot_bot_quota_interval;
	rcbot_bot_quota_interval = nullptr;
	delete rcbot_show_welcome_msg;
	rcbot_show_welcome_msg = nullptr;
	delete rcbot_force_class;
	rcbot_force_class = nullptr;
	delete rcbot_process_usercmds_offset;
	rcbot_process_usercmds_offset = nullptr;
	delete rcbot_mvm_revive_markers;
	rcbot_mvm_revive_markers = nullptr;
	delete rcbot_loglevel;
	rcbot_loglevel = nullptr;
	delete rcbot_const_point_master_offset;
	rcbot_const_point_master_offset = nullptr;
	delete rcbot_shoot_breakable_cos;
	rcbot_shoot_breakable_cos = nullptr;
	delete rcbot_stats_inrange_dist;
	rcbot_stats_inrange_dist = nullptr;
	delete rcbot_tf2_payload_dist_retreat;
	rcbot_tf2_payload_dist_retreat = nullptr;
	delete rcbot_wpt_autotype;
	rcbot_wpt_autotype = nullptr;
	delete rcbot_wptplace_width;
	rcbot_wptplace_width = nullptr;
}
#pragma GCC visibility pop
