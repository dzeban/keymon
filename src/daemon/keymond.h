/*
 * Keymon - keyboard monitor and statistic system.
 * See details on http://reduct.ru/~avd/keymon
 *
 * Copyright (C) 2013  Alex Dzyoba <avd@reduct.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <netlink/netlink.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>

#define KEYMON_GENL_FAMILY_NAME "keymon"
#define KEYMON_MC_GROUP_NAME "keymon_mc_group"

#define KEYMON_GENL_VERSION 1

// -----------------------------------------------------------------------------
// Keymon generic netlink commands.
//
// First and last commands are special cases used for validation and must not be
// changed. Don't change the order. New commands is added in between.
// -----------------------------------------------------------------------------
enum keymon_genl_commands {
	__KEYMON_GENL_CMD_UNSPEC = 0,

	KEYMON_GENL_CMD_NOTIFY,   // Outcoming keyboard notify command

	__KEYMON_GENL_CMD_LAST,
	KEYMON_GENL_CMD_MAX = __KEYMON_GENL_CMD_LAST - 1
};

// -----------------------------------------------------------------------------
// Keymon generic netlink attributes. 
//
// First and last attributes are special cases used for validation and must not
// be changed. Don't change the order. To add new attribute - insert in between 
// and update keymon_nla_policy.
// -----------------------------------------------------------------------------
enum keymon_genl_attrs {
	__KEYMON_GENL_ATTR_UNSPEC = 0,

	KEYMON_GENL_ATTR_NOTIFICATION, // FIXME: String with notification

	__KEYMON_GENL_ATTR_LAST,
	KEYMON_GENL_ATTR_MAX = __KEYMON_GENL_ATTR_LAST - 1
};

// -----------------------------------------------------------------------------
// Attributes policy. 
// This is used by generic netlink contoller to validate our attributes
// -----------------------------------------------------------------------------
struct nla_policy keymon_nla_policy[ KEYMON_GENL_ATTR_MAX + 1 ] = {
	[ KEYMON_GENL_ATTR_NOTIFICATION ] = { .type = NLA_STRING }
};
