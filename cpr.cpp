#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <print>

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <nlohmann/json.hpp>

#include "transactional.h"

using json = nlohmann::json;

class SearchIcon : public Glib::Object
{
	public:

	const std::string fileId;
	SearchIcon (const std::string& id) 
		: fileId(id)
	{
		/*
		auto loader = Gdk::PixbufLoader::create();
		loader->write(reinterpret_cast<const guint8*>(data.data()), data.size());
		loader->close();

		m_pixbuf = loader->get_pixbuf();
		*/
	};
};

auto getThumbnail (const std::string& fid)
{
	std::cout << "getting thumbnail" << std::endl;
	const std::string str =  "get_files/thumbnail";
	const std::string key = getenv("HYDRUS_KEY");

	cpr::Response r = cpr::Get( cpr::Url{"http://localhost:45869/" + str},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, 
				{"file_id", "114169906"}
		},
		cpr::Header{{"Content-Type", "application/json"}});

	std::string raw = r.text;

	auto loader = Gdk::PixbufLoader::create();
	loader->write(reinterpret_cast<const guint8*>(r.text.data()), r.text.size());
	loader->close();

	return loader->get_pixbuf();
}


int main (int argc, char** argv)
{

	auto app = Gtk::Application::create("org.gtkmm.examples.base");

	const std::string str = argc > 1 ? argv[1] : "get_files/file";
	const std::string key = getenv("HYDRUS_KEY");

	cpr::Response r = cpr::Get( cpr::Url{"http://localhost:45869/" + str},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, 
				{"file_id", "114169906"}
		},
		cpr::Header{{"Content-Type", "application/json"}});

	std::string raw = r.text;

	auto loader = Gdk::PixbufLoader::create();
	loader->write(reinterpret_cast<const guint8*>(r.text.data()), r.text.size());
	loader->close();

	auto builder = Gtk::Builder::create_from_file("../megacute.xml.ui");
	auto imageWidget = builder->get_widget<Gtk::Image>("image");
	auto window 	 = builder->get_widget<Gtk::Window>("window");
	auto grid 	 = builder->get_widget<Gtk::GridView>("icons");

	auto model   = Gio::ListStore<SearchIcon>::create();
	auto select  = Gtk::SingleSelection::create(model);
	auto factory = Gtk::SignalListItemFactory::create();
	
	factory->signal_setup().connect(
		[&](auto l){ 
			std::cout << "setup called" << std::endl;
			auto picture = Gtk::make_managed<Gtk::Picture>();
			picture->set_size_request(50,150);
			l->set_child(*picture);
		});

	factory->signal_bind().connect(
		[&](auto l){

			std::cout << "bind called" << std::endl;
			auto col = std::dynamic_pointer_cast<SearchIcon>(l->get_item());
			if (!col) return;
			auto picture = dynamic_cast<Gtk::Picture*>(l->get_child());
			if (!picture) return;

			picture->set_pixbuf( getThumbnail(col->fileId));
		});

	grid->set_model( select);
	grid->set_factory( factory);

	for(int i = 0; i < 100; i ++)
	model->append( Glib::make_refptr_for_instance<SearchIcon>(
			new SearchIcon("114169906")));

		
	
	const auto activate = [&](void)
	{
		app->add_window(*window);
		window->set_visible(true);
		imageWidget->set( loader->get_pixbuf());
	};

	
	Database db ("test");

	app->signal_activate().connect(activate);
	return app->run(argc, argv);
}

