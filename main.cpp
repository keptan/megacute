//#include "graphics.h"
#include "transactional.h"
#include "api.h"
#include <ranges>
#include <concepts>


class Tags
{
	struct Tag
	{
		std::string name;
		double sigma;
		double mu;
	};

	Database db;
	public:
	Tags (std::string path)
		:db(path)
	{
		auto t = db.transaction();
		db.CREATE("TABLE IF NOT EXISTS tags (tag STRING NOT_NULL PRIMARY KEY UNIQUE)");

		db.CREATE("TABLE IF NOT EXISTS tagScore"
				"(tag STRING NOT_NULL PRIMARY_KEY UNIQUE REFERENCES tags,"
				"mu REAL NOT_NULL, sigma REAL NOT_NULL)");

		t.commit();
	}

	template < std::ranges::input_range R>
	requires std::same_as< std::ranges::range_value_t<R>, Tag>
	void insert (const R& tags)
	{
		auto t  = db.transaction();
		auto ti = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
		auto si = db.INSERT("OR IGNORE INTO tagScore (tag, mu, sigma) VALUES (?, ?, ?)");

		for (const auto& t : tags) 
		{
			ti.push(t.name);
			si.push(t.name, t.sigma, t.mu);
		}

		t.commit();
	}

	void insert (const Tag& t)
	{
		auto ti = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
		auto si = db.INSERT("OR IGNORE INTO tagScore (tag, mu, sigma) VALUES (?, ?, ?)");

		ti.push(t.name);
		si.push(t.name, t.sigma, t.mu);
	}
};

auto main (int argc, char** argv) -> int
{
	Hydrus api{"http://localhost:45869/", getenv("HYDRUS_KEY")};
	//SearchWindow window(api);
	auto future = api.search("1girl, blonde");
	auto ids = future.get()["file_ids"].get<std::vector<FileId>>();

	//for(const auto i : ids) window.addFile(i);

	api.setTagService("all known tags");

	//lets test our database library from cureMaid
	Database db ("test");
		//return window.app->run(argc, argv);
}
