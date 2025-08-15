#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <filesystem>
#include <string>

struct Config {
	static constexpr const char DefaultSocketPath[] = "/var/run/gitlabnss.sock";
	static constexpr uint16_t DefaultSocketPerms = 0666u;
	static constexpr const char DefaultSocketOwner[] = "root:root";
	static constexpr unsigned DefaultUIDOffset = 0;
	static constexpr unsigned DefaultGIDOffset = 0;
	static constexpr const char DefaultShell[] = "/usr/bin/bash";

	struct {
		std::filesystem::path socketPath;
		uint16_t socketPerms;
		std::string socketOwner;
	} general;
	struct {
		std::string baseUrl;
		std::string apikey;
	} gitlabapi;
	struct {
		std::filesystem::path homesRoot;
		unsigned uidOffset;
		unsigned gidOffset;
		std::string groupPrefix;
		std::string shell;
	} nss;

	static Config fromFile(const std::filesystem::path& file) noexcept;
};

#endif