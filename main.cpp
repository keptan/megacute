#include "graphics.h"
#include "api.h"

auto main (int argc, char** argv) -> int
{
	Hydrus api{"http://localhost:45869/", getenv("HYDRUS_KEY")};
	SearchWindow window(api);
	auto future = api.search("1girl, blonde");
	auto ids = future.get()["file_ids"].get<std::vector<FileId>>();

	for(const auto i : ids) window.addFile(i);

	api.setTagService("all known tags");


	return window.app->run(argc, argv);
}
