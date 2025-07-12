#pragma once
#include "transactional.h"
#include "api.h"
#include "tags.h"
#include <gdkmm/pixbuf.h>

#include <queue>
#include <atomic>
#include <mutex>
#include <optional>
#include <chrono>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

class SearchIcon : public Glib::Object
{
	public:
	const FileId fileId;
	Glib::RefPtr< Gdk::Pixbuf> icon;


	SearchIcon (const FileId& id, Glib::RefPtr< Gdk::Pixbuf> icon) 
		: fileId(id), icon(icon)
	{};
};



template< typename T>
class TQueue
{
	std::queue<T> m_queue;
	std::mutex mutex;
	
	public:
	void push( T item)
	{
		std::lock_guard lock (mutex);
		m_queue.push(item);
	}

	T pop (void)
	{
		std::lock_guard lock (mutex);

			T item = m_queue.front();
			m_queue.pop();
			return item;
	}

	void clear (void)
	{
		std::lock_guard lock (mutex);
		while( m_queue.size()) m_queue.pop();
	}

	int size (void)
	{
		std::lock_guard lock (mutex);
		return m_queue.size();
	}
};

struct Commander
{
	//have some data that holds the thumbnails, and images, and tags
	//swaps them out in one motion after an update with a mutex
	//notifies the gui when there's a new valid state
	
	SkillMan tags;
	Hydrus api;
	Glib::Dispatcher thumbnail_dispatch;
	Glib::Dispatcher image_dispatch;

	std::atomic<bool> cancelSearch;
	std::future< void> searchTask;

	TQueue< Glib::RefPtr< SearchIcon>> queue;

	std::mutex iconLoadQueueMutex;
	std::queue< std::future<void>> iconLoadQueue;
	std::atomic<int> icount;
	Glib::RefPtr< SearchIcon> left;
	Glib::RefPtr< SearchIcon> right;

	int leftStreak = 0;
	int rightStreak = 0;

	Commander (const std::string database, const std::string res, const std::string key)
		: tags(database), api(res, key), left(nullptr), right(nullptr)
	{
		api.setTagService("all known tags").get();
		api.setRatingService("skill").get();
	}

	~Commander (void)
	{
		cancelSearch = true;
		if(searchTask.valid()) searchTask.wait();

		for(const auto& t : tags.names) 
		{
			if(t.adjusted() != 10.0) api.postRating( t.name, int( t.adjusted()));
		}

	}
	
	void search (const std::vector<std::string> query)
	{
		cancelSearch = true;
		if(searchTask.valid()) searchTask.wait();
		cancelSearch = false;

		searchTask = std::async( std::launch::async, [&, query=query]()
			{
				tags.clear();

				for( const auto& s : api.search( query).get())
				{
					if(cancelSearch)
					{
						queue.clear();
						return;
					}

					tags.retrieve(s);

					auto loader = Gdk::PixbufLoader::create();
					std::string raw = api.retrieveThumbnail(s).get();
					loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
					loader->close();
					
					queue.push( Glib::make_refptr_for_instance<SearchIcon>(
								new SearchIcon(s, loader->get_pixbuf())));

					if(queue.size() > 10) thumbnail_dispatch.emit();
				}
				thumbnail_dispatch.emit();
			});
	}

	void clearImageQueue (void)
	{
		while(iconLoadQueue.size())
		{
			auto& future = iconLoadQueue.front();
			if( future.valid())
			{
				if( future.wait_for(0.3s) == std::future_status::ready)
				{
					try
					{
					future.get();
					}
					catch (Glib::Error e)
					{
						std::print("imagequeue error: {}\n", e.what());
					}
					iconLoadQueue.pop();
				}
				else
				{
					break;
				}
			}
			else
			{
				iconLoadQueue.pop();
			}
		}
	}

	bool searchRunning (void)
	{
		if(!searchTask.valid()) return false;
		if(searchTask.wait_for(0s) == std::future_status::ready)
		{
			searchTask.wait();
			return false;
		}
		return true;
	}

	void compRun (const int winner)
	{
		clearImageQueue();
		if(searchRunning()) return;
		if(!left || !right) return;
		std::vector<std::string> leftTags; 
		std::vector<std::string> rightTags;   

		try
		{
			leftTags  = api.retrieveTags(left->fileId).get();
			rightTags = api.retrieveTags(right->fileId).get();
		}catch( const std::exception& e)
		{
			leftTags = {};
			rightTags = {};
			std::print("failed to retrieve tags: {}\n", e.what());
		}

		if(winner == 0) 
		{
			tags.adjudicateImages( left->fileId, right->fileId);
			tags.splitAdjudicate(leftTags, rightTags);
			leftStreak++;
			imageSelected( left->fileId);
		}
		if(winner == 1) 
		{
			tags.adjudicateImages( right->fileId, left->fileId);
			tags.splitAdjudicate(rightTags, leftTags);
			rightStreak++;
			imageSelected(left->fileId);
		}
		if(winner == 2)
		{
			tags.adjudicateImages(right->fileId, left->fileId, true);
			tags.splitAdjudicate(rightTags, leftTags, true);
			leftStreak = 0;
			rightStreak = 0;
			imageSelected(left->fileId, true);
		}
	}

	void imageSelected (const FileId file, bool reset = false)
	{
		clearImageQueue();
		if(searchRunning()) return;
		iconLoadQueue.push( std::async( std::launch::async, [&, file=file]()
		{
			int count = ++icount;

			FileId fl = file;
			FileId fr = tags.looksmatch( tags.retrieve(fl)).name;

			if(fl == fr || leftStreak > 3 || rightStreak > 1 || reset)
			{
				std::print("reset image selection and rerolled\n");
				leftStreak = 0;
				rightStreak = 0;
				auto l = tags.random();
				fl 	= l.name;
				fr = tags.looksmatch(l).name;
			}

			std::string raw = api.retrieveFile(fl).get();
			std::string raw2 = api.retrieveFile(fr).get();

			try
			{
			auto loader = Gdk::PixbufLoader::create();
			loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
			loader->close();
			auto pixbuf  =  loader->get_pixbuf();
			left = Glib::make_refptr_for_instance<SearchIcon>(
			new SearchIcon( fl, pixbuf));
			}
			catch ( Glib::Error e)
			{
				std::print("falling back to thumbnail!\n");
				std::string raw = api.retrieveThumbnail(fl).get();
				auto loader = Gdk::PixbufLoader::create();
				loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
				loader->close();
				auto pixbuf  =  loader->get_pixbuf();
				left = Glib::make_refptr_for_instance<SearchIcon>(
				new SearchIcon( fl, pixbuf));
			}

			try
			{
			auto loader2 = Gdk::PixbufLoader::create();
			loader2->write(reinterpret_cast<const guint8*>(raw2.data()), raw2.size());
			loader2->close();
			auto pixbuf2 =  loader2->get_pixbuf();
			right = Glib::make_refptr_for_instance<SearchIcon>(
			new SearchIcon( fr, pixbuf2));
			}
			catch ( Glib::Error e)
			{
				std::print("falling back to thumbnail!\n");
				std::string raw = api.retrieveThumbnail(fr).get();
				auto loader = Gdk::PixbufLoader::create();
				loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
				loader->close();
				auto pixbuf  =  loader->get_pixbuf();
				right = Glib::make_refptr_for_instance<SearchIcon>(
				new SearchIcon( fr, pixbuf));
			}

			if(icount == count) image_dispatch.emit();
		}));
	}

};
