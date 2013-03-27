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

#include "keymon.h"

//=============================================================================
//   keymon_send_notification
//=============================================================================
/// Sends NOTIFY command over generic netlink socket.
///
/// This is actually workqueue handler that does netlink stuff in non-atomic
/// context.
///
/// @param param - parameters of notification
///
/// * Constructs netlink message for NOTIFY command (KEYMON_GENL_CMD_NOTIFY)
/// * Adds notification attribute (KEYMON_GENL_ATTR_NOTIFICATION)
/// * Sends notification to userspace daemon.
//
static void keymon_send_notification( struct work_struct *work )
{
	struct keymon_work *w = NULL;
	struct keyboard_notifier_param *param = NULL;

	struct sk_buff *skb;
	void *msg;
	char notification[256];
	int rc = 0;

	// ---------------------------------------------
	// Dereference keyboard notification parameters
	// ---------------------------------------------
	w = container_of( work, struct keymon_work, ws );
	if( !w )
	{
		km_log( "Failed to get work struct container.\n" );
		return;
	}

	param = w->kb_param;
	if( !param )
	{
		km_log( "Failed to get keyboard notification param.\n");
		return;
	}

	// ----------------------------------------------
	// Construct netlink and generic netlink headers
	// ----------------------------------------------
	skb = nlmsg_new( NLMSG_GOODSIZE, GFP_KERNEL );
	if( !skb )
	{
		km_log( "Failed to construct message\n" );
		goto fail;
	}

	msg = genlmsg_put( skb, 
	                   0,           // PID is whatever
	                   0,           // Sequence number (don't care)
	                   &keymon_genl_family,   // Pointer to family struct
	                   0,                     // Flags
	                   KEYMON_GENL_CMD_NOTIFY // Generic netlink command 
	                   );
	if( !msg )
	{
		km_log( "Failed to create generic netlink message\n" );
		goto fail;
	}

	// -------------------------------------
	// Put genetlink NOTIFICATION attribute 
	// -------------------------------------
	snprintf(notification, 256, "Down %d, shift %d, ledstate %d, value %u", 
	param->down, param->shift, param->ledstate, param->value );

	rc = nla_put_string(skb, KEYMON_GENL_ATTR_NOTIFICATION, notification);
	if( rc )
	{
		km_log( "Failed to construct notification. rc = %d\n", rc);
		goto fail;
	}

	// --------------------------
	// Finalize and send message
	// --------------------------
	genlmsg_end( skb, msg );

	rc = genlmsg_multicast_allns( skb, 0, keymon_mc_group.id, GFP_KERNEL );

	// If error - fail.
	// ESRCH is forever alone case - no one is listening for our messages 
	// and it's ok, since userspace daemon can be unloaded.
	if( rc && rc != -ESRCH ) 
	{
		km_log( "Failed to send message. rc = %d\n", rc );
		goto fail;
	}

	km_log( "Notification successfully sent!\n" );

fail:
	// Need this to free work struct
	kfree( w );
	return;
}

//=============================================================================
//   keymon_kb_nf_cb 
//=============================================================================
/// Keymon keyboard notifier callback.
///
/// @param nb     - notifier block. Not used
/// @param code   - code of the event
/// @param _param - parameters of notification
///
/// Checks keyboard notification and sends packed notification to userspace 
/// daemon via netlink socket. 
///
/// Note: keyboard notifier is run in interrupt/atomic context so we cannot block
/// here. Therefore we returns NOTIFY_DONE as soon as possible in all cases
/// except one when notification itself is incorrect.
///
/// @return NOTIFY_DONE on success and NOTIFY_BAD on error.
static int keymon_kb_nf_cb( struct notifier_block *nb, unsigned long code, void *_param )
{
	struct keyboard_notifier_param *param = NULL;
	struct keymon_work *w = NULL;

	param = (struct keyboard_notifier_param *)_param;

	if( !param )
	{
		km_log("Bad keyboard notification\n");
		return NOTIFY_BAD;
	}

	// 
	// NOTE: Little portion of common sense below.
	//
	// Single press of keyboard button generates up to 6 notifications.
	//
	// There are 2 major event types:
	// 	- keycode
	// 	- keysym
	// This events is stored in code argument (some systems also send 
	// "post keysym" event but we really don't care about it.
	// 
	// Each of this events (keycode, keysym, post_keysym) is send both on key
	// down and key up. That's where we got 6 notifications and that's really
	// too many info.
	//
	// So here we will look only for keycodes of key down events. We don't need
	// to know about keysym, cause we intersted in physical buttons not symbols.
	//
	
	// param->down can have values 0 for key up, 1 for key down and 2 for key
	// hold.
	//
	// param->shift is responsible for keyboard modifier keys like Shift(1),
	// Control(4) and Alt(8). Combination of modifier keys leads to mask sum.
	// For example, Ctrl+Shift+t combination will have param->shift mask 1+4=5.
	//
	// param->value is value of event depending of event type. For keycode it's
	// a keycode, for keysym it's a keysym (oh, thank you, Captain Hindsight).
	//
	// param->ledstate is mask for keyboard LED states. 
	// Scrool Lock is 1, Num Lock is 2, Caps Lock is 4. 

	if( param->down && code == KBD_KEYCODE )
	{
		km_log("Down %d, shift %d, ledstate %d, value %u\n", 
		param->down, param->shift, param->ledstate, param->value );

		// ---------------------------------------------------------
		// Submit notification work.
		// (We can't send notification here in atomic/irq context).
		// ---------------------------------------------------------
		w = (struct keymon_work *)kzalloc( sizeof(struct keymon_work), GFP_ATOMIC );
		if( !w )
		{
			km_log( "Failed to submit notification to workqueue\n" );
			return NOTIFY_BAD; // FIXME: Does keyboard notifier cares about our problems?
		}

		INIT_WORK( &w->ws, keymon_send_notification );
		w->kb_param = param; // Save notification data

		queue_work( keymon_wq, &w->ws );
		
	}

	return NOTIFY_DONE;
}

