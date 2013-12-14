/*
 * Copyright © 2004-2013 Jens Oknelid, paskharen@gmail.com
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

#include "wulformanager.hh"
#include "WulforUtil.hh"

#include <iostream>
#include <glib/gi18n.h>
#include "hashdialog.hh"
#include "settingsdialog.hh"

using namespace std;
using namespace dcpp;

WulforManager *WulforManager::manager = NULL;
string WulforManager::argv1;

void WulforManager::start(int argc, char **argv)
{
	if (argc > 1)
	{
		argv1 = argv[1];
	}

	// Create WulforManager
	dcassert(!manager);
	manager = new WulforManager();
	manager->createMainWindow();
}

void WulforManager::stop()
{
	dcassert(manager);
	delete manager;
	manager = NULL;
}

WulforManager *WulforManager::get()
{
	dcassert(manager);
	return manager;
}

WulforManager::WulforManager():
mainWin(NULL),clientThread(NULL)
{
	abort = FALSE;

	// Initialize sempahore variables
	clientCondValue = 0;
	g_cond_init(&clientCond);
	g_mutex_init(&clientCondMutex);

	g_mutex_init(&clientCallMutex);
	g_mutex_init(&clientQueueMutex);
	g_rw_lock_init(&entryMutex);

	GError *error = NULL;

	clientThread = g_thread_try_new("client",threadFunc_client, (gpointer)this, &error);
	if (error != NULL)
	{
		cerr << "Unable to create client thread: " << error->message << endl;
		g_error_free(error);
		exit(EXIT_FAILURE);
	}
	// Determine path to data files
	path = string(_DATADIR) + G_DIR_SEPARATOR_S + g_get_prgname();
	if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
	{
		cerr << path << " is inaccessible, falling back to current directory instead.\n";
		path = ".";
	}

	// Set the custom icon search path so GTK+ can find our icons
	const string iconPath = path + G_DIR_SEPARATOR_S + "icons";
	const string themes = path + G_DIR_SEPARATOR_S + "themes";
	GtkIconTheme *iconTheme = gtk_icon_theme_get_default();
	gtk_icon_theme_append_search_path(iconTheme, iconPath.c_str());
	gtk_icon_theme_append_search_path(iconTheme, themes.c_str());
}

WulforManager::~WulforManager()
{
	abort = TRUE;

	g_mutex_lock(&clientCondMutex);
	clientCondValue++;
	g_cond_signal(&clientCond);
	g_mutex_unlock(&clientCondMutex);

	g_thread_join(clientThread);

	g_cond_clear(&clientCond);
	g_mutex_clear(&clientCondMutex);
	g_mutex_clear(&clientCallMutex);
	g_mutex_clear(&clientQueueMutex);
	g_rw_lock_clear(&entryMutex);
}

void WulforManager::createMainWindow()
{
	dcassert(!mainWin);
	mainWin = new MainWindow();
	WulforManager::insertEntry_gui(mainWin);
	mainWin->show();
}

void WulforManager::deleteMainWindow()
{
	// response dialogs: hash, settings
	DialogEntry *hashDialogEntry = getHashDialog_gui();
	DialogEntry *settingsDialogEntry = getSettingsDialog_gui();

	if (hashDialogEntry != NULL)
	{
		gtk_dialog_response(GTK_DIALOG(hashDialogEntry->getContainer()), GTK_RESPONSE_OK);
	}
	if (settingsDialogEntry != NULL)
	{
		dynamic_cast<Settings*>(settingsDialogEntry)->response_gui();
	}

	mainWin->remove();
	gtk_main_quit();
}

gpointer WulforManager::threadFunc_client(gpointer data)
{
	WulforManager *man = (WulforManager *)data;
	man->processClientQueue();
	return NULL;
}
//TODO: remove ?
void WulforManager::processClientQueue()
{
	FuncBase *func;

	while (!abort)
	{
		g_mutex_lock(&clientCondMutex);
		while (clientCondValue < 1)
			g_cond_wait(&clientCond, &clientCondMutex);
		clientCondValue--;
		g_mutex_unlock(&clientCondMutex);

		g_mutex_lock(&clientCallMutex);
		g_mutex_lock(&clientQueueMutex);
		while (!clientFuncs.empty())
		{
			func = clientFuncs.front();
			clientFuncs.pop_front();
			g_mutex_unlock(&clientQueueMutex);

			func->call(NULL);
			delete func;

			g_mutex_lock(&clientQueueMutex);
		}
		g_mutex_unlock(&clientQueueMutex);
		g_mutex_unlock(&clientCallMutex);
	}

	g_thread_exit(NULL);
}

void WulforManager::dispatchGuiFunc(FuncBase *func)
{
		g_idle_add((GSourceFunc)((func)->call_),func);
}

void WulforManager::dispatchClientFunc(FuncBase *func)
{
	g_mutex_lock(&clientQueueMutex);
	clientFuncs.push_back(func);
	g_mutex_unlock(&clientQueueMutex);

	g_mutex_lock(&clientCondMutex);
	clientCondValue++;
	g_cond_signal(&clientCond);
	g_mutex_unlock(&clientCondMutex);
}

MainWindow *WulforManager::getMainWindow()
{
	dcassert(mainWin);
	return mainWin;
}

string WulforManager::getURL()
{
	return argv1;
}

string WulforManager::getPath() const
{
	return path;
}

void WulforManager::insertEntry_gui(Entry *entry)
{
	g_rw_lock_writer_lock(&entryMutex);
	entries[entry->getID()] = entry;
	g_rw_lock_writer_unlock(&entryMutex);
}

// Should be called from a callback, so gdk_threads_enter/leave is called automatically.
void WulforManager::deleteEntry_gui(Entry *entry)
{
	const string &id = entry->getID();
	deque<FuncBase *>::iterator fIt;

	g_mutex_lock(&clientCallMutex);

	// Erase any pending calls to this bookentry.
	g_mutex_lock(&clientQueueMutex);
	fIt = clientFuncs.begin();
	while (fIt != clientFuncs.end())
	{
		if ((*fIt)->getID() == id)
		{
			delete *fIt;
			fIt = clientFuncs.erase(fIt);
		}
		else
			++fIt;
	}
	g_mutex_unlock(&clientQueueMutex);

	// Remove the bookentry from the list.
	g_rw_lock_writer_lock(&entryMutex);
	if (entries.find(id) != entries.end())
		entries.erase(id);
	g_rw_lock_writer_unlock(&entryMutex);

	g_mutex_unlock(&clientCallMutex);

	delete entry;
	entry = NULL;
}

bool WulforManager::isEntry_gui(Entry *entry)
{
	g_rw_lock_writer_lock(&entryMutex);

	unordered_map<string, Entry *>::const_iterator it = find_if(entries.begin(), entries.end(),
		CompareSecond<string, Entry *>(entry));

	if (it == entries.end())
		entry = NULL;

	g_rw_lock_writer_unlock(&entryMutex);

	return (entry != NULL);
}

DialogEntry* WulforManager::getDialogEntry_gui(const string &id)
{
	DialogEntry *ret = NULL;

	g_rw_lock_reader_lock(&entryMutex);
	if (entries.find(id) != entries.end())
		ret = dynamic_cast<DialogEntry *>(entries[id]);
	g_rw_lock_reader_unlock(&entryMutex);

	return ret;
}

void WulforManager::onReceived_gui(const string& link)
{
	dcassert(mainWin);

	if (WulforUtil::isHubURL(link) && SETTING(URL_HANDLER))
		mainWin->showHub_gui(link);

	else if (WulforUtil::isMagnet(link) && SETTING(MAGNET_REGISTER))
		mainWin->actionMagnet_gui(link);
}

gint WulforManager::openHashDialog_gui()
{
	Hash *h = new Hash();
	gint response = h->run();

	return response;
}

gint WulforManager::openSettingsDialog_gui()
{
	Settings *s = new Settings();
	gint response = s->run();

	return response;
}

DialogEntry *WulforManager::getHashDialog_gui()
{
	return getDialogEntry_gui(Util::toString(Entry::HASH_DIALOG) + ":");
}

DialogEntry *WulforManager::getSettingsDialog_gui()
{
	return getDialogEntry_gui(Util::toString(Entry::SETTINGS_DIALOG) + ":");
}
