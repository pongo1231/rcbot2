#include "build_info.h"

// generated with versioning/generate.py, AMBuild handles this
#include <build_version_auto.h>

const char *build_info::authors       = "Cheeseh, RoboCop, nosoop";
const char *build_info::url           = "http://rcbot.bots-united.com/";

const char *build_info::long_version  = "nosoop/rcbot2@" GIT_COMMIT_HASH;
const char *build_info::short_version = "nosoop@" GIT_COMMIT_SHORT_HASH;

const char *build_info::date          = __DATE__;