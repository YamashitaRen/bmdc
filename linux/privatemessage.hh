/*
 * Copyright © 2004-2017 Jens Oknelid, paskharen@gmail.com
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * In addition, as a special exception, compiling, linking, and/or
 * using OpenSSL with this program is allowed.
 */

#ifndef _BMDC_PRIVATE_MESSAGE_HH
#define _BMDC_PRIVATE_MESSAGE_HH

#include "../dcpp/stdinc.h"
#include "../dcpp/DCPlusPlus.h"
#include "../dcpp/ClientManagerListener.h"
#include "bookentry.hh"
#include "message.hh"
#include "UserCommandMenu.hh"
#include <vector>
#include "../dcpp/Flags.h"


class WulforSettingsManager;
class EmoticonsDialog;

class PrivateMessage:
	public BookEntry,
	public dcpp::ClientManagerListener,
	public dcpp::Flags
{
	public:
		PrivateMessage(const std::string &cid, const std::string &hubUrl);
		virtual ~PrivateMessage();
		virtual void show();
		GtkWidget *createmenu() override;

		// GUI functions
		void addMessage_gui(std::string message, Msg::TypeMsg typemsg);
		void addStatusMessage_gui(std::string message, Msg::TypeMsg typemsg);
		void preferences_gui();
		void sendMessage_p(std::string message) { sendMessage_client(message); }
		bool getIsOffline() const { return isSet(OFFLINE); }

	private:
		using dcpp::ClientManagerListener::on;
		//@ Status of PM's user
		typedef enum
		{
			NORMAL = 0,
			OFFLINE = 1,
			BOT = 2,
		} UserStatus;
	
		// GUI functions
		void setStatus_gui(std::string text);
		void addLine_gui(Msg::TypeMsg typemsg, const std::string &line);
		void applyTags_gui(const std::string &line);
		void applyEmoticons_gui();
		void getSettingTag_gui(WulforSettingsManager *wsm, Tag::TypeTag type, std::string &fore, std::string &back, bool &bold, bool &italic);
		GtkTextTag* createTag_gui(const std::string &tagname, Tag::TypeTag type);
		void updateCursor(GtkWidget *widget);
		void updateOnlineStatus_gui(bool online);
		void readLog(const std::string& logPath, const unsigned setting);

		// GUI callbacks
		static gboolean onFocusIn_gui(GtkWidget *widget, GdkEventFocus *event, gpointer data);
		static gboolean onKeyPress_gui(GtkWidget *widget, GdkEventKey *event, gpointer data);
		static gboolean onLinkTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onHubTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onMagnetTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event, GtkTextIter *iter, gpointer data);
		static gboolean onIpTagEvent_gui(GtkTextTag *tag, GObject *textView, GdkEvent *event , GtkTextIter *iter, gpointer data);//BMDC
		static gboolean onChatPointerMoved_gui(GtkWidget *widget, GdkEventMotion *event, gpointer data);
		static gboolean onChatVisibilityChanged_gui(GtkWidget* widget, GdkEventVisibility* event, gpointer data);
		static gboolean onEmotButtonRelease_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);
		static void onChatScroll_gui(GtkAdjustment *adjustment, gpointer data);
		static void onChatResize_gui(GtkAdjustment *adjustment, gpointer data);
		static void onSendMessage_gui(GtkEntry *entry, gpointer data);
		static void onCopyURIClicked_gui(GtkMenuItem *item, gpointer data);
		static void onOpenLinkClicked_gui(GtkMenuItem *item, gpointer data);
		static void onOpenHubClicked_gui(GtkMenuItem *item, gpointer data);
		static void onSearchMagnetClicked_gui(GtkMenuItem *item, gpointer data);
		static void onMagnetPropertiesClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadToClicked_gui(GtkMenuItem *item, gpointer data);
		static void onDownloadClicked_gui(GtkMenuItem *item, gpointer data);
		static void onCommandClicked_gui(GtkWidget *widget, gpointer data);
		static void onUseEmoticons_gui(GtkWidget *widget, gpointer data);
		static gboolean onChatCommandButtonRelease_gui(GtkWidget *widget, GdkEventButton *event, gpointer data);

		static void onCopyIpItem_gui(GtkWidget *wid, gpointer data);
		static void onRipeDbItem_gui(GtkWidget *wid, gpointer data);
		static void onCloseItem(gpointer data);
		static void onCopyCID(gpointer data);
		static void onAddFavItem(gpointer data);
		static void onCopyNicks(gpointer data);
		void setImageButton(const std::string country);

		// Client functions
		void sendMessage_client(std::string message);
		void addFavoriteUser_client();
		void removeFavoriteUser_client();
		void getFileList_client();
		void grantSlot_client();

		// client callback
		virtual void on(dcpp::ClientManagerListener::UserConnected, const dcpp::UserPtr& aUser) noexcept;
		virtual void on(dcpp::ClientManagerListener::UserDisconnected, const dcpp::UserPtr& aUser) noexcept;

		GtkTextBuffer *messageBuffer;
		GtkTextMark *mark, *start_mark, *end_mark, *tag_mark, *emot_mark;
		std::string cid;
		std::string hubUrl;
		std::vector<std::string> history;
		int historyIndex;
		bool sentAwayMessage;
		static const int maxLines = 500; ///@todo: make these preferences
		GdkCursor* handCursor;
		std::string selectedTagStr;
		GtkTextTag* selectedTag;
		bool scrollToBottom;
		GtkTextTag *TagsMap[Tag::TAG_LAST];
		Tag::TypeTag tagMsg, tagNick;
		bool useEmoticons;
		gint totalEmoticons;
		EmoticonsDialog *emotdialog;
		UserCommandMenu *userCommandMenu;
		bool notCreated;
		GtkWidget* m_menu;

};

#else
class PrivateMessage;
#endif
