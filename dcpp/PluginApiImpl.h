/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * The PluginApiImpl class contains implementations of certain callback functions,
 * they are simply separated here.
 */

#ifndef DCPLUSPLUS_DCPP_PLUGIN_API_IMPL_H
#define DCPLUSPLUS_DCPP_PLUGIN_API_IMPL_H

#include <cstdint>

#include "forward.h"
#include "typedefs.h"
#include "PluginDefs.h"

namespace dcpp {

class PluginApiImpl
{
public:
	static void initAPI(DCCore& dcCore);
	static void releaseAPI();

	static HubDataPtr DCAPI copyData(const HubDataPtr hub);
	static void DCAPI releaseData(HubDataPtr hub);

	static UserDataPtr DCAPI copyData(const UserDataPtr user);
	static void DCAPI releaseData(UserDataPtr user);

	static QueueDataPtr DCAPI copyData(const QueueDataPtr qi);
	static void DCAPI releaseData(QueueDataPtr qi);

	static ConfigValuePtr DCAPI copyData(const ConfigValuePtr val);
	static void DCAPI releaseData(ConfigValuePtr val);

private:
	// Functions for DCCore
	static intfHandle DCAPI registerInterface(const char* guid, dcptr_t funcs);
	static DCInterfacePtr DCAPI queryInterface(const char* guid, uint32_t version);
	static Bool DCAPI releaseInterface(intfHandle hInterface);

	// Functions for DCHooks
	static hookHandle DCAPI createHook(const char* guid, DCHOOK defProc);
	static Bool DCAPI destroyHook(hookHandle hHook);

	static subsHandle DCAPI bindHook(const char* guid, DCHOOK hookProc, void* pCommon);
	static Bool DCAPI runHook(hookHandle hHook, dcptr_t pObject, dcptr_t pData);
	static size_t DCAPI releaseHook(subsHandle hHook);

	// Functions For DCConfig
	static const char* DCAPI getPath(PathType type);
	static void DCAPI setConfig(const char* guid, const char* setting, ConfigValuePtr val);
	static ConfigValuePtr DCAPI getConfig(const char* guid, const char* setting, ConfigType type);

	// Functions for DCLog
	static void DCAPI log(const char* msg);

	// Functions for DCConnection
	static void DCAPI sendProtocolCmd(ConnectionDataPtr conn, const char* cmd);
	static void DCAPI terminateConnection(ConnectionDataPtr conn, Bool graceless);
	static void DCAPI sendUdpData(const char* ip, uint32_t port, dcptr_t data, size_t n);

	// Functions for DCUtils
	static size_t DCAPI toUtf8(char* dst, const char* src, size_t n);
	static size_t DCAPI fromUtf8(char* dst, const char* src, size_t n);

	static size_t DCAPI Utf8toWide(wchar_t* dst, const char* src, size_t n);
	static size_t DCAPI WidetoUtf8(char* dst, const wchar_t* src, size_t n);

	static size_t DCAPI toBase32(char* dst, const uint8_t* src, size_t n);
	static size_t DCAPI fromBase32(uint8_t* dst, const char* src, size_t n);
	
	// Functions for DCUI - the host has to define these
	static void DCAPI playSound(const char* path);
	static void DCAPI notify(const char* title, const char* message);


	// Functions for DCQueue
	static QueueDataPtr DCAPI addList(UserDataPtr user, Bool silent);
	static QueueDataPtr DCAPI addDownload(const char* hash, uint64_t size, const char* target);
	static QueueDataPtr DCAPI findDownload(const char* target);
	static void DCAPI removeDownload(QueueDataPtr qi);

	static void DCAPI setPriority(QueueDataPtr qi, QueuePrio priority);

	// Functions for DCHub
	static HubDataPtr DCAPI addHub(const char* url, const char* nick, const char* password);
	static HubDataPtr DCAPI findHub(const char* url);
	static void DCAPI removeHub(HubDataPtr hub);

	static void DCAPI emulateProtocolCmd(HubDataPtr hub, const char* cmd);
	static void DCAPI sendProtocolCmd(HubDataPtr hub, const char* cmd);

	static void DCAPI sendHubMessage(HubDataPtr hub, const char* message, Bool thirdPerson);
	static void DCAPI sendLocalMessage(HubDataPtr hub, const char* msg, MsgType type);
	static Bool DCAPI sendPrivateMessage(UserDataPtr user, const char* message, Bool thirdPerson);

	static UserDataPtr DCAPI findUser(const char* cid, const char* hubUrl);

	static DCHooks dcHooks;
	static DCConfig dcConfig;
	static DCLog dcLog;

	static DCConnection dcConnection;
	static DCHub dcHub;
	static DCQueue dcQueue;
	static DCUtils dcUtils;
	
	static DCUI dcUI;

	static Socket apiSocket;
};

} // namepsace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_PLUGIN_API_H)

/**
 * @file
 * $Id: PluginApiImpl.h 1248 2012-01-22 01:49:30Z crise $
 */
