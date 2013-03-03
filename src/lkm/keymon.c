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

#include "keymon.h"

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
	}

	return NOTIFY_DONE;
}

//=============================================================================
//   keymon_genl_register_cmd
//=============================================================================
/// Handler for REGISTER commmand arrived on netlink socket.
/// @param skb  - socket buffer
/// @param info - generic netlink info
/// @return 0 on success, error code otherwise
static int keymon_genl_register_cmd( struct sk_buff *skb, struct genl_info *info )
{
	return 0;
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
/// Unregisters generic netlink family, unregisters keyboard notifier.
/// This function called from module_exit and on failure in module_init.
static void cleanup(void)
{
	int ret = 0;
	
	// -------------------------------------------------
	// Unregister family (ops unregisters automatically)
	// -------------------------------------------------
	if( keymon_genl_family.id )
	{
		ret = genl_unregister_family( &keymon_genl_family );
		if( ret != 0 )
		{
			km_log("Failed to unregister generic netlink family. Error %d\n", ret);
		}
	}

	// -------------------------------------------------
	// Unregister keyboard notifier
	// -------------------------------------------------
	// FIXME: Should somehow check that keymon_kb_nf is registered like
	// if(keymon_genl_family.id)
	ret = unregister_keyboard_notifier( &keymon_kb_nf );
	if( ret != 0 )
	{
		km_log("Failed to unregister keyboard notifier. Error %d\n", ret);
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
	int ret = 0;

	// ----------------------------------------------
	// Register generic netlink family
	// ----------------------------------------------
	ret = genl_register_family_with_ops( &keymon_genl_family, 
                          keymon_genl_ops, ARRAY_SIZE(keymon_genl_ops) );
	if( ret )
	{
		km_log( "Failed to register generic netlink family. Error %d\n", ret );
		goto fail;
	}
	km_log( "Generic netlink family id = %d\n", keymon_genl_family.id );

	// ---------------------------------------------
	// Register keyboard notifier
	// ---------------------------------------------
	ret = register_keyboard_notifier( &keymon_kb_nf );
	if( ret )
	{
		km_log( "Failed to register keyboard notifier. Error %d\n", ret );
		goto fail;
	}

	return 0;

fail:
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
