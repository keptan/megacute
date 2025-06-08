#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <print>

#include <fstream>
#include <format>
#include <string>

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <nlohmann/json.hpp>

#include "transactional.h"
#include "api.h"
#include "commander.h"

void writeApi (const std::string& res, const std::string& key)
{
	std::ofstream rout("megacute_res");
	std::ofstream kout("megacute_key");
	rout << res;
	kout << key;
}

std::string readRes (void)
{
	std::string out;
	std::ifstream read("megacute_res");
	if(read) std::getline(read, out);
	return out;
}

std::string readKey (void)
{
	std::string out;
	std::ifstream read("megacute_key");
	if(read) std::getline(read, out);
	return out;
}


int setup (int argc, char** argv, std::string& resOut, std::string& keyOut)
{

	if(hydrusTest(readRes(), readKey()))
	{
		resOut = readRes();
		keyOut = readKey();
		return 0;
	}

	auto app = Gtk::Application::create();
	auto builder    	 = Gtk::Builder::create_from_file("setup.xml");
	auto window  			 = builder->get_widget<Gtk::Window>("sWindow");
	auto infoBox 			 = builder->get_widget<Gtk::TextView>("infoBox");
	auto requestButton = builder->get_widget<Gtk::Button>("check");
	auto addressBox		 = builder->get_widget<Gtk::Entry>("address");
	auto keyBox				 = builder->get_widget<Gtk::Entry>("key");
	auto statusBox 		 = builder->get_widget<Gtk::Text>("status");
	auto doneButton 	 = builder->get_widget<Gtk::Button>("done");

	auto textBuffer = Gtk::TextBuffer::create();

	textBuffer->set_text
	(
	 "Before running add a inc/dec rating service named 'skill'\n"
	 "And then go fetch an API key that can edit ratings, tags, search, and see paths\n"
	 "Your API key will be stored in plaintext\n"
	);
	const auto activate = [&](void)
	{
		infoBox->set_buffer(textBuffer);
		app->add_window(*window);
		window->set_visible(true);
	};


	requestButton->signal_clicked().connect(
			[&]()
			{
				const std::string res = addressBox->get_text();
				const std::string key = keyBox->get_text();
				auto test = hydrusTest(res, key);
				std::print("{} {} {}\n", res, key, test);

				if(test)
				{
					statusBox->set_text("ready!");
					doneButton->set_sensitive(true);
				}
				else
				{
					statusBox->set_text("not ready!");
					doneButton->set_sensitive(false);
				}
			});

	doneButton->signal_clicked().connect(
			[&]()
			{
				const std::string res = addressBox->get_text();
				const std::string key = keyBox->get_text();
				auto test = hydrusTest(res, key);
				std::print("{} {} {}\n", res, key, test);

				if(test)
				{
					writeApi(res, key);
					resOut = res;
					keyOut = key;
					window->close();
				}
			});

	app->signal_activate().connect(activate);
	return app->run(argc, argv);
}
