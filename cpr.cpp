#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <print>

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <nlohmann/json.hpp>

#include "transactional.h"
#include "api.h"

using json = nlohmann::json;

class SearchIcon : public Glib::Object
{
	public:

	const FileId fileId;
	SearchIcon (const FileId& id) 
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

auto getThumbnail (Hydrus& api, FileId id)
{
	std::cout << "getting thumbnail" << std::endl;

	std::string raw = api.retrieveThumbnail(id).get();

	auto loader = Gdk::PixbufLoader::create();
	loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
	loader->close();

	return loader->get_pixbuf();
}


int main (int argc, char** argv)
{

	auto app = Gtk::Application::create("megacute...");

	auto builder = Gtk::Builder::create_from_file("../megacute.xml.ui");
	auto imageWidget = builder->get_widget<Gtk::Image>("image");
	auto window 	 = builder->get_widget<Gtk::Window>("window");
	auto grid 	 = builder->get_widget<Gtk::GridView>("icons");

	auto model   = Gio::ListStore<SearchIcon>::create();
	auto select  = Gtk::SingleSelection::create(model);
	auto factory = Gtk::SignalListItemFactory::create();

	Hydrus api("http://localhost:45869/", getenv("HYDRUS_KEY"));
	
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

			picture->set_pixbuf( getThumbnail(api, col->fileId));
		});

	grid->set_model( select);
	grid->set_factory( factory);

	for(int i = 0; i < 1; i ++)
	model->append( Glib::make_refptr_for_instance<SearchIcon>(
			new SearchIcon("ac4293228e6b64b92b2f31d32084597e9b1414f5594069fa1bf0a9fd811dd927")));

		
	
	const auto activate = [&](void)
	{
		app->add_window(*window);
		window->set_visible(true);
		//imageWidget->set( loader->get_pixbuf());
	};

	

	app->signal_activate().connect(activate);
	return app->run(argc, argv);
}

