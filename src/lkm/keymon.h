/*
 *	Copyright (C) 2013 Alex Dzyoba <avd@reduct.ru>
 *
 *	This file is part of Keymon.
 *	http://reduct.ru/~avd/keymon
 *
 *	Keymon is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	
 *	Keymon is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with Keymon.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/version.h>  /* KERNEL_VERSION macro */
#include <linux/init.h>		/* Needed for the init macros */

#include <net/netlink.h>    /* Common Netlink API */
#include <net/genetlink.h>  /* Special Generic Netlink API */

#include <linux/keyboard.h> /* Needed for                   */
#include <linux/notifier.h> /*            keyboard notifier */

#define km_log(fmt, args...) printk(KERN_DEBUG "Keymon: In %s:%d. " fmt, __FUNCTION__, __LINE__, ## args)

static int keymon_genl_register_cmd( struct sk_buff *skb, struct genl_info *info );
static int keymon_genl_notify_dump( struct sk_buff *skb, struct netlink_callback *cb );
static int keymon_kb_nf_cb( struct notifier_block *nb, unsigned long code, void *_param );

// -----------------------------------------------------------------------------
// Keymon generic netlink commands.
//
// First and last commands are special cases used for validation and must not be
// changed. Don't change the order. New commands is added in between.
// -----------------------------------------------------------------------------
enum keymon_genl_commands {
	__KEYMON_GENL_CMD_UNSPEC = 0,

	KEYMON_GENL_CMD_REGISTER, // Incoming daemon registration command
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

	KEYMON_GENL_ATTR_PID, // Integer of daemon PID to send netlink messages

	__KEYMON_GENL_ATTR_LAST,
	KEYMON_GENL_ATTR_MAX = __KEYMON_GENL_ATTR_LAST - 1
};

// -----------------------------------------------------------------------------
// Attributes policy. 
// This is used by generic netlink contoller to validate our attributes
// -----------------------------------------------------------------------------
struct nla_policy keymon_nla_policy[ KEYMON_GENL_ATTR_MAX + 1 ] = {

#if ( LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0) )
	[ KEYMON_GENL_ATTR_PID ] = { .type = NLA_U32 } 
#else
	[ KEYMON_GENL_ATTR_PID ] = { .type = NLA_S32 } 
#endif

};

// -----------------------------------------------------------------------------
// Keymon generic netlink family definition
// -----------------------------------------------------------------------------
#define KEYMON_GENL_VERSION 1
struct genl_family keymon_genl_family = {
	.id      = GENL_ID_GENERATE, // Generate ID 
	.hdrsize = 0, // No custom header
	.name    = "Keymon",
	.version = KEYMON_GENL_VERSION,
	.maxattr = KEYMON_GENL_ATTR_MAX
};

// -----------------------------------------------------------------------------
// Keymon generic netlink operations for generic netlink family defined above
// -----------------------------------------------------------------------------
struct genl_ops keymon_genl_ops[] = {
	{
		.cmd    = KEYMON_GENL_CMD_NOTIFY,
		.doit   = NULL,
		.dumpit = keymon_genl_notify_dump, 
		.policy = keymon_nla_policy
	},
	{
		.cmd    = KEYMON_GENL_CMD_REGISTER,
		.doit   = keymon_genl_register_cmd,
		.dumpit = NULL,
		.policy = keymon_nla_policy
	}
};


// -----------------------------------------------------------------------------
// Keyboard notifier
// -----------------------------------------------------------------------------
static struct notifier_block keymon_kb_nf = {
	.notifier_call = keymon_kb_nf_cb
};
