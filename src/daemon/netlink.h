#ifndef NETLINK_H
#define NETLINK_H

#include <error.h>

#include <netlink/netlink.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>

#include "../include/genl_def.h"

struct nl_sock *sk = NULL;

int keymon_mc_group_id = -1;

int nl_sock_init();
void nl_sock_cleanup();

extern int receiver(struct nl_msg *msg, void *arg);

#endif // NETLINK_H
