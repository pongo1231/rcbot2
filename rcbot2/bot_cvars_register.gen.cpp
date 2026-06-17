// AUTO-GENERATED — do not edit manually
#include "icvar.h"
#include "convar.h"
#include "bot_cvars.h"


void RCBOT2_Cvar_Register(ICvar *cvar)
{
	rcbot_tf2_debug_spies_cloakdisguise = cvar->FindVar("rcbot_tf2_debug_spies_cloakdisguise");
	if (!rcbot_tf2_debug_spies_cloakdisguise)
		rcbot_tf2_debug_spies_cloakdisguise = new ConVar("rcbot_tf2_debug_spies_cloakdisguise", "0", 0, ""); // placeholder

	rcbot_tf2_medic_letgotime = cvar->FindVar("rcbot_tf2_medic_letgotime");
	if (!rcbot_tf2_medic_letgotime)
		rcbot_tf2_medic_letgotime = new ConVar("rcbot_tf2_medic_letgotime", "0", 0, ""); // placeholder

	rcbot_tf2_pyro_airblast = cvar->FindVar("rcbot_tf2_pyro_airblast_ammo");
	if (!rcbot_tf2_pyro_airblast)
		rcbot_tf2_pyro_airblast = new ConVar("rcbot_tf2_pyro_airblast_ammo", "0", 0, ""); // placeholder

	rcbot_projectile_tweak = cvar->FindVar("rcbot_projtweak");
	if (!rcbot_projectile_tweak)
		rcbot_projectile_tweak = new ConVar("rcbot_projtweak", "0", 0, ""); // placeholder

	bot_cmd_enable_wpt_sounds = cvar->FindVar("rcbot_enable_wpt_sounds");
	if (!bot_cmd_enable_wpt_sounds)
		bot_cmd_enable_wpt_sounds = new ConVar("rcbot_enable_wpt_sounds", "0", 0, ""); // placeholder

	bot_general_difficulty = cvar->FindVar("rcbot_skill");
	if (!bot_general_difficulty)
		bot_general_difficulty = new ConVar("rcbot_skill", "0", 0, ""); // placeholder

	bot_visrevs_clients = cvar->FindVar("rcbot_visrevs_clients");
	if (!bot_visrevs_clients)
		bot_visrevs_clients = new ConVar("rcbot_visrevs_clients", "0", 0, ""); // placeholder

	bot_spyknifefov = cvar->FindVar("rcbot_spyknifefov");
	if (!bot_spyknifefov)
		bot_spyknifefov = new ConVar("rcbot_spyknifefov", "0", 0, ""); // placeholder

	bot_visrevs = cvar->FindVar("rcbot_visrevs");
	if (!bot_visrevs)
		bot_visrevs = new ConVar("rcbot_visrevs", "0", 0, ""); // placeholder

	bot_pathrevs = cvar->FindVar("rcbot_pathrevs");
	if (!bot_pathrevs)
		bot_pathrevs = new ConVar("rcbot_pathrevs", "0", 0, ""); // placeholder

	bot_command = cvar->FindVar("rcbot_cmd");
	if (!bot_command)
		bot_command = new ConVar("rcbot_cmd", "0", 0, ""); // placeholder

	bot_attack = cvar->FindVar("rcbot_flipout");
	if (!bot_attack)
		bot_attack = new ConVar("rcbot_flipout", "0", 0, ""); // placeholder

	bot_scoutdj = cvar->FindVar("rcbot_scoutdj");
	if (!bot_scoutdj)
		bot_scoutdj = new ConVar("rcbot_scoutdj", "0", 0, ""); // placeholder

	bot_anglespeed = cvar->FindVar("rcbot_anglespeed");
	if (!bot_anglespeed)
		bot_anglespeed = new ConVar("rcbot_anglespeed", "0", 0, ""); // placeholder

	bot_stop = cvar->FindVar("rcbot_stop");
	if (!bot_stop)
		bot_stop = new ConVar("rcbot_stop", "0", 0, ""); // placeholder

	bot_waypointpathdist = cvar->FindVar("rcbot_wpt_pathdist");
	if (!bot_waypointpathdist)
		bot_waypointpathdist = new ConVar("rcbot_wpt_pathdist", "0", 0, ""); // placeholder

	bot_rj = cvar->FindVar("rcbot_rj");
	if (!bot_rj)
		bot_rj = new ConVar("rcbot_rj", "0", 0, ""); // placeholder

	bot_defrate = cvar->FindVar("rcbot_defrate");
	if (!bot_defrate)
		bot_defrate = new ConVar("rcbot_defrate", "0", 0, ""); // placeholder

	bot_beliefmulti = cvar->FindVar("rcbot_beliefmulti");
	if (!bot_beliefmulti)
		bot_beliefmulti = new ConVar("rcbot_beliefmulti", "0", 0, ""); // placeholder

	bot_belief_fade = cvar->FindVar("rcbot_belief_fade");
	if (!bot_belief_fade)
		bot_belief_fade = new ConVar("rcbot_belief_fade", "0", 0, ""); // placeholder

	bot_change_class = cvar->FindVar("rcbot_change_classes");
	if (!bot_change_class)
		bot_change_class = new ConVar("rcbot_change_classes", "0", 0, ""); // placeholder

	bot_use_vc_commands = cvar->FindVar("rcbot_voice_cmds");
	if (!bot_use_vc_commands)
		bot_use_vc_commands = new ConVar("rcbot_voice_cmds", "0", 0, ""); // placeholder

	bot_use_disp_dist = cvar->FindVar("rcbot_disp_dist");
	if (!bot_use_disp_dist)
		bot_use_disp_dist = new ConVar("rcbot_disp_dist", "0", 0, ""); // placeholder

	bot_max_cc_time = cvar->FindVar("rcbot_max_cc_time");
	if (!bot_max_cc_time)
		bot_max_cc_time = new ConVar("rcbot_max_cc_time", "0", 0, ""); // placeholder

	bot_min_cc_time = cvar->FindVar("rcbot_min_cc_time");
	if (!bot_min_cc_time)
		bot_min_cc_time = new ConVar("rcbot_min_cc_time", "0", 0, ""); // placeholder

	bot_avoid_radius = cvar->FindVar("rcbot_avoid_radius");
	if (!bot_avoid_radius)
		bot_avoid_radius = new ConVar("rcbot_avoid_radius", "0", 0, ""); // placeholder

	bot_avoid_strength = cvar->FindVar("rcbot_avoid_strength");
	if (!bot_avoid_strength)
		bot_avoid_strength = new ConVar("rcbot_avoid_strength", "0", 0, ""); // placeholder

	bot_messaround = cvar->FindVar("rcbot_messaround");
	if (!bot_messaround)
		bot_messaround = new ConVar("rcbot_messaround", "0", 0, ""); // placeholder

	bot_heavyaimoffset = cvar->FindVar("rcbot_heavyaimoffset");
	if (!bot_heavyaimoffset)
		bot_heavyaimoffset = new ConVar("rcbot_heavyaimoffset", "0", 0, ""); // placeholder

	bot_aimsmoothing = cvar->FindVar("rcbot_aimsmoothing");
	if (!bot_aimsmoothing)
		bot_aimsmoothing = new ConVar("rcbot_aimsmoothing", "0", 0, ""); // placeholder

	bot_bossattackfactor = cvar->FindVar("rcbot_bossattackfactor");
	if (!bot_bossattackfactor)
		bot_bossattackfactor = new ConVar("rcbot_bossattackfactor", "0", 0, ""); // placeholder

	rcbot_enemyshootfov = cvar->FindVar("rcbot_enemyshootfov");
	if (!rcbot_enemyshootfov)
		rcbot_enemyshootfov = new ConVar("rcbot_enemyshootfov", "0", 0, ""); // placeholder

	rcbot_enemyshoot_gravgun_fov = cvar->FindVar("rcbot_enemyshoot_gravgun_fov");
	if (!rcbot_enemyshoot_gravgun_fov)
		rcbot_enemyshoot_gravgun_fov = new ConVar("rcbot_enemyshoot_gravgun_fov", "0", 0, ""); // placeholder

	rcbot_wptplace_width = cvar->FindVar("rcbot_wpt_width");
	if (!rcbot_wptplace_width)
		rcbot_wptplace_width = new ConVar("rcbot_wpt_width", "0", 0, ""); // placeholder

	rcbot_wpt_autoradius = cvar->FindVar("rcbot_wpt_autoradius");
	if (!rcbot_wpt_autoradius)
		rcbot_wpt_autoradius = new ConVar("rcbot_wpt_autoradius", "0", 0, ""); // placeholder

	rcbot_wpt_autotype = cvar->FindVar("rcbot_wpt_autotype");
	if (!rcbot_wpt_autotype)
		rcbot_wpt_autotype = new ConVar("rcbot_wpt_autotype", "0", 0, ""); // placeholder

	rcbot_move_sentry_time = cvar->FindVar("rcbot_move_sentry_time");
	if (!rcbot_move_sentry_time)
		rcbot_move_sentry_time = new ConVar("rcbot_move_sentry_time", "0", 0, ""); // placeholder

	rcbot_move_sentry_kpm = cvar->FindVar("rcbot_move_sentry_kpm");
	if (!rcbot_move_sentry_kpm)
		rcbot_move_sentry_kpm = new ConVar("rcbot_move_sentry_kpm", "0", 0, ""); // placeholder

	rcbot_smoke_time = cvar->FindVar("rcbot_smoke_time");
	if (!rcbot_smoke_time)
		rcbot_smoke_time = new ConVar("rcbot_smoke_time", "0", 0, ""); // placeholder

	rcbot_move_disp_time = cvar->FindVar("rcbot_move_disp_time");
	if (!rcbot_move_disp_time)
		rcbot_move_disp_time = new ConVar("rcbot_move_disp_time", "0", 0, ""); // placeholder

	rcbot_move_disp_healamount = cvar->FindVar("rcbot_move_disp_healamount");
	if (!rcbot_move_disp_healamount)
		rcbot_move_disp_healamount = new ConVar("rcbot_move_disp_healamount", "0", 0, ""); // placeholder

	rcbot_demo_runup_dist = cvar->FindVar("rcbot_demo_runup");
	if (!rcbot_demo_runup_dist)
		rcbot_demo_runup_dist = new ConVar("rcbot_demo_runup", "0", 0, ""); // placeholder

	rcbot_demo_jump = cvar->FindVar("rcbot_enable_pipejump");
	if (!rcbot_demo_jump)
		rcbot_demo_jump = new ConVar("rcbot_enable_pipejump", "0", 0, ""); // placeholder

	rcbot_move_tele_time = cvar->FindVar("rcbot_move_tele_time");
	if (!rcbot_move_tele_time)
		rcbot_move_tele_time = new ConVar("rcbot_move_tele_time", "0", 0, ""); // placeholder

	rcbot_move_tele_tpm = cvar->FindVar("rcbot_move_tele_tpm");
	if (!rcbot_move_tele_tpm)
		rcbot_move_tele_tpm = new ConVar("rcbot_move_tele_tpm", "0", 0, ""); // placeholder

	rcbot_tf2_protect_cap_time = cvar->FindVar("rcbot_tf2_prot_cap_time");
	if (!rcbot_tf2_protect_cap_time)
		rcbot_tf2_protect_cap_time = new ConVar("rcbot_tf2_prot_cap_time", "0", 0, ""); // placeholder

	rcbot_tf2_protect_cap_percent = cvar->FindVar("rcbot_tf2_protect_cap_percent");
	if (!rcbot_tf2_protect_cap_percent)
		rcbot_tf2_protect_cap_percent = new ConVar("rcbot_tf2_protect_cap_percent", "0", 0, ""); // placeholder

	rcbot_tf2_spy_kill_on_cap_dist = cvar->FindVar("rcbot_tf2_spy_kill_on_cap_dist");
	if (!rcbot_tf2_spy_kill_on_cap_dist)
		rcbot_tf2_spy_kill_on_cap_dist = new ConVar("rcbot_tf2_spy_kill_on_cap_dist", "0", 0, ""); // placeholder

	rcbot_move_dist = cvar->FindVar("rcbot_move_dist");
	if (!rcbot_move_dist)
		rcbot_move_dist = new ConVar("rcbot_move_dist", "0", 0, ""); // placeholder

	rcbot_shoot_breakables = cvar->FindVar("rcbot_shoot_breakables");
	if (!rcbot_shoot_breakables)
		rcbot_shoot_breakables = new ConVar("rcbot_shoot_breakables", "0", 0, ""); // placeholder

	rcbot_shoot_breakable_dist = cvar->FindVar("rcbot_shoot_breakable_dist");
	if (!rcbot_shoot_breakable_dist)
		rcbot_shoot_breakable_dist = new ConVar("rcbot_shoot_breakable_dist", "0", 0, ""); // placeholder

	rcbot_shoot_breakable_cos = cvar->FindVar("rcbot_shoot_breakable_cos");
	if (!rcbot_shoot_breakable_cos)
		rcbot_shoot_breakable_cos = new ConVar("rcbot_shoot_breakable_cos", "0", 0, ""); // placeholder

	rcbot_move_obj = cvar->FindVar("rcbot_move_obj");
	if (!rcbot_move_obj)
		rcbot_move_obj = new ConVar("rcbot_move_obj", "0", 0, ""); // placeholder

	rcbot_taunt = cvar->FindVar("rcbot_taunt");
	if (!rcbot_taunt)
		rcbot_taunt = new ConVar("rcbot_taunt", "0", 0, ""); // placeholder

	bot_highfive = cvar->FindVar("rcbot_highfive");
	if (!bot_highfive)
		bot_highfive = new ConVar("rcbot_highfive", "0", 0, ""); // placeholder

	rcbot_notarget = cvar->FindVar("rcbot_notarget");
	if (!rcbot_notarget)
		rcbot_notarget = new ConVar("rcbot_notarget", "0", 0, ""); // placeholder

	rcbot_nocapturing = cvar->FindVar("rcbot_dontcapture");
	if (!rcbot_nocapturing)
		rcbot_nocapturing = new ConVar("rcbot_dontcapture", "0", 0, ""); // placeholder

	rcbot_jump_obst_dist = cvar->FindVar("rcbot_jump_obst_dist");
	if (!rcbot_jump_obst_dist)
		rcbot_jump_obst_dist = new ConVar("rcbot_jump_obst_dist", "0", 0, ""); // placeholder

	rcbot_jump_obst_speed = cvar->FindVar("rcbot_jump_obst_speed");
	if (!rcbot_jump_obst_speed)
		rcbot_jump_obst_speed = new ConVar("rcbot_jump_obst_speed", "0", 0, ""); // placeholder

	rcbot_speed_boost = cvar->FindVar("rcbot_speed_boost");
	if (!rcbot_speed_boost)
		rcbot_speed_boost = new ConVar("rcbot_speed_boost", "0", 0, ""); // placeholder

	rcbot_melee_only = cvar->FindVar("rcbot_melee_only");
	if (!rcbot_melee_only)
		rcbot_melee_only = new ConVar("rcbot_melee_only", "0", 0, ""); // placeholder

	rcbot_debug_iglev = cvar->FindVar("rcbot_debug_iglev");
	if (!rcbot_debug_iglev)
		rcbot_debug_iglev = new ConVar("rcbot_debug_iglev", "0", 0, ""); // placeholder

	rcbot_dont_move = cvar->FindVar("rcbot_dontmove");
	if (!rcbot_dont_move)
		rcbot_dont_move = new ConVar("rcbot_dontmove", "0", 0, ""); // placeholder

	rcbot_runplayercmd_dods = cvar->FindVar("rcbot_runplayer_cmd_dods");
	if (!rcbot_runplayercmd_dods)
		rcbot_runplayercmd_dods = new ConVar("rcbot_runplayer_cmd_dods", "0", 0, ""); // placeholder

	rcbot_ladder_offs = cvar->FindVar("rcbot_ladder_offs");
	if (!rcbot_ladder_offs)
		rcbot_ladder_offs = new ConVar("rcbot_ladder_offs", "0", 0, ""); // placeholder

	rcbot_ffa = cvar->FindVar("rcbot_ffa");
	if (!rcbot_ffa)
		rcbot_ffa = new ConVar("rcbot_ffa", "0", 0, ""); // placeholder

	rcbot_prone_enemy_only = cvar->FindVar("rcbot_prone_enemy_only");
	if (!rcbot_prone_enemy_only)
		rcbot_prone_enemy_only = new ConVar("rcbot_prone_enemy_only", "0", 0, ""); // placeholder

	rcbot_menu_update_time1 = cvar->FindVar("rcbot_menu_update_time1");
	if (!rcbot_menu_update_time1)
		rcbot_menu_update_time1 = new ConVar("rcbot_menu_update_time1", "0", 0, ""); // placeholder

	rcbot_menu_update_time2 = cvar->FindVar("rcbot_menu_update_time2");
	if (!rcbot_menu_update_time2)
		rcbot_menu_update_time2 = new ConVar("rcbot_menu_update_time2", "0", 0, ""); // placeholder

	rcbot_autowaypoint_dist = cvar->FindVar("rcbot_autowpt_dist");
	if (!rcbot_autowaypoint_dist)
		rcbot_autowaypoint_dist = new ConVar("rcbot_autowpt_dist", "0", 0, ""); // placeholder

	rcbot_stats_inrange_dist = cvar->FindVar("rcbot_stats_inrange_dist");
	if (!rcbot_stats_inrange_dist)
		rcbot_stats_inrange_dist = new ConVar("rcbot_stats_inrange_dist", "0", 0, ""); // placeholder

	rcbot_squad_idle_time = cvar->FindVar("rcbot_squad_idle_time");
	if (!rcbot_squad_idle_time)
		rcbot_squad_idle_time = new ConVar("rcbot_squad_idle_time", "0", 0, ""); // placeholder

	rcbot_bots_form_squads = cvar->FindVar("rcbot_bots_form_squads");
	if (!rcbot_bots_form_squads)
		rcbot_bots_form_squads = new ConVar("rcbot_bots_form_squads", "0", 0, ""); // placeholder

	rcbot_listen_dist = cvar->FindVar("rcbot_listen_dist");
	if (!rcbot_listen_dist)
		rcbot_listen_dist = new ConVar("rcbot_listen_dist", "0", 0, ""); // placeholder

	rcbot_footstep_speed = cvar->FindVar("rcbot_footstep_speed");
	if (!rcbot_footstep_speed)
		rcbot_footstep_speed = new ConVar("rcbot_footstep_speed", "0", 0, ""); // placeholder

	rcbot_bot_squads_percent = cvar->FindVar("rcbot_bot_squads_percent");
	if (!rcbot_bot_squads_percent)
		rcbot_bot_squads_percent = new ConVar("rcbot_bot_squads_percent", "0", 0, ""); // placeholder

	rcbot_tooltips = cvar->FindVar("rcbot_tooltips");
	if (!rcbot_tooltips)
		rcbot_tooltips = new ConVar("rcbot_tooltips", "0", 0, ""); // placeholder

	rcbot_debug_notasks = cvar->FindVar("rcbot_debug_notasks");
	if (!rcbot_debug_notasks)
		rcbot_debug_notasks = new ConVar("rcbot_debug_notasks", "0", 0, ""); // placeholder

	rcbot_debug_dont_shoot = cvar->FindVar("rcbot_debug_dont_shoot");
	if (!rcbot_debug_dont_shoot)
		rcbot_debug_dont_shoot = new ConVar("rcbot_debug_dont_shoot", "0", 0, ""); // placeholder

	rcbot_debug_show_route = cvar->FindVar("rcbot_debug_show_route");
	if (!rcbot_debug_show_route)
		rcbot_debug_show_route = new ConVar("rcbot_debug_show_route", "0", 0, ""); // placeholder

	rcbot_tf2_autoupdate_point_time = cvar->FindVar("rcbot_tf2_autoupdate_point_time");
	if (!rcbot_tf2_autoupdate_point_time)
		rcbot_tf2_autoupdate_point_time = new ConVar("rcbot_tf2_autoupdate_point_time", "0", 0, ""); // placeholder

	rcbot_tf2_payload_dist_retreat = cvar->FindVar("rcbot_tf2_payload_dist_retreat");
	if (!rcbot_tf2_payload_dist_retreat)
		rcbot_tf2_payload_dist_retreat = new ConVar("rcbot_tf2_payload_dist_retreat", "0", 0, ""); // placeholder

	rcbot_spy_runaway_health = cvar->FindVar("rcbot_spy_runaway_health");
	if (!rcbot_spy_runaway_health)
		rcbot_spy_runaway_health = new ConVar("rcbot_spy_runaway_health", "0", 0, ""); // placeholder

	rcbot_supermode = cvar->FindVar("rcbot_supermode");
	if (!rcbot_supermode)
		rcbot_supermode = new ConVar("rcbot_supermode", "0", 0, ""); // placeholder

	rcbot_addbottime = cvar->FindVar("rcbot_addbottime");
	if (!rcbot_addbottime)
		rcbot_addbottime = new ConVar("rcbot_addbottime", "0", 0, ""); // placeholder

	rcbot_hijack_afk_time = cvar->FindVar("rcbot_hijack_afk_time");
	if (!rcbot_hijack_afk_time)
		rcbot_hijack_afk_time = new ConVar("rcbot_hijack_afk_time", "0", 0, ""); // placeholder

	rcbot_gamerules_offset = cvar->FindVar("rcbot_gamerules_offset");
	if (!rcbot_gamerules_offset)
		rcbot_gamerules_offset = new ConVar("rcbot_gamerules_offset", "0", 0, ""); // placeholder

	rcbot_bot_quota_interval = cvar->FindVar("rcbot_bot_quota_interval");
	if (!rcbot_bot_quota_interval)
		rcbot_bot_quota_interval = new ConVar("rcbot_bot_quota_interval", "0", 0, ""); // placeholder

	rcbot_show_welcome_msg = cvar->FindVar("rcbot_show_welcome_msg");
	if (!rcbot_show_welcome_msg)
		rcbot_show_welcome_msg = new ConVar("rcbot_show_welcome_msg", "0", 0, ""); // placeholder

	rcbot_force_class = cvar->FindVar("rcbot_force_class");
	if (!rcbot_force_class)
		rcbot_force_class = new ConVar("rcbot_force_class", "0", 0, ""); // placeholder

	rcbot_process_usercmds_offset = cvar->FindVar("rcbot_process_usercmds_offset");
	if (!rcbot_process_usercmds_offset)
		rcbot_process_usercmds_offset = new ConVar("rcbot_process_usercmds_offset", "0", 0, ""); // placeholder

	rcbot_mvm_revive_markers = cvar->FindVar("rcbot_mvm_revive_markers");
	if (!rcbot_mvm_revive_markers)
		rcbot_mvm_revive_markers = new ConVar("rcbot_mvm_revive_markers", "0", 0, ""); // placeholder

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

	delete rcbot_wptplace_width;
	rcbot_wptplace_width = nullptr;

	delete rcbot_wpt_autoradius;
	rcbot_wpt_autoradius = nullptr;

	delete rcbot_wpt_autotype;
	rcbot_wpt_autotype = nullptr;

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

	delete rcbot_shoot_breakable_cos;
	rcbot_shoot_breakable_cos = nullptr;

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

	delete rcbot_stats_inrange_dist;
	rcbot_stats_inrange_dist = nullptr;

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

	delete rcbot_tf2_payload_dist_retreat;
	rcbot_tf2_payload_dist_retreat = nullptr;

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

}
