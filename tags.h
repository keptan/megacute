#pragma once

#include <string>
#include <ranges>
#include <unordered_set>
#include <set>
#include <memory>
#include <random>
#include <cstdlib>
#include "transactional.h"
#include "trueskill.h"
#include "api.h"
#include "trueskill.h"


struct Tag
{
	std::string name;
	double mu = 100.0;
	double sigma = 30.0;
	int seen = 0;

	double adjusted (void) const
	{
		return mu - (sigma * 3);
	}

	bool operator < (const Tag& o) const
	{
		if(mu == o.mu) return name < o.name;
		return	mu < o.mu;
	}

	bool operator == (const Tag& o) const
	{
		return name == o.name;
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
		db.CREATE("TABLE IF NOT EXISTS tags (tag STRING NOT NULL PRIMARY KEY UNIQUE)");

		db.CREATE("TABLE IF NOT EXISTS tagScore"
				"(tag STRING NOT NULL PRIMARY KEY UNIQUE REFERENCES tags,"
				"mu REAL NOT NULL, sigma REAL NOT NULL)");

		t.commit();
	}
 
	template < std::ranges::input_range R>
	requires std::same_as< std::ranges::range_value_t<R>, Tag>
	void insert (const R& tags)
	{
		auto transaction  = db.transaction();
		auto ti = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
		auto si = db.INSERT("OR REPLACE INTO tagScore (tag, mu, sigma) VALUES (?, ?, ?)");

		for (const auto& t : tags) 
		{
			ti.push(t.name);
			si.push(t.name, t.mu, t.sigma);
		}

		transaction.commit();
	}

	void insert (const Tag& t)
	{
		auto ti = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
		auto si = db.INSERT("OR REPLACE INTO tagScore (tag, mu, sigma) VALUES (?, ?, ?)");

		ti.push(t.name);
		si.push(t.name, t.mu, t.sigma);
	}

	Tag retrieve (const std::string& s)
	{
		Tag out;
		out.name = s;
		std::string query = s;
		const auto search = db.SELECT<std::string, double, double>("tag, mu, sigma FROM tagScore WHERE tag = ?", s);

		if(search.size())
		{
			const auto [tag, mu, sigma] = search[0];
			out.mu = mu;
			out.sigma = sigma;
		}
		return out;

	}
};

std::vector< Tag> trueskill ( const std::vector< Tag>& winner, const std::vector< Tag>& loser, const std::vector< Tag>& shared, bool tie = false)
{
	std::vector< Tag> out;
	std::vector< std::vector <double>> players;
	int idIt = 0;

	for(const auto& t : winner)
	{
		if(t.name == "~")
		{
			players.push_back( {t.mu, t.sigma, 1, 1, -1});
			continue;
		}
		players.push_back( {t.mu, t.sigma, 1, 1, double(idIt)});
		out.push_back(t);
		idIt++;
	}

	for(const auto& t : loser)
	{
		if(t.name == "~")
		{
			players.push_back( {t.mu, t.sigma, 2, 1, -1});
			continue;
		}
		players.push_back( {t.mu, t.sigma, 2, 1, double(idIt)});
		out.push_back(t);
		idIt++;
	}

	for(const auto& t : shared)
	{
		if(t.name == "~")
		{
			players.push_back( {t.mu, t.sigma, 3, 1, -1});
			continue;
		}
		players.push_back( {t.mu, t.sigma, 3, 1, double(idIt)});
		out.push_back(t);
		idIt++;
	}


	std::vector<int> teams = {0,1};
	if(shared.size()) teams = {0,2,1};

	if(tie)
	{
		teams = {0, 0};
		if(shared.size()) teams = {0, 0, 0};
	}

	if(shared.size()) rate( players, teams ,3); 
	else rate( players, teams, 2);

	for(const auto& p : players)
	{
		if(int(p[4]) != -1)
		{
			out[int(p[4])].mu = p[0];
			out[int(p[4])].sigma = p[1];
		}
	}

	return out;

}

class SkillMan
{

	struct HashPT
	{
		std::size_t operator () (const Tag& t) const
		{
			return std::hash< std::string>()(t.name);
		}

		std::size_t operator () (const std::string& s) const
		{
			return std::hash< std::string>()(s);
		}

		using is_transparent = void;
	};

	struct EqualPT
	{
		bool operator () (const Tag& a, const Tag& b) const
		{
			return a.name == b.name;
		}

		bool operator () (const Tag& a, const std::string& b) const
		{
			return a.name == b;
		}

		bool operator () (const std::string& a, const Tag& b) const
		{
			return a == b.name;
		}
		using is_transparent = void;
	};

	struct muSort
	{
		bool operator () (const Tag& a, const Tag& b) const
		{
			if( a.mu == b.mu) return a.name < b.name;
			return a.mu < b.mu;
		}
	};

	struct sigmaSort
	{
		bool operator () (const Tag& a, const Tag& b) const
		{
			if( a.sigma == b.sigma) return a.name < b.name;
			return a.sigma > b.sigma;
		}
	};

	Tags database;

	std::unordered_set<Tag, HashPT, EqualPT> otherTags;
	std::vector<Tag> sigmaOrder;
	std::vector<Tag> muOrder;
	int seen = 0;
	std::random_device rd;
	std::mt19937 gen;


	public:
	std::unordered_set<Tag, HashPT, EqualPT> names;

	SkillMan (const std::string p)
		: database(p), gen(rd())
	{
	}

	~SkillMan (void)
	{
		database.insert(otherTags);
		database.insert(names);
	}

	void orderInsert (const Tag& t)
	{
		const auto it = names.find(t.name);
		if(it != names.end())
		{
			auto sigmaIt = std::equal_range( sigmaOrder.begin(), sigmaOrder.end(), *it, sigmaSort()).first;
			auto muIt = std::equal_range( muOrder.begin(), muOrder.end(), *it, muSort()).first;
			sigmaOrder.erase(sigmaIt);
			muOrder.erase(muIt);
			names.erase(it);
		}
			names.insert(t);
			sigmaOrder.insert( std::upper_bound( sigmaOrder.begin(), sigmaOrder.end(), t, sigmaSort()), t);
			muOrder.insert( std::upper_bound( muOrder.begin(), muOrder.end(), t, muSort()), t);
	}

	void tagInsert (const Tag& t)
	{
		const auto it = otherTags.find(t.name);
		if(it != otherTags.end()) otherTags.erase(it);
		otherTags.insert(t);
	}

	Tag randomLowSigma (void)
	{
		assert( sigmaOrder.size());

		//grab from the lowest 5% of sigmas...
		auto left  = sigmaOrder.begin();
		auto right = left + std::min( sigmaOrder.size() ,std::max( static_cast<std::size_t>(20), sigmaOrder.size() / 20)); 

		//grab a not seen low sigma image
		auto min = left;
		for(auto i = left; i < right; i++)
		{
			if(i->seen < min->seen) min = i;
		}
		return *min;
	}

	Tag looksmatch (const Tag& t)
	{
		assert( muOrder.size());
		const auto it = std::upper_bound( muOrder.begin(), muOrder.end(), t, muSort());

		auto left  = std::max( muOrder.begin(), it - 3);
		auto right = std::min( muOrder.end(), it + 10);
		
		//grab within 15 images the most recently not seen
		auto min = left;
		for(auto i = left; i < right; i++)
		{
			if(i->seen < min->seen && t != *i) min = i;
		}

		return *min;
	}

	Tag retrieve (const std::string& s)
	{
		auto it = names.find(s);
		if(it != names.end()) 
		{
			return *it;
		}

		Tag out = database.retrieve(s);
		orderInsert(out);
		return out;
	}

	Tag retrieveTag (const std::string& s)
	{
		auto it = otherTags.find(s);
		if(it != otherTags.end()) return *it;

		Tag out = database.retrieve(s);
		tagInsert(out);
		return out;
	}

	Tag random (void)
	{
		return randomLowSigma();	
	}

	void clear (void)
	{
		database.insert(names);
		muOrder.clear();
		sigmaOrder.clear();
		names.clear();
	}

	std::vector< Tag> teamPad ( const std::vector< std::string>& team, int pad)
	{
		std::vector< Tag> acc;
		Tag accT{"~", 0, 0};

		if(team.size() == 0)
		{
			accT.sigma = 100;
			accT.mu = 30;
		}
		else
		{
			for(auto s : team)
			{
				auto t = retrieveTag(s);
				acc.push_back(t);
				accT.sigma += t.sigma;
				accT.mu += t.mu;
			}

			accT.sigma /= team.size();
			accT.mu 	 /= team.size();
		}

		while(acc.size() < pad) acc.push_back(accT);
		return acc;
	}

	void splitAdjudicate (std::vector< std::string>& team1, std::vector< std::string>& team2, bool tie = false)
	{
		std::vector< std::string> team1Characters;
		std::vector< std::string> team2Characters;
		std::vector< std::string> team1Artists;
		std::vector< std::string> team2Artists;

		const auto stripper = [&](auto& team, auto& dest, auto& string)
		{
			auto t1c = std::remove_if(team.begin(), team.end(), [&](auto& s){ return s.find(string) != std::string::npos;});
			dest.insert(dest.end(), t1c, team.end());
			team.erase(t1c, team.end());
		};

		stripper(team1, team1Characters, "character:");
		stripper(team2, team2Characters, "character:");

		stripper(team1, team1Artists, "creator:");
		stripper(team2, team2Artists, "creator:");

		if(team1Characters.size() || team2Characters.size()) adjudicateTags( team1Characters, team2Characters);
		if(team1Artists.size() || team2Artists.size()) adjudicateTags( team1Artists, team2Artists);
		if(team1.size() || team2.size()) adjudicateTags( team1, team2);
	}

	void adjudicateTags (std::vector< std::string>& team1, std::vector< std::string>& team2, bool tie = false )
	{
		std::unordered_set< std::string> set1;
		std::vector< std::string> team1a;
		std::vector< std::string> team2a;
		std::vector< std::string> team3a;

		for(const auto& s : team1) set1.insert(s);
		for(const auto& s : team2)
		{
			if( set1.count(s))
			{
				set1.erase(s);
				team3a.push_back(s);
			}
			else team2a.push_back(s);
		}
		for(const auto& s : set1) team1a.push_back(s);

		auto tags1 = teamPad(team1a, std::max( {team1a.size(), team2a.size(), team3a.size()}));
		auto tags2 = teamPad(team2a, std::max( {team1a.size(), team2a.size(), team3a.size()}));
		auto tags3 = teamPad(team3a, std::max( {team1a.size(), team2a.size(), team3a.size()}));
		auto tResults = trueskill(tags1, tags2, tags3, tie);

		int c = 0;
		for(auto& t : tResults)
		{
			t.seen = seen;

			if (c++ == 5) 
			{
				std::print("...\n");
			}
			if(c < 5) std::print("tag: {} score: {:.2f} {}\n", t.name, t.mu - (3* t.sigma), t.seen);

			tagInsert(t);
		}
	}

	void adjudicateImages (const std::string& winner, const std::string& loser, bool tie = false)
	{
		seen++;
		auto p1 	 = retrieve(winner);
		auto p2		 = retrieve(loser);
		std::vector<Tag> idTeam  = {p1};
		std::vector<Tag> idTeam2 = {p2};
		std::vector<Tag> idTeam3;

		auto results = trueskill( idTeam, idTeam2, idTeam3, tie);
		for(auto& t : results)
		{
			t.seen = seen;
			std::print("image: {} score: {:.2f} {}\n", t.name.substr(0,5), t.mu - (3* t.sigma), t.seen);
			orderInsert(t);
		}
	}

};

