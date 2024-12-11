#include <config.hpp>

#include <toml++/toml.hpp>

#include <iostream>
#include <string>

using namespace std::string_literals;

Config Config::fromFile(const std::filesystem::path& file) noexcept {
	auto config = toml::parse_file("/etc/gitlabnss/gitlabnss.conf");
	if (!config) {
		std::cout << "Not found" << std::endl;
		// No config found
		return Config{}; /** \todo do something sensible **/
	} else {
		auto table = config.table();
		return Config{
				.gitlabapi =
						{.baseUrl = table["gitlabapi"]["base_url"].value_or(""s),
						 .apikey = table["gitlabapi"]["key"].value_or(""s)},
				.nss = {.homesRoot = table["nss"]["homes_root"].value<std::string>().transform([](auto s) {
							return std::filesystem::path(s);
						}),
						.uidOffset = table["nss"]["uid_offset"].value_or(Config::DefaultUIDOffset),
						.gidOffset = table["nss"]["gid_offset"].value_or(Config::DefaultGIDOffset)}
		};
	}
}