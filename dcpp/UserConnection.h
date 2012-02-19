/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_USER_CONNECTION_H
#define DCPLUSPLUS_DCPP_USER_CONNECTION_H

#include "forward.h"
#include "TimerManager.h"
#include "UserConnectionListener.h"
#include "BufferedSocketListener.h"
#include "BufferedSocket.h"
#include "CriticalSection.h"
#include "HintedUser.h"
#include "AdcCommand.h"
#include "MerkleTree.h"
#ifdef _USELUA
    #include "ScriptManager.h"
#endif
#include "PluginManager.h"

namespace dcpp {
#ifdef _USELUA
class UserConnectionScriptInstance : public ScriptInstance {
protected:
	bool onUserConnectionMessageIn(UserConnection* aConn, const string& aLine);
	bool onUserConnectionMessageOut(UserConnection* aConn, const string& aLine);
};
#endif
class UserConnection : public PluginEntity<ConnectionData> , public Speaker<UserConnectionListener>,
	private BufferedSocketListener, public Flags, private CommandHandler<UserConnection>,
	private boost::noncopyable
#ifdef _USELUA
	,public UserConnectionScriptInstance
#endif
{
public:
	friend class ConnectionManager;

	static const string FEATURE_MINISLOTS;
	static const string FEATURE_XML_BZLIST;
	static const string FEATURE_ADCGET;
	static const string FEATURE_ZLIB_GET;
	static const string FEATURE_TTHL;
	static const string FEATURE_TTHF;
	static const string FEATURE_ADC_BAS0;
	static const string FEATURE_ADC_BASE;
	static const string FEATURE_ADC_BZIP;
	static const string FEATURE_ADC_TIGR;

	static const string FILE_NOT_AVAILABLE;

	enum Modes {
		MODE_COMMAND = BufferedSocket::MODE_LINE,
		MODE_DATA = BufferedSocket::MODE_DATA
	};

	enum Flags {
		FLAG_NMDC = 0x01,
		FLAG_OP = FLAG_NMDC << 1,
		FLAG_UPLOAD = FLAG_OP << 1,
		FLAG_DOWNLOAD = FLAG_UPLOAD << 1,
		FLAG_INCOMING = FLAG_DOWNLOAD << 1,
		FLAG_ASSOCIATED = FLAG_INCOMING << 1,
		FLAG_HASSLOT = FLAG_ASSOCIATED << 1,
		FLAG_HASEXTRASLOT = FLAG_HASSLOT << 1,
		FLAG_INVALIDKEY = FLAG_HASEXTRASLOT << 1,
		FLAG_SUPPORTS_MINISLOTS = FLAG_INVALIDKEY << 1,
		FLAG_SUPPORTS_XML_BZLIST = FLAG_SUPPORTS_MINISLOTS << 1,
		FLAG_SUPPORTS_ADCGET = FLAG_SUPPORTS_XML_BZLIST << 1,
		FLAG_SUPPORTS_ZLIB_GET = FLAG_SUPPORTS_ADCGET << 1,
		FLAG_SUPPORTS_TTHL = FLAG_SUPPORTS_ZLIB_GET << 1,
		FLAG_SUPPORTS_TTHF = FLAG_SUPPORTS_TTHL << 1
	};

	enum States {
		// ConnectionManager
		STATE_UNCONNECTED,
		STATE_CONNECT,

		// Handshake
		STATE_SUPNICK,		// ADC: SUP, Nmdc: $Nick
		STATE_INF,
		STATE_LOCK,
		STATE_DIRECTION,
		STATE_KEY,

		// UploadManager
		STATE_GET,			// Waiting for GET
		STATE_SEND,			// Waiting for $Send

		// DownloadManager
		STATE_SND,	// Waiting for SND
		STATE_IDLE, // No more downloads for the moment

		// Up & down
		STATE_RUNNING,		// Transmitting data

	};

	short getNumber() { return (short)((((size_t)this)>>2) & 0x7fff); }

	// NMDC stuff
	void myNick(const string& aNick) { send("$MyNick " + Text::fromUtf8(aNick, encoding) + '|'); }
	void lock(const string& aLock, const string& aPk) { send ("$Lock " + aLock + " Pk=" + aPk + '|'); }
	void key(const string& aKey) { send("$Key " + aKey + '|'); }
	void direction(const string& aDirection, int aNumber) { send("$Direction " + aDirection + " " + Util::toString(aNumber) + '|'); }
	void fileLength(const string& aLength) { send("$FileLength " + aLength + '|'); }
	void error(const string& aError) { send("$Error " + aError + '|'); }
	void listLen(const string& aLength) { send("$ListLen " + aLength + '|'); }
	void getListLen() { send("$GetListLen|"); }
	void maxedOut(size_t queue_position = 0) {
		 bool sendPos = queue_position > 0;
		 if(isSet(FLAG_NMDC)) {
			send("$MaxedOut" + (sendPos ? (" " + Util::toString(queue_position)) : Util::emptyString) + "|"); 
		 } else {
		  	AdcCommand cmd(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_SLOTS_FULL, "Slots full");
			if(sendPos) {
				cmd.addParam("QP", Util::toString(queue_position));
			}	
			send(cmd);
		 }
	}

	void fileNotAvail(const std::string& msg = FILE_NOT_AVAILABLE) { isSet(FLAG_NMDC) ? send("$Error " + msg + "|") : send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_FILE_NOT_AVAILABLE, msg)); }
	void supports(const StringList& feat);

	// ADC Stuff
	void sup(const StringList& features);
	void inf(bool withToken);
	void get(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) { send(AdcCommand(AdcCommand::CMD_GET).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }
	void snd(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) { send(AdcCommand(AdcCommand::CMD_SND).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }
	void send(const AdcCommand& c) { send(c.toString(0, isSet(FLAG_NMDC))); }

	void setDataMode(int64_t aBytes = -1) { dcassert(socket); socket->setDataMode(aBytes); }
	void setLineMode(size_t rollback) { dcassert(socket); socket->setLineMode(rollback); }

	void connect(const string& aServer, const string& aPort, const string& localPort, const BufferedSocket::NatRoles natRole);
	void accept(const Socket& aServer);

	void updated() { if(socket) socket->updated(); }

	void disconnect(bool graceless = false) { if(socket) socket->disconnect(graceless); }
	void transmitFile(InputStream* f) { socket->transmitFile(f); }

	const string& getDirectionString() {
		dcassert(isSet(FLAG_UPLOAD) ^ isSet(FLAG_DOWNLOAD));
		return isSet(FLAG_UPLOAD) ? UPLOAD : DOWNLOAD;
	}

	const UserPtr& getUser() const { return user; }
	UserPtr& getUser() { return user; }
	const HintedUser getHintedUser() const { return HintedUser(user, hubUrl); }

	bool isSecure() const { return socket && socket->isSecure(); }
	bool isTrusted() const { return socket && socket->isTrusted(); }
	std::string getCipherName() const { return socket ? socket->getCipherName() : Util::emptyString; }
	vector<uint8_t> getKeyprint() const { return socket ? socket->getKeyprint() : vector<uint8_t>(); }
	const string getPort() const { if(socket) return socket->getPort(); else return 0; }
	string getRemoteIp() const { return socket->getIp(); }
	Download* getDownload() { dcassert(isSet(FLAG_DOWNLOAD)); return download; }
	void setDownload(Download* d) { dcassert(isSet(FLAG_DOWNLOAD)); download = d; }
	Upload* getUpload() { dcassert(isSet(FLAG_UPLOAD)); return upload; }
	void setUpload(Upload* u) { dcassert(isSet(FLAG_UPLOAD)); upload = u; }

	void handle(AdcCommand::SUP t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::INF t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GET t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::SND t, const AdcCommand& c) { fire(t, this, c);	}
	void handle(AdcCommand::STA t, const AdcCommand& c);
	void handle(AdcCommand::RES t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GFI t, const AdcCommand& c) { fire(t, this, c);	}

	// Ignore any other ADC commands for now
	template<typename T> void handle(T , const AdcCommand& ) { }

	int64_t getChunkSize() const { return chunkSize; }
	void updateChunkSize(int64_t leafSize, int64_t lastChunk, uint64_t ticks);
	//BMDC++
	void sendRaw(const string& raw) { send(raw);}
	ConnectionData* getPluginObject() noexcept;

	GETSET(string, hubUrl, HubUrl);
	GETSET(string, token, Token);
	GETSET(string, encoding, Encoding);
	GETSET(States, state, State);
	GETSET(uint64_t, lastActivity, LastActivity);
	GETSET(double, speed, Speed);
private:
	int64_t chunkSize;
	BufferedSocket* socket;
	bool secure;
	UserPtr user;

	static const string UPLOAD, DOWNLOAD;

	union {
		Download* download;
		Upload* upload;
	};

	// We only want ConnectionManager to create this...
	UserConnection(bool secure_) noexcept : encoding(Text::systemCharset), state(STATE_UNCONNECTED),
		lastActivity(0), speed(0), chunkSize(0), socket(0), secure(secure_), download(NULL) {
	}

	virtual ~UserConnection() {
		BufferedSocket::putSocket(socket);
	}

	friend struct DeleteFunction;

	void setUser(const UserPtr& aUser) {
		user = aUser;
	}

	void onLine(const string& aLine) noexcept;

	void send(const string& aString);

	virtual void on(Connected) noexcept;
	virtual void on(Line, const string&) noexcept;
	virtual void on(Data, uint8_t* data, size_t len) noexcept;
	virtual void on(BytesSent, size_t bytes, size_t actual) noexcept ;
	virtual void on(ModeChange) noexcept;
	virtual void on(TransmitDone) noexcept;
	virtual void on(Failed, const string&) noexcept;
	virtual void on(Updated) noexcept;
};

} // namespace dcpp

#endif // !defined(USER_CONNECTION_H)
