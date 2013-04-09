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

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/version.h>  /* KERNEL_VERSION macro */
#include <linux/init.h>		/* Needed for the init macros */

#include <net/netlink.h>    /* Common Netlink API */
#include <net/genetlink.h>  /* Special Generic Netlink API */

#include <linux/keyboard.h> /* Needed for                   */
#include <linux/notifier.h> /*            keyboard notifier */

#include <linux/hardirq.h>

#include "../include/genl_def.h"

#define km_log(fmt, args...) printk(KERN_DEBUG "Keymon: In %s:%d. " fmt, __FUNCTION__, __LINE__, ## args)

// ---------------------------------------------------------------------------
// Keymon work queue.
//
// Used to defer work from keyboard notification callback 
// (which is in atomic/irq context) to kernel thread handling workqueue.
static struct workqueue_struct *keymon_wq;
#define KEYMON_WQ_NAME "keymon_wq"

// Work struct passed to handler
struct keymon_notification
{
	int down;           // Pressure of the key?
	int shift;          // Current shift mask
	int ledstate;		// Current led state
	unsigned int value; // keycode, unicode value or keysym

	struct work_struct ws;  // Workqueue item
};
// ---------------------------------------------------------------------------

static int keymon_genl_notify_dump( struct sk_buff *skb, struct netlink_callback *cb );
static int keymon_kb_nf_cb( struct notifier_block *nb, unsigned long code, void *_param );

// -----------------------------------------------------------------------------
// Keymon generic netlink family definition
// -----------------------------------------------------------------------------
struct genl_family keymon_genl_family = {
	.id      = GENL_ID_GENERATE, // Generate ID 
	.hdrsize = 0, // No custom header
	.name    = KEYMON_GENL_FAMILY_NAME,
	.version = KEYMON_GENL_VERSION,
	.maxattr = KEYMON_GENL_ATTR_MAX
};

// Multicast group for netlink family
struct genl_multicast_group keymon_mc_group = {
	.name = KEYMON_MC_GROUP_NAME
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
};

// -----------------------------------------------------------------------------
// Keyboard notifier
// -----------------------------------------------------------------------------
static struct notifier_block keymon_kb_nf = {
	.notifier_call = keymon_kb_nf_cb
};
