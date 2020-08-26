/* selinux routines
 * Copyright (C) 2011 Avesh Agarwal <avagarwa@redhat.com>
 * Copyright (C) 2019 Andrew Cagney <cagney@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/gpl2.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include "security_selinux.h"

#include "defs.h"		/* for so_serial_t */
#include "log.h"

static int selinux_ready = 0;

void init_avc(void)
{
	if (!is_selinux_enabled()) {
		libreswan_log("selinux support is NOT enabled.");
		return;
	} else {
		libreswan_log("selinux support is enabled.");
	}

	if (avc_open(NULL, 0) == 0)
		selinux_ready = 1;
	else
		libreswan_log("selinux: could not initialize avc.");
}

int within_range(char *sl, security_context_t range)
{
	int rtn = 1;
	security_id_t slsid;
	security_id_t rangesid;
	struct av_decision avd;
	security_class_t tclass;
	access_vector_t av;

	if (!selinux_ready) {
		/* mls may not be enabled */
		dbg("selinux check failed");
		return 0;
	}

	/*
	 * * Get the sids for the sl and range contexts
	 */
	rtn = avc_context_to_sid(sl, &slsid);
	if (rtn != 0) {
		dbg("within_range: Unable to retrieve sid for sl context (%s)", sl);
		return 0;
	}
	rtn = avc_context_to_sid(range, &rangesid);
	if (rtn != 0) {
		dbg("within_range: Unable to retrieve sid for range context (%s)", range);
		return 0;
	}

	/*
	** Straight up test between sl and range
	**/
	tclass = string_to_security_class("association");
	av = string_to_av_perm(tclass, "polmatch");
	rtn = avc_has_perm(slsid, rangesid, tclass, av, NULL, &avd);
	if (rtn != 0) {
		dbg("within_range: The sl (%s) is not within range of (%s)", sl, range);
		return 0;
	}
	dbg("within_range: The sl (%s) is within range of (%s)", sl, range);
	return 1;
}
