#pragma once

#include <string>
#include <ranges>
#include <unordered_set>
#include <set>
#include <memory>
#include "transactional.h"
#include "trueskill.h"

struct Tag
{
	std::string name;
	double sigma = 100;
	double mu = 100;

	double adjusted (void) const
	{
		return sigma - (mu * 3);
	}

	bool operator < (const Tag& o) const
	{
		return	adjusted() < o.adjusted();
	}
};

class Tags
{
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
		for (const auto& t : tags) 
		{
			insert(t);
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

	Tag retrieve (const std::string& s)
	{
		Tag out;
		out.name = s;
		std::string query = "'" + s + "';";
		const auto search = db.SELECT<std::string, double, double>("tag, mu, sigma FROM tagScore WHERE tag = " + query );

		if(search.size())
		{
			const auto [tag, mu, sigma] = search[0];
			out.mu = mu;
			out.sigma = sigma;
		}
		return out;

	}
};

class SkillMan
{

	struct HashPT
	{
		std::size_t operator () (const std::shared_ptr< Tag>& t) const
		{
			return std::hash< std::string>()(t->name);
		}

		std::size_t operator () (const std::string& s) const
		{
			return std::hash< std::string>()(s);
		}

		using is_transparent = void;
	};

	struct EqualPT
	{
		bool operator () (const std::shared_ptr< Tag>& a, const std::shared_ptr< Tag>& b) const
		{
			return a->name == b->name;
		}

		bool operator () (const std::shared_ptr< Tag>& a, const std::string& b) const
		{
			return a->name == b;
		}

		bool operator () (const std::string& a, const std::shared_ptr< Tag>& b) const
		{
			return a == b->name;
		}
		using is_transparent = void;
	};

	std::set< std::shared_ptr<Tag>, decltype([](const auto& a, const auto& b){return *a < *b;})> scores; //for the tags inside the images and their ratings
	std::unordered_set< std::shared_ptr<Tag>, HashPT, EqualPT> names;
	Tags database;

	public:
	SkillMan (const std::string p)
		: database(p)
	{}

	~SkillMan (void)
	{
		for(const auto t : names) database.insert(*t);	
	}

	Tag retrieve (const std::string& s)
	{
		auto it = names.find(s);
		if(it != names.end()) return **it;
		Tag out = database.retrieve(s);
		auto shared = std::make_shared<Tag>(out);
		scores.insert(shared);
		names.insert(shared);
		return *shared;
	}

	void insert (const Tag& t)
	{
		auto it = names.find(t.name);
		if(it != names.end())
		{
			**it = t;
			return;
		}

		auto shared = std::make_shared<Tag>(t);
		scores.insert(shared);
		names.insert(shared);
		return;
	}
};