//=============================================================================
//   keymon_genl_notify_dump
//=============================================================================
/// Currently this is just stub that does nothing. 
/// FIXME: This should return last intercepted keycodes on dump request
/// @param skb  - socket buffer
/// @param info - generic netlink info
/// @return 0 on success, error code otherwise
static int keymon_genl_notify_dump( struct sk_buff *skb, struct netlink_callback *cb )
{
	return 0;
}

//=============================================================================
//   cleanup
//=============================================================================
/// Does the cleanup.
/// Unregisters generic netlink family, unregisters keyboard notifier and
/// destroys workqueue.
///
/// This function called from module_exit and on failure in module_init.
static void cleanup(void)
{
	int rc = 0;
	
	// -------------------------------------------------
	// Unregister keyboard notifier
	// -------------------------------------------------
	// FIXME: Should somehow check that keymon_kb_nf is registered like
	// if(keymon_genl_family.id)
	rc = unregister_keyboard_notifier( &keymon_kb_nf );
	if( rc != 0 )
	{
		km_log("Failed to unregister keyboard notifier. Error %d\n", rc);
	}

	// -------------------
	// Halt workqueue
	// -------------------
	if( keymon_wq )
	{
		flush_workqueue( keymon_wq );
		destroy_workqueue( keymon_wq );
	}

	// -------------------------------------------------
	// Unregister family (ops unregisters automatically)
	// -------------------------------------------------
	if( keymon_genl_family.id )
	{
		rc = genl_unregister_family( &keymon_genl_family );
		if( rc != 0 )
		{
			km_log("Failed to unregister generic netlink family. Error %d\n", rc);
		}
	}

	return;
}

//=============================================================================
//   keymon_init
//=============================================================================
/// Module entry point.
/// Registers generic netlink family, registers keyboard notifier.
/// @return 0 on success, error code otherwise
static int keymon_init(void)
{
	int rc = 0;

	// ----------------------------------------------
	// Register generic netlink family
	// ----------------------------------------------
	rc = genl_register_family_with_ops( &keymon_genl_family, 
                          keymon_genl_ops, ARRAY_SIZE(keymon_genl_ops) );
	if( rc )
	{
		km_log( "Failed to register generic netlink family. Error %d\n", rc );
		goto fail;
	}
	km_log( "Generic netlink family id = %d\n", keymon_genl_family.id );

	// ----------------------------------------------------
	// Register multicast group for generic netlink family
	// ----------------------------------------------------
	rc = genl_register_mc_group( &keymon_genl_family, &keymon_mc_group );
	if( rc )
	{
		km_log( "Failed to register multicast group. Error %d\n", rc );
		goto fail;
	}

	// -----------------------
	// Initialize workqueue
	// -----------------------
	keymon_wq = create_workqueue( KEYMON_WQ_NAME );
	if( !keymon_wq )
	{
		km_log( "Failed to create workqueue.\n" );
		goto fail;
	}
	
	// ---------------------------------------------
	// Register keyboard notifier
	// ---------------------------------------------
	rc = register_keyboard_notifier( &keymon_kb_nf );
	if( rc )
	{
		km_log( "Failed to register keyboard notifier. Error %d\n", rc );
		goto fail;
	}

	return 0;

fail:
	// FIXME: Isn't it will be called by module_exit?
	cleanup();
	return -EINVAL;
}

//=============================================================================
//   keymon_exit
//=============================================================================
/// Module exit point.
static void keymon_exit(void)
{
	cleanup();
	return;
}

module_init(keymon_init);
module_exit(keymon_exit);

MODULE_AUTHOR("Alex Dzyoba <avd@reduct.ru>");
MODULE_DESCRIPTION("Kernel part of keymon - keyboard statistic system");
MODULE_LICENSE("GPL");
