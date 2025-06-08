#pragma once 

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <future>
#include <print>
#include "tags.h"


using namespace nlohmann;
using FileId = std::string;

bool hydrusTest (const std::string& res, const std::string& key)
{
	cpr::Response r = cpr::Get( cpr::Url{res + "verify_access_key"},
			cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}});
	if (r.status_code != 200) return false;
	return true;
	json response = json::parse(r.text);
}

class Hydrus
{
	const std::string res;
	const std::string key;

 	std::string tagService;
	std::string ratingService;

	//lru cache here for the thumbnails I suppose
	//and images? we can prefetch the images maybe based on expected ELO's of winners
	//that sounds pretty sick tbqyh
	//maybe some kind of manager would be more appropriate for that instead of the endpoint itself though
	//lets make a slim endpoint class that is only responsible for getting the images
	//and then we can make the cache and shit the actual interaction layer
	
	json doRequest (cpr::AsyncResponse request)
	{
		auto result  = request.get();
		return json::parse(result.text);
	}

	std::string fileRequest (cpr::AsyncResponse request)
	{
		auto result = request.get();
		return result.text;
	}

	public:

	Hydrus (const std::string_view res, const std::string_view k)
		: res(res), key(k)
	{

	}

	std::future<std::string> postRating (const FileId id, int rating)
	{
		json data;
		data["hash"] = id;
		data["rating"] = rating;
		data["rating_service_key"] = ratingService;

		cpr::AsyncResponse fr = cpr::PostAsync( 
		cpr::Url{res + "edit_ratings/set_rating"},
		cpr::Header{{"Hydrus-Client-API-Access-Key", key}, {"Content-Type", "application/json"}},
		cpr::Body {data.dump()});

		return std::async(std::launch::async, &Hydrus::fileRequest, this, std::move(fr));
	}

	std::future<std::string> retrieveThumbnail (const FileId id)
	{
		cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/thumbnail"},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"hash", id}});
		return std::async(std::launch::async, &Hydrus::fileRequest, this, std::move(fr));
	}

	std::future<std::string> retrieveFile (const FileId id)
	{
		cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/file"},
		cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"hash", id}});
		return std::async(std::launch::async, &Hydrus::fileRequest, this, std::move(fr));
	}

	std::future<json> retrieveMetadata (const FileId id)
	{
		json formatted = id;
		cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/file_metadata"},
			cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"hash", id}});
		return std::async(std::launch::async, &Hydrus::doRequest, this, std::move(fr));
	}

	std::future< std::vector<std::string>> retrieveTags (const FileId id)
	{
		return std::async( std::launch::async, [&, id=id]()
				{
					auto json = retrieveMetadata(id).get();
					auto tags = json.at("metadata").at(0).at("tags").at(tagService).at("display_tags").at("0").get<std::vector<std::string>>();
					//std::vector<std::string> tags;
					return tags;
				});
	}

	std::future<void> setTagService (const std::string str)
	{
		return std::async( std::launch::async, [&, str=str]()
				{
					json formatted = str;
					auto services = cpr::GetAsync( cpr::Url{res + "get_service"},
						cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"service_name", str}});

					json response = json::parse(services.get().text);
					tagService = response.at("service").at("service_key");
					});
	}

	std::future<void> setRatingService (const std::string str)
	{
				return std::async( std::launch::async, [&, str=str]()
				{
					json formatted = str;
					auto services = cpr::GetAsync( cpr::Url{res + "get_service"},
						cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"service_name", str}});

					json response = json::parse(services.get().text);
					ratingService = response.at("service").at("service_key");
					});
	}

	std::future< std::vector<FileId>> search (const std::vector<std::string> tags)
	{
		return std::async(std::launch::async, [&, tags=tags]()
				{
					json formatted = tags;
					cpr::AsyncResponse fr = cpr::GetAsync( cpr::Url{res + "get_files/search_files"},
					cpr::Parameters{{"Hydrus-Client-API-Access-Key", key}, {"return_hashes", "true"}, {"tags", formatted.dump()}});

					auto req = std::async(std::launch::async, &Hydrus::doRequest, this, std::move(fr));
					auto ids = req.get()["hashes"].get<std::vector<FileId>>();
					return ids;
				});
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
