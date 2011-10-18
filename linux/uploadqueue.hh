//      uploadqueue.hh
#ifndef UPLOADQUEUE_H
#define UPLOADQUEUE_H

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>
#include <dcpp/UploadManager.h>
#include "bookentry.hh"
#include "treeview.hh"

class UploadQueue: 
		public BookEntry,
		private dcpp::UploadManagerListener
{
	public:
		UploadQueue();
		virtual ~UploadQueue();
	
	private:
		
};

#endif /* UPLOADQUEUE_H */ 