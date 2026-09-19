#ifndef NET_ENET_H
#define NET_ENET_H

#if defined(_WIN32)
#define GROUP NET_WINSOCK_GROUP
#endif

#include "3rdparty/enet.h"

#if defined(_WIN32)
#undef GROUP
#endif

#endif
