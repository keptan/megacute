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
#include "commander.h"

using json = nlohmann::json;

/*
auto getFile (Hydrus& api, FileId id)
{
	std::cout << "getting file" << std::endl;

	std::string raw = api.retrieveFile(id).get();

	auto loader = Gdk::PixbufLoader::create();
	loader->write(reinterpret_cast<const guint8*>(raw.data()), raw.size());
	loader->close();

	return loader->get_pixbuf();
}
*/
class TagEntry : public Glib::Object
{
	public:
	const std::string tag;


	TagEntry (const std::string& t) 
		: tag(t)
	{};
};





int main (int argc, char** argv)
{
	Commander commander( "test", getenv("HYDRUS_KEY"));
	auto app = Gtk::Application::create();

	auto builder = Gtk::Builder::create_from_file("../megacute.xml.ui");
	auto imageWidget = builder->get_widget<Gtk::Image>("picture");
	auto imageWidget2 = builder->get_widget<Gtk::Image>("picture2");
	auto window 	 = builder->get_widget<Gtk::Window>("window");
	auto grid 	 = builder->get_widget<Gtk::GridView>("icons");
	auto tagList = builder->get_widget<Gtk::ListView>("tagList");
	auto box 			= builder->get_widget<Gtk::Box>("box");
	auto tagEntry = builder->get_widget<Gtk::Entry>("search");
	auto searchButton = builder->get_widget<Gtk::Button>("searchButton");

	auto model   = Gio::ListStore<SearchIcon>::create();
	auto select  = Gtk::SingleSelection::create(model);
	auto factory = Gtk::SignalListItemFactory::create();

	auto tagModel 	= Gio::ListStore<TagEntry>::create();
	auto tagSelect  = Gtk::SingleSelection::create(tagModel);
	auto tagFactory = Gtk::SignalListItemFactory::create();


	
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

			picture->set_pixbuf( col->icon);
		});

	grid->set_model( select);
	grid->set_factory( factory);
	grid->signal_activate().connect(
			[&](auto l)
			{
				auto col = std::dynamic_pointer_cast<SearchIcon>(model->get_object(l));
				imageWidget->set( col->icon);
				imageWidget2->set( col->icon);
				commander.imageSelected( col->fileId);

			});



		tagFactory->signal_setup().connect(
				[&](auto l)
				{
					auto label = Gtk::make_managed<Gtk::Label>("", Gtk::Align::START);
					l->set_child(*label);
				});

			tagFactory->signal_bind().connect(
					[&](auto l)
					{
						auto tag = std::dynamic_pointer_cast<TagEntry>(l->get_item());
						if(!tag) return;
						auto label = dynamic_cast<Gtk::Label*>(l->get_child());
						if(!label) return;

						label->set_text( tag->tag);
					});

				tagList->set_model( tagSelect);
				tagList->set_factory( tagFactory);
				tagList->signal_activate().connect(
						[&](auto l)
						{
							tagModel->remove(l);
						});


		/* left and right competition logic*/
		auto key_controller = Gtk::EventControllerKey::create();
    key_controller->signal_key_pressed().connect(
        [&](const guint keyval, const guint keycode, Gdk::ModifierType state) -> bool 
				{
            if (keyval == GDK_KEY_Left) {
								commander.compRun(0);
                return true;
            } else if (keyval == GDK_KEY_Right) {
								commander.compRun(1);
                return true;
            } else if (keyval == GDK_KEY_Up) {
							commander.compRun(2);
							return true;
						}

            return false; // Allow event propagation
        }, false
    );

		auto motion_controller = Gtk::EventControllerMotion::create();
		motion_controller->signal_enter().connect([&](const auto a, const auto b)
				{
					box->grab_focus();
				});


		box->add_controller(key_controller);
		box->add_controller(motion_controller);
		box->set_can_focus(true);
		box->set_focusable(true);
		box->set_focus_on_click(true);

		/* selecting images and searching logic */

	commander.thumbnail_dispatch.connect( [&](){
			for(int c = 0; c < commander.queue.size(); c++)
			{
				auto i = commander.queue.pop();	
				model->append(i);
			};});

	commander.image_dispatch.connect( [&]()
			{
				if(commander.left) imageWidget->set( (*commander.left).icon);
				if(commander.right) imageWidget2->set( (*commander.right).icon);
			});

	tagEntry->signal_activate().connect(
			[&]()
			{
				const auto text = tagEntry->get_text();
				if(!text.size()) return;
				tagModel->append( Glib::make_refptr_for_instance<TagEntry>(new TagEntry(text)));
				tagEntry->set_text("");
			});

	searchButton->signal_clicked().connect(
			[&]()
			{
				std::vector<std::string> tags;
				for(int i = 0; i < tagModel->get_n_items(); i++)
				{
					auto string =  tagModel->get_item(i);
					if(string) tags.push_back( string->tag);
				}
				model->remove_all();
				commander.search(tags);
			});

	std::vector<std::string> defaultTags = {"-cool", "system:archive", "system:filetype is image"};
	for(const auto& t : defaultTags) tagModel->append( Glib::make_refptr_for_instance<TagEntry>(new TagEntry(t)));
	
	const auto activate = [&](void)
	{
		app->add_window(*window);
		window->set_visible(true);
		//imageWidget->set( loader->get_pixbuf());
	};

	

	app->signal_activate().connect(activate);
	return app->run(argc, argv);
}

