#include <config.hpp>
#include <error.hpp>
#include <rpcclient.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <grp.h>
#include <nss.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/stat.h>

#include <filesystem>
#include <span>
#include <spanstream>

namespace fs = std::filesystem;

static auto initLogger() {
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	std::vector<spdlog::sink_ptr> sinks{console_sink};
	auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
	logger->set_level(spdlog::level::trace);
	logger->flush_on(spdlog::level::info);
	SPDLOG_LOGGER_DEBUG(logger, "Logger created");
	return logger;
}

static auto logger = initLogger();
static auto config = Config::fromFile(fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf");

void populatePasswd(passwd& pwd, const User::Reader& user, std::span<char> buffer) {
	auto stream = std::ospanstream(buffer);
	// Username
	pwd.pw_name = buffer.data() + stream.tellp();
	stream << user.getUsername().cStr() << '\0';
	// Password
	const char Password[] = "*"; // user can't login with PW: https://www.man7.org/linux/man-pages/man5/shadow.5.html
	pwd.pw_passwd = buffer.data() + stream.tellp();
	stream << Password << '\0';
	// UID
	pwd.pw_uid = user.getId() + config.nss.uidOffset;
	// GID
	pwd.pw_gid = user.getGroups().size() > 0 ? (user.getGroups()[0].getId() + config.nss.gidOffset) : 65534 /*nogroup*/;
	// Real Name
	pwd.pw_gecos = buffer.data() + stream.tellp();
	stream << user.getName().cStr() << '\0';
	// Shell
	pwd.pw_shell = buffer.data() + stream.tellp();
	stream << config.nss.shell << '\0';
	// Home directory
	pwd.pw_dir = buffer.data() + stream.tellp();
	std::string homedir = config.nss.homesRoot / user.getUsername().cStr();
	fs::create_directories(config.nss.homesRoot / user.getUsername().cStr());
	chown(homedir.c_str(), pwd.pw_uid, pwd.pw_gid);
	chmod(homedir.c_str(), config.nss.homePerms);
	stream << homedir << '\0';
}

extern "C" {
#if 0
nss_status _nss_gitlab_getspnam_r(const char* name, spwd* spwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_DEBUG(logger, "getspnam_r({})", name);
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
};
#endif

nss_status _nss_gitlab_getpwuid_r(uid_t uid, passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_DEBUG(logger, "getpwuid_r({})", uid);
	if (uid < config.nss.uidOffset)
		return nss_status::NSS_STATUS_NOTFOUND;
	SPDLOG_LOGGER_DEBUG(logger, "Fetching User {}", uid - config.nss.uidOffset);
	auto io = kj::setupAsyncIo();
	auto& waitScope = io.waitScope;
	auto daemon = initClient(io);

	if (!daemon)
		return NSS_STATUS_UNAVAIL;

	auto request = daemon->getUserByIDRequest();
	request.setId(uid - config.nss.uidOffset);
	auto promise = request.send().wait(waitScope);

	auto user = promise.getUser();
	switch (static_cast<Error>(promise.getErrcode())) {
	case Error::Ok:
		populatePasswd(*pwd, user, {buf, buflen});
		SPDLOG_LOGGER_DEBUG(logger, "Found!");
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_DEBUG(logger, "Not Found");
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_ERROR(logger, "Other Error");
		SPDLOG_LOGGER_ERROR(logger, "Error {}", promise.getErrcode());
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}

nss_status _nss_gitlab_getpwnam_r(const char* name, passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_DEBUG(logger, "getpwnam_r({})", name);
	auto io = kj::setupAsyncIo();
	auto& waitScope = io.waitScope;
	auto daemon = initClient(io);

	if (!daemon)
		return NSS_STATUS_UNAVAIL;

	auto request = daemon->getUserByNameRequest();
	request.setName(name);
	auto promise = request.send().wait(waitScope);

	auto user = promise.getUser();
	switch (static_cast<Error>(promise.getErrcode())) {
	case Error::Ok:
		populatePasswd(*pwd, user, {buf, buflen});
		SPDLOG_LOGGER_DEBUG(logger, "Found!");
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_DEBUG(logger, "Not Found");
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_ERROR(logger, "Other Error");
		SPDLOG_LOGGER_ERROR(logger, "Error {}", promise.getErrcode());
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}

#if 0
nss_status _nss_gitlab_setpwent() {
	SPDLOG_LOGGER_DEBUG(logger, "setpwent()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}

nss_status _nss_gitlab_getpwent_r(passwd* pwd, char* buf, size_t buflen, int* errnop) {
	SPDLOG_LOGGER_DEBUG(logger, "getpwent_r()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}

nss_status _nss_gitlab_endpwent() {
	SPDLOG_LOGGER_DEBUG(logger, "endpwent()");
	/** \todo implement **/
	return nss_status::NSS_STATUS_NOTFOUND;
}
#endif

/**********************************************************************************************************************/
/* GROUPS                                                                                                             */
/**********************************************************************************************************************/
void populateGroup(group& group, const Group::Reader& obj, std::span<char> buffer) {
	auto stream = std::ospanstream(buffer);
	// Username
	group.gr_name = buffer.data() + stream.tellp();
	stream << obj.getName().cStr() << '\0';
	// Password
	const char Password[] = "*"; // user can't login with PW: https://www.man7.org/linux/man-pages/man5/shadow.5.html
	group.gr_passwd = buffer.data() + stream.tellp();
	stream << Password << '\0';
	// GID
	group.gr_gid = obj.getId() + config.nss.gidOffset;
	// Members
	group.gr_mem = nullptr;
}

nss_status _nss_gitlab_getgrgid_r(gid_t gid, group* result_buf, char* buf, size_t buflen, group** result) {
	SPDLOG_LOGGER_INFO(logger, "getgrgid_r({})", gid);
	if (gid < config.nss.gidOffset)
		return nss_status::NSS_STATUS_NOTFOUND;
	auto io = kj::setupAsyncIo();
	auto& waitScope = io.waitScope;
	auto daemon = initClient(io);

	if (!daemon)
		return NSS_STATUS_UNAVAIL;

	auto request = daemon->getGroupByIDRequest();
	request.setId(gid - config.nss.gidOffset);
	auto promise = request.send().wait(waitScope);

	auto user = promise.getGroup();
	switch (static_cast<Error>(promise.getErrcode())) {
	case Error::Ok:
		populateGroup(*result_buf, user, {buf, buflen});
		SPDLOG_LOGGER_DEBUG(logger, "Found!");
		*result = result_buf;
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_DEBUG(logger, "Not Found");
		*result = nullptr;
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_ERROR(logger, "Other Error");
		SPDLOG_LOGGER_ERROR(logger, "Error {}", promise.getErrcode());
		*result = nullptr;
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}

nss_status _nss_gitlab_getgrnam_r(const char* name, group* result_buf, char* buf, size_t buflen, group** result) {
	SPDLOG_LOGGER_INFO(logger, "getgrnam_r({})", name);
	auto io = kj::setupAsyncIo();
	auto& waitScope = io.waitScope;
	auto daemon = initClient(io);

	if (!daemon)
		return NSS_STATUS_UNAVAIL;

	auto request = daemon->getGroupByNameRequest();
	request.setName(name);
	auto promise = request.send().wait(waitScope);

	auto user = promise.getGroup();
	switch (static_cast<Error>(promise.getErrcode())) {
	case Error::Ok:
		populateGroup(*result_buf, user, {buf, buflen});
		SPDLOG_LOGGER_DEBUG(logger, "Found!");
		*result = result_buf;
		return nss_status::NSS_STATUS_SUCCESS;
	case Error::NotFound:
		SPDLOG_LOGGER_DEBUG(logger, "Not Found");
		*result = nullptr;
		return nss_status::NSS_STATUS_NOTFOUND;
	default:
		SPDLOG_LOGGER_ERROR(logger, "Other Error");
		SPDLOG_LOGGER_ERROR(logger, "Error {}", promise.getErrcode());
		*result = nullptr;
		return nss_status::NSS_STATUS_UNAVAIL;
	}
}
}