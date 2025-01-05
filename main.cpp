//#include "graphics.h"
#include "transactional.h"
#include "api.h"
#include <ranges>
#include <concepts>



auto main (int argc, char** argv) -> int
{
	Hydrus api{"http://localhost:45869/", getenv("HYDRUS_KEY")};
	//SearchWindow window(api);
	api.setTagService("all known tags").get();
	auto future = api.search({"1girl", "blonde"}).get();
	//for(const auto i : ids) window.addFile(i);

	SkillMan tags("test");
	Tag a = tags.retrieve("sigma");
	//Tag b{"sigma", 0, 0};
	//tags.insert(b);
	std::print("{} {} {}\n", a.name, a.mu, a.sigma);


}
