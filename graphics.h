#pragma once
#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include "api.h"

struct SearchIcon : public Glib::Object
{
	const FileId file;
	
	SearchIcon (const FileId id)
		: file(id)
	{}
};

struct SearchWindow
{

	Hydrus& api;
	Glib::RefPtr<Gtk::Application> app;
	Gtk::Window* window;
	Gtk::Entry* search;
	Glib::RefPtr<Gio::ListStore<SearchIcon>> model;


	SearchWindow(Hydrus& api)
		:api(api), window(nullptr)
	{
		app  	 	 = Gtk::Application::create("megacute");
		auto builder 	 = Gtk::Builder::create_from_file("../megacute.xml.ui");
		auto picture = builder->get_widget<Gtk::Picture>("picture");
		window 	 = builder->get_widget<Gtk::Window>("window");
		search 	 = builder->get_widget<Gtk::Entry>("search");

		auto grid 	 = builder->get_widget<Gtk::GridView>("icons");

		model   = Gio::ListStore<SearchIcon>::create();
		auto select  = Gtk::SingleSelection::create(model);
		auto factory = Gtk::SignalListItemFactory::create();

		factory->signal_setup().connect(
			[&](auto l){ 
				auto picture = Gtk::make_managed<Gtk::Picture>();
				picture->set_size_request(50,150);
				l->set_child(*picture);
			});

		factory->signal_bind().connect(
			[&](auto l){

				auto col = std::dynamic_pointer_cast<SearchIcon>(l->get_item());
				if (!col) return;
				auto picture = dynamic_cast<Gtk::Picture*>(l->get_child());
				if (!picture) return;

				picture->set_pixbuf( loadThumbnail(api, col->file));
			});

		grid->set_model( select);
		grid->set_factory( factory);
		app->signal_activate().connect(
				[&](void){
			app->add_window(*window);
			window->set_visible(true);
			});

		search->set_placeholder_text("1girl, blonde");
		search->signal_activate().connect(
				[&](void)
				{
					std::string str = search->get_text();
					searchBox( str);
				});

		grid->signal_activate().connect(
				[&](auto pos)
				{
					auto col = model->get_item(pos);
					if(!col) return;
					printData(col->file);
				});

	}

	void addFile (const FileId id)
	{
		model->append( Glib::make_refptr_for_instance<SearchIcon>(
					new SearchIcon(id)));
	}

	void searchBox (const std::string_view str)
	{
		auto future = api.search(str);
		auto ids = future.get().value<std::vector<FileId>>("file_ids", {});
		model->remove_all();
		for(const auto i : ids) addFile(i);

	}

	void printData (const FileId id)
	{
		api.retrieveTags(id);	
	}
};

auto loadThumbnail (Hydrus& api, const FileId id)
{

	auto future = api.retrieveThumbnail(id);
	auto loader = Gdk::PixbufLoader::create();

	const auto data = future.get();
	loader->write(reinterpret_cast<const guint8*>(data.data()), data.size());
	loader->close();

	return loader->get_pixbuf();
}
