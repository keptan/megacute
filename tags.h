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
		si.push(t.name, t.mu, t.sigma);
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

	std::unordered_set<Tag, HashPT, EqualPT> names;
	std::vector<Tag> sigmaOrder;
	std::vector<Tag> muOrder;
	int seen = 0;
	std::random_device rd;
	std::mt19937 gen;


	public:
	SkillMan (const std::string p)
		: database(p), gen(rd())
	{
	}

	~SkillMan (void)
	{
		database.insert(names);
	}

	void orderInsert (const Tag& t)
	{
		const auto it = names.find(t.name);
		if(it != names.end())
		{
			sigmaOrder.erase( std::equal_range( sigmaOrder.begin(), sigmaOrder.end(), *it, sigmaSort()).first );
			muOrder.erase( std::equal_range( muOrder.begin(), muOrder.end(), *it, muSort()).first );
			names.erase(it);
		}
			names.insert(t);
			sigmaOrder.insert( std::upper_bound( sigmaOrder.begin(), sigmaOrder.end(), t, sigmaSort()), t);
			muOrder.insert( std::upper_bound( muOrder.begin(), muOrder.end(), t, muSort()), t);
	}

	Tag randomLowSigma (void)
	{
		assert( sigmaOrder.size());

		std::uniform_int_distribution dis(0, int(sigmaOrder.size())/8);

		auto out = sigmaOrder.begin() + dis(gen);
		while(out != sigmaOrder.end() && out->seen > seen) out++;
		if(out == sigmaOrder.end())
		{
			seen++;
			return *sigmaOrder.begin();
		}
		return *out;
	}

	Tag looksmatch (const Tag& t)
	{
		assert( muOrder.size());
		const auto it = std::equal_range( muOrder.begin(), muOrder.end(), t, muSort()).first;

		int leftDistance = std::min( int(std::distance(muOrder.begin(), it)), int(muOrder.size())/8);
		int rightDistance = std::min( int(std::distance(it, muOrder.end())), int(muOrder.size())/8);

		std::uniform_int_distribution dis(0 - leftDistance, rightDistance );
	 	int offset = dis(gen);
		if(offset == 0) offset = 1;

		auto out = it + offset;
		while(out != muOrder.begin() && out->seen > seen && out->name == t.name) out--;
		if(out == muOrder.begin())
		{
			seen++;
			return *(it + offset);
		}

		return *out;
	}

	Tag retrieve (const std::string& s)
	{
		auto it = names.find(s);
		if(it != names.end()) return *it;

		Tag out = database.retrieve(s);
		orderInsert(out);
		return out;
	}

	Tag random (void)
	{
		return randomLowSigma();	
	}

	void clear (void)
	{
		muOrder.clear();
		sigmaOrder.clear();
		names.clear();
	}

	std::vector< Tag> teamPad ( const std::vector< std::string>& team, int pad)
	{
		std::vector< Tag> acc;
		Tag accT{"~", 0, 0};

		for(auto s : team)
		{
			auto t = retrieve(s);
			acc.push_back(t);
			accT.sigma += t.sigma;
			accT.mu += t.mu;
		}

		accT.sigma /= team.size();
		accT.mu 	 /= team.size();

		while(acc.size() < pad) acc.push_back(accT);
		return acc;
	}

	void adjudicate ( const std::string& player1, const std::string& player2, bool tie = false
			/*std::vector< std::string>& team1, std::vector< std::string>& team2,*/ )
	{
		/*
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
		*/

		auto p1 	 = retrieve(player1);
		auto p2		 = retrieve(player2);

		std::vector<Tag> idTeam  = {p1};
		std::vector<Tag> idTeam2 = {p2};
		std::vector<Tag> idTeam3;

		auto results = trueskill( idTeam, idTeam2, idTeam3, tie);
		for(auto t : results)
		{
			std::print("{} {} {}\n", t.name, t.mu, t.sigma);
			orderInsert(t);
		}
	}

};

