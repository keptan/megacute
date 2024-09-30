#include <cpr/cpr.h>
#include <json/json.h>
#include <iostream>
#include <string>

int main (int argc, char** argv)
{

	const std::string str = argc > 1 ? argv[1] : "verify_access_key";

	cpr::Response r = cpr::Get( cpr::Url{"http://localhost:45869/" + str},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", "b6e34eb829c2580339f3d676d0186f19881e6db891b863c0fb74d41c204e0e3d"}
		});

	std::cout << r.text << std::endl;

	Json::Value event;

	return 0;
}
