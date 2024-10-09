#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <future>
#include <print>


using namespace nlohmann;
using FileID = unsigned int;

struct Hydrus
{
	const std::string res;
	const std::string key;

	//lru cache here for the thumbnails I suppose
	//and images? we can prefetch the images maybe based on expected ELO's of winners
	//that sounds pretty sick tbqyh
	//maybe some kind of manager would be more appropriate for that instead of the endpoint itself though
	//lets make a slim endpoint class that is only responsible for getting the images
	//and then we can make the cache and shit the actual interaction layer
	//
	json doRequest (cpr::AsyncResponse request)
	{
		auto result  = request.get();
		return json::parse(result.text);
	}

	std::future<json> search (const std::vector<std::string> tags)
	{
		json formatted = tags;
		cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/search_files"},
			cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"tags", formatted.dump()}});

		return std::async(std::launch::async, &Hydrus::doRequest, this, std::move(fr));
	}

	std::future<json> retrieveThumbnail (const FileID id)
	{
		json formatted = id;
		cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/thumbnails"},
			cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"file_id", formatted.dump()}});
		return std::async(std::launch::async, &Hydrus::doRequest, this, std::move(fr));
	}
};

/*
auto main (int argc, char** argv) -> int
{
	Hydrus api{"http://localhost:45869/", getenv("HYDRUS_KEY")};
	auto future = api.search({"1girl", "blonde"});

	std::print("printing results: ");
	auto ids = future.get()["file_ids"].get<std::vector<FileID>>();
	for(const auto i : ids) std::print("{}, ", i);
	std::print("\n");

	return 0;
}
*/
