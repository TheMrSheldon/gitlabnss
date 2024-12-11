#include <nss.h>
#include <pwd.h>
#include <shadow.h>

#include <filesystem>
#include <span>
#include <spanstream>

#include <config.hpp>
#include <gitlabapi.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

using gitlab::Error;
using gitlab::User;

const char DefaultShell[] = "/usr/bin/bash";
const fs::path HomeDirBase = fs::path("/") / "gitlabhome";

static auto initLogger() {
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	//auto basic_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/var/log/nss_gitlab.log");
	//std::vector<spdlog::sink_ptr> sinks{console_sink, basic_sink};
	std::vector<spdlog::sink_ptr> sinks{console_sink};
	auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
	logger->flush_on(spdlog::level::info);
	SPDLOG_LOGGER_INFO(logger, "Logger created");
	return logger;
}

static auto logger = initLogger();
static auto config = Config::fromFile(fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf");
static gitlab::GitLab gitlabclient{config};

void populatePasswd(passwd& pwd, User& user, std::span<char> buffer) {
	auto stream = std::ospanstream(buffer);
	// Username
	pwd.pw_name = buffer.data() + stream.tellp();
	stream << user.username << '\0';
	// Password
	const char Password[] = "";
	pwd.pw_passwd = buffer.data() + stream.tellp();
	stream << Password << '\0';
	// UID
	pwd.pw_uid = user.id + config.nss.uidOffset;
	// GID
	/** \todo **/
	// Real Name
	pwd.pw_gecos = buffer.data() + stream.tellp();
	stream << user.name << '\0';
	// Shell
	pwd.pw_shell = buffer.data() + stream.tellp();
	stream << DefaultShell << '\0';
	// Home directory
	pwd.pw_dir = buffer.data() + stream.tellp();
	std::string homedir = HomeDirBase / user.username;
	fs::create_directories(HomeDirBase / user.username);
	chown(homedir.c_str(), pwd.pw_uid, pwd.pw_gid);
	stream << homedir << '\0';

	SPDLOG_LOGGER_INFO(logger, "Written PWD:");
	SPDLOG_LOGGER_INFO(logger, "    user name: {}", pwd.pw_name);
	SPDLOG_LOGGER_INFO(logger, "    full name: {}", pwd.pw_gecos);
	SPDLOG_LOGGER_INFO(logger, "    homedir:   {}", pwd.pw_dir);
	SPDLOG_LOGGER_INFO(logger, "    shell:     {}", pwd.pw_shell);
}

extern "C" {
#if 0
nss_status _nss_gitlab_getspnam_r(const char* name, spwd* spwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_INFO(logger, "getspnam_r({})", name);
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
};
#endif

nss_status _nss_gitlab_getpwuid_r(uid_t uid, passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_INFO(logger, "getpwuid_r({})", uid);
	if (uid < config.nss.uidOffset)
		return nss_status::NSS_STATUS_NOTFOUND;
	SPDLOG_LOGGER_INFO(logger, "Fetching User {}", uid - config.nss.uidOffset);
	Error err;
	switch (gitlab::User user; err = gitlabclient.fetchUserByID(uid - config.nss.uidOffset, user)) {
	case Error::Ok:
		//gitlabclient.fetchGroups(user);
		populatePasswd(*pwd, user, {buf, buflen});
		SPDLOG_LOGGER_INFO(logger, "Found!");
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_INFO(logger, "Not Found");
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_INFO(logger, "Other Error");
		SPDLOG_LOGGER_INFO(logger, "Error {}", static_cast<unsigned>(err));
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}

nss_status _nss_gitlab_getpwnam_r(const char* name, passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_INFO(logger, "getpwnam_r({})", name);
	switch (gitlab::User user; gitlabclient.fetchUserByUsername(name, user)) {
	case Error::Ok:
		//gitlabclient.fetchGroups(user);
		populatePasswd(*pwd, user, {buf, buflen});
		SPDLOG_LOGGER_INFO(logger, "Found!");
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_INFO(logger, "Not Found");
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_INFO(logger, "Other Error");
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}

#if 0
nss_status _nss_gitlab_setpwent() {
	SPDLOG_LOGGER_INFO(logger, "setpwent()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}

nss_status _nss_gitlab_getpwent_r(passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_INFO(logger, "getpwent_r()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}

nss_status _nss_gitlab_endpwent() {
	SPDLOG_LOGGER_INFO(logger, "endpwent()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}
#endif

/**********************************************************************************************************************/
/* GROUPS                                                                                                             */
/**********************************************************************************************************************/
nss_status _nss_gitlab_getgrgid_r(gid_t gid, group* result_buf, char* buffer, size_t buflen, group** result) {
	SPDLOG_LOGGER_INFO(logger, "getgrgid_r({})", gid);
	return nss_status::NSS_STATUS_NOTFOUND;
}
nss_status _nss_gitlab_getgrnam_r(const char* name, group* result_buf, char* buffer, size_t buflen, group** result) {
	SPDLOG_LOGGER_INFO(logger, "getgrnam_r({})", name);
	return nss_status::NSS_STATUS_NOTFOUND;
}
}