#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <filesystem>
#include <optional>
#include <string>

struct Config {
	static constexpr unsigned DefaultUIDOffset = 0;
	static constexpr unsigned DefaultGIDOffset = 0;

	struct {
		std::string baseUrl;
		std::string apikey;
	} gitlabapi;
	struct {
		std::optional<std::filesystem::path> homesRoot;
		unsigned uidOffset;
		unsigned gidOffset;
	} nss;

	static Config fromFile(const std::filesystem::path& file) noexcept;
};

#endif