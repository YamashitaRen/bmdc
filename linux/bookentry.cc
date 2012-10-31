/*
 * Copyright © 2004-2012 Jens Oknelid, paskharen@gmail.com
 * Copyright © 2010-2012 Mank freedcpp@seznam.cz
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

#include "bookentry.hh"
#include "wulformanager.hh"
#include "settingsmanager.hh"
#include "WulforUtil.hh"

using namespace std;

BookEntry::BookEntry(const EntryType type, const string &text, const string &glade, const string &id):
	Entry(type, glade, id),
	eventBox(NULL),
	labelBox(NULL),
	tabMenuItem(NULL),
	closeButton(NULL),
	label(NULL),
	fItem(NULL),
	bold(false),
	urgent(false),
	icon(NULL),
	popTabMenuItem(NULL),
	type(type),
	IsCloseButton(false)

{
	GSList *group = NULL;
	labelBox = gtk_hbox_new(FALSE, 5);

	eventBox = gtk_event_box_new();
	gtk_event_box_set_above_child(GTK_EVENT_BOX(eventBox), TRUE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventBox), FALSE);

	// icon
	icon = gtk_image_new();
	gtk_box_pack_start(GTK_BOX(labelBox), icon, FALSE, FALSE, 0);

	// Make the eventbox fill to all left-over space.
	gtk_box_pack_start(GTK_BOX(labelBox), GTK_WIDGET(eventBox), TRUE, TRUE, 0);

	label = GTK_LABEL(gtk_label_new(text.c_str()));
	gtk_container_add(GTK_CONTAINER(eventBox), GTK_WIDGET(label));

	// Align text to the left (x = 0) and in the vertical center (0.5)
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    if(IsCloseButton || WGETB("use-close-button"))
     {
	   closeButton = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(closeButton), GTK_RELIEF_NONE);
        gtk_button_set_focus_on_click(GTK_BUTTON(closeButton), FALSE);

        // Shrink the padding around the close button
        GtkRcStyle *rcstyle = gtk_rc_style_new();
        rcstyle->xthickness = rcstyle->ythickness = 0;
        gtk_widget_modify_style(closeButton, rcstyle);
        gtk_rc_style_unref(rcstyle);

        // Add the stock icon to the close button
        GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(closeButton), image);
        gtk_box_pack_start(GTK_BOX(labelBox), closeButton, FALSE, FALSE, 0);

    #if GTK_CHECK_VERSION(2, 12, 0)
        gtk_widget_set_tooltip_text(closeButton, _("Close tab"));
    #else
        tips = gtk_tooltips_new();
        g_object_ref_sink(tips);
        gtk_tooltips_enable(tips);
        gtk_tooltips_set_tip(tips, closeButton, _("Close tab"), NULL);
    #endif
    }
	gtk_widget_show_all(labelBox);

	tabMenuItem = gtk_radio_menu_item_new_with_label(group, text.c_str());
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(tabMenuItem));

	setLabel_gui(text);
	setIcon_gui(type);

	// Associates entry to the widget for later retrieval in MainWindow::switchPage_gui()
	g_object_set_data(G_OBJECT(getContainer()), "entry", (gpointer)this);

	g_signal_connect(getLabelBox(), "button-press-event", G_CALLBACK(onButtonPressed_gui), (gpointer)this);
	g_signal_connect(getLabelBox(), "button-release-event", G_CALLBACK(onButtonReleased_gui), (gpointer)this);
}

GtkWidget* BookEntry::getContainer()
{
	return getWidget("mainBox");
}

void BookEntry::setIcon_gui(const EntryType type)
{
	string stock;
	switch (type)
	{
		case Entry::FAVORITE_HUBS : stock = WGETS("icon-favorite-hubs"); break;
		case Entry::FAVORITE_USERS : stock = WGETS("icon-favorite-users"); break;
		case Entry::IGNORE_USERS : stock = WGETS("icon-ignore"); break;
		case Entry::PUBLIC_HUBS : stock = WGETS("icon-public-hubs"); break;
		case Entry::DOWNLOAD_QUEUE : stock = WGETS("icon-queue"); break;
		case Entry::SEARCHS:
		case Entry::SEARCH : stock = WGETS("icon-search"); break;
		case Entry::SEARCH_ADL : stock = WGETS("icon-search-adl"); break;
		case Entry::SEARCH_SPY : stock = WGETS("icon-search-spy"); break;
		case Entry::FINISHED_DOWNLOADS : stock = WGETS("icon-finished-downloads"); break;
		case Entry::FINISHED_UPLOADS : stock = WGETS("icon-finished-uploads"); break;
		case Entry::PRIVATE_MESSAGE : stock = WGETS("icon-pm-online"); break;
		case Entry::HUB : stock = WGETS("icon-hub-offline"); break;
		case Entry::SHARE_BROWSER : stock = WGETS("icon-directory"); break;
		case Entry::NOTEPAD : stock = WGETS("icon-notepad"); break;
		case Entry::SYSTEML : stock = WGETS("icon-system"); break;
		case Entry::ABOUT_CONFIG : stock = WGETS("icon-system"); break;//for now
		default: ;
	}
	gtk_image_set_from_stock(GTK_IMAGE(icon), stock.c_str(), GTK_ICON_SIZE_MENU);//BUTTON
}

void BookEntry::setIcon_gui(const std::string stock)
{
	gtk_image_set_from_stock(GTK_IMAGE(icon), stock.c_str(), GTK_ICON_SIZE_MENU);//BUTTON
}

void BookEntry::setIconPixbufs_gui(const std::string iconspath)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(iconspath.c_str(),15,15,NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon),pixbuf);
}

void BookEntry::setLabel_gui(string text)
{
	// Update the tab menu item label
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(tabMenuItem));
	if (child && GTK_IS_LABEL(child))
		gtk_label_set_text(GTK_LABEL(child), text.c_str());
    
	if(IsCloseButton || WGETB("use-close-button"))
    {
        // Update the notebook tab label
        #if GTK_CHECK_VERSION(2, 12, 0)
            gtk_widget_set_tooltip_text(eventBox, text.c_str());
        #else
            gtk_tooltips_set_tip(tips, eventBox, text.c_str(), text.c_str());
        #endif
    }

    if(WGETB("custom-font-size"))
    {
		GtkStyle *style = gtk_widget_get_default_style();
		PangoFontDescription *desc = pango_font_description_copy(style->font_desc);
		gint font_sized = pango_font_description_get_size (desc);
		if(font_sized != (gint)WGETI("book-font-size"))
		{
			gint fsize = (gint)WGETI("book-font-size")*10*PANGO_SCALE;
			if(fsize >=1)
				pango_font_description_set_size (desc, fsize);
		}
		gtk_widget_modify_font (GTK_WIDGET(label),desc);
    }

	glong len = g_utf8_strlen(text.c_str(), -1);

	// Truncate the label text
	if (len > labelSize)
	{
		gchar truncatedText[text.size()];
		const string clipText = "...";
		len = labelSize - g_utf8_strlen(clipText.c_str(), -1);
		g_utf8_strncpy(truncatedText, text.c_str(), len);
		truncatedLabelText = truncatedText + clipText;
	}
	else
	{
		truncatedLabelText = text;
	}

	labelText = text;
	updateLabel_gui();

	// Update the main window title if the current tab is selected.
	if (isActive_gui())
		WulforManager::get()->getMainWindow()->setTitle(getLabelText());
}

void BookEntry::setBold_gui()
{
	if (!bold && !isActive_gui())
	{
		bold = TRUE;
		updateLabel_gui();
	}
}

void BookEntry::setUrgent_gui()
{
	if (!isActive_gui())
	{
		MainWindow *mw = WulforManager::get()->getMainWindow();

		if (!urgent)
		{
			bold = TRUE;
			urgent = TRUE;
			updateLabel_gui();
		}

		if (!mw->isActive_gui())
			mw->setUrgent_gui();
	}
}

void BookEntry::setActive_gui()
{
	if (bold || urgent)
	{
		bold = FALSE;
		urgent = FALSE;
		updateLabel_gui();
	}
}

bool BookEntry::isActive_gui()
{
	MainWindow *mw = WulforManager::get()->getMainWindow();

	return mw->isActive_gui() && mw->currentPage_gui() == getContainer();
}

void BookEntry::updateLabel_gui()
{
	const char *format = "%s";
	bool color = WGETB("colorize-tab-text");
	string colortext = "<span foreground=\"";
	colortext +=  urgent ? WGETS("color-tab-text-urgent") : WGETS("color-tab-text-bold");
	colortext += "\">%s</span>";

	if (urgent)
		format = color ? colortext.c_str() : "<i><b>%s</b></i>";
	else if (bold)
		format = color ? colortext.c_str() : "<b>%s</b>";

	char *markup = g_markup_printf_escaped(format, truncatedLabelText.c_str());
	gtk_label_set_markup(label, markup);
	g_free(markup);

}

const string& BookEntry::getLabelText() const
{
	return labelText;
}
//BMDC++
void BookEntry::onCloseItem(gpointer data)
{
	BookEntry *book = reinterpret_cast<BookEntry *>(data);
	book->removeBooK_GUI();
}

void BookEntry::removeBooK_GUI()
{
	WulforManager::get()->getMainWindow()->removeBookEntry_gui(this);
}

GtkWidget *BookEntry::createmenu()
{
    GtkWidget *closeTabMenuItem;
    popTabMenuItem = gtk_menu_new();
    closeTabMenuItem = gtk_menu_item_new_with_label(_("Close"));
    gtk_menu_shell_append(GTK_MENU_SHELL(popTabMenuItem),closeTabMenuItem);
    gtk_widget_show(closeTabMenuItem);
    g_signal_connect_swapped(closeTabMenuItem, "activate", G_CALLBACK(onCloseItem), (gpointer)this);
    return popTabMenuItem;
}

gboolean BookEntry::onButtonPressed_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	BookEntry *book = reinterpret_cast<BookEntry *>(data);
	book->previous = event->type;
	return FALSE;
}

gboolean BookEntry::onButtonReleased_gui(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	BookEntry *book = reinterpret_cast<BookEntry *>(data);

	if (event->button == 3 && event->type == GDK_BUTTON_RELEASE)
	{
		book->fItem = (book->createItemFirstMenu());
		// show menu
		book->createmenu();
		WulforUtil::remove_signals_from_widget(book->fItem,GDK_ALL_EVENTS_MASK);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(book->popTabMenuItem),book->fItem);
		gtk_widget_show(book->fItem);
		gtk_widget_show(book->popTabMenuItem);
		g_object_ref_sink(book->popTabMenuItem);
		gtk_menu_popup(GTK_MENU(book->popTabMenuItem),NULL, NULL, NULL,NULL,0,0);
	}
	return FALSE;
}

GtkWidget *BookEntry::createItemFirstMenu()
{
	GtkWidget *item = NULL;
	string stock, info;
	
	switch (this->type)
	{
		case Entry::FAVORITE_HUBS :
					stock = WGETS("icon-favorite-hubs");
					info = _("Favorite Hubs");
					break;
		case Entry::FAVORITE_USERS :
					stock = WGETS("icon-favorite-users");
					info = _("Favorite Users");
					break;
		case Entry::IGNORE_USERS :
					stock = WGETS("icon-ignore");
					info = _("Ignore Users");
					break;
		case Entry::PUBLIC_HUBS :
					stock = WGETS("icon-public-hubs");
					info = _("Public Hubs");
					break;
		case Entry::DOWNLOAD_QUEUE :
					stock = WGETS("icon-queue");
					info = _("Download Queue");
					break;
		case Entry::SEARCH :
					stock = WGETS("icon-search");
					info = _("Search");
					break;
		case Entry::SEARCH_ADL :
					stock = WGETS("icon-search-adl");
					info = _("ADL Search");
					break;
		case Entry::SEARCH_SPY :
					stock = WGETS("icon-search-spy");
					info = _("Spy Search");
					break;
		case Entry::FINISHED_DOWNLOADS :
					stock = WGETS("icon-finished-downloads");
					info = _("Finished Downloads");
					break;
		case Entry::FINISHED_UPLOADS :
					stock = WGETS("icon-finished-uploads");
					info = _("Finished Uploads");
					break;
		case Entry::PRIVATE_MESSAGE :
					stock = WGETS("icon-pm-online");
					info = _("Private Message");
					break;
		case Entry::HUB :
					stock = WGETS("icon-hub-offline");
					info = _("Hub");
					break;
		case Entry::SHARE_BROWSER :
					stock = WGETS("icon-directory");
					info = _("Share Browser");
					break;
		case Entry::NOTEPAD :
					stock = WGETS("icon-notepad");
					info = _("Notepad");
					break;
		case Entry::SYSTEML :
					stock = WGETS("icon-system");
					info = _("System Log");
					break;
		case Entry::ABOUT_CONFIG:
					stock = WGETS("icon-system"); //for now
					info = _("About:Config");
					break;
		default: ;
	}

	item = gtk_image_menu_item_new_from_stock(stock.c_str(),NULL);
	gtk_menu_item_set_label(GTK_MENU_ITEM(item),info.c_str());
	return item;
}
