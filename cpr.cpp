#include <cpr/cpr.h>
#include <json/json.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <print>

int main (int argc, char** argv)
{

	const std::string str = argc > 1 ? argv[1] : "verify_access_key";
	const std::string key = getenv("HYDRUS_KEY");

	if(key.size() == 0) std::cout << "set the HYDRUS_KEY enviroment variable to your hydrus API access key";

	cpr::Response r = cpr::Get( cpr::Url{"http://localhost:45869/" + str},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}
		});

	std::print("{0}\n", r.text);

	Json::Value event;

	return 0;
}

