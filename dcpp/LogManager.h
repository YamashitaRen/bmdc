/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_LOG_MANAGER_H
#define DCPLUSPLUS_DCPP_LOG_MANAGER_H

#include <deque>
#include <utility>

#include "typedefs.h"

#include "CriticalSection.h"
#include "Singleton.h"
#include "Speaker.h"
#include "LogManagerListener.h"

namespace dcpp {

using std::pair;
using std::deque;

class LogManager : public Singleton<LogManager>, public Speaker<LogManagerListener>
{
public:
	enum Area { CHAT, PM, DOWNLOAD, FINISHED_DOWNLOAD, UPLOAD, SYSTEM, STATUS, RAW , CHECK_USER, PROTO ,LAST };
	enum { FILE, FORMAT };
	enum Sev { LOW, NORMAL, HIGH };

	struct MessageData
	{
		MessageData(Sev _sev = Sev::LOW,time_t _t = time(NULL)):
		sev(_sev),tim(_t) {}
		Sev sev;
		time_t tim;
	};
	typedef std::deque<pair<std::string,MessageData> > List;

	void log(Area area, const string &message, int sev = Sev::NORMAL)
	{
		ParamMap params;
		params["message"] = message;
		log(area,params,sev);
	}

	void log(Area area, ParamMap& params, int sev = Sev::NORMAL) noexcept;
	void message(const string& msg,int sev = Sev::NORMAL);

	List getLastLogs();
	string getPath(Area area, ParamMap& params) const;
	string getPath(Area area) const;

	const string& getSetting(int area, int sel) const;
	void saveSetting(int area, int sel, const string& setting);
	void clearLogs(){ Lock l(cs); lastLogs.clear(); }
private:
	void log(const string& area, const string& msg,int sev = Sev::NORMAL) noexcept;

	friend class Singleton<LogManager>;
	CriticalSection cs;
	List lastLogs;

	int options[LAST][2];

	LogManager();
	virtual ~LogManager();
};

#define LOG(area, msg) LogManager::getInstance()->log(LogManager::area, msg)

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_LOG_MANAGER_H)
