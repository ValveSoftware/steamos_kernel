#ifndef __BACKPORT_NET_MAC80211_H
#define __BACKPORT_NET_MAC80211_H

/* on some kernels, libipw also uses this, so override */
#define ieee80211_rx mac80211_ieee80211_rx
#include_next <net/mac80211.h>

#endif /* __BACKPORT_NET_MAC80211_H */
