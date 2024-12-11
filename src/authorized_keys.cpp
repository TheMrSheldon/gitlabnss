#include <config.hpp>
#include <gitlabapi.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	auto config = Config::fromFile(fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf");
	gitlab::User user;
	gitlab::GitLab gitlabclient{config};
	gitlabclient.fetchUserByUsername(argv[1], user);
	switch (std::vector<std::string> keys; gitlabclient.fetchAuthorizedKeys(user.id, keys)) {
	case gitlab::Error::Ok:
		for (auto& key : keys)
			std::cout << key << std::endl;
		return 0;
	default:
		return 1;
	}
}