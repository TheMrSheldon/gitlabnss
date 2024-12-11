/**
 * @file gitlabnssd.cpp
 * @brief The gitlabnss daemon executable
 */

#include <config.hpp>
#include <gitlabapi.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <capnp/ez-rpc.h>
#include <protocol/messages.capnp.h>

#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>

namespace fs = std::filesystem;

static void initLogger() {
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto basic_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/var/log/gitlabnss.log");
	std::vector<spdlog::sink_ptr> sinks{console_sink, basic_sink};
	auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
	logger->flush_on(spdlog::level::info);
	spdlog::set_default_logger(logger);
}

class UnixListenSocket final {
private:
	int sockfd;

public:
	UnixListenSocket(const std::filesystem::path& path) : sockfd(socket(AF_UNIX, SOCK_STREAM, 0)) {
		if (sockfd == -1)
			return;
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		/** GCC does not have strcpy_s :( **/
		/** \todo bounds checking **/
		std::strcpy(addr.sun_path, path.c_str());
		if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
			close();
			return;
		}
	}
	~UnixListenSocket() { close(); }

	void close() {
		::close(sockfd);
		sockfd = -1;
	}

	operator bool() const noexcept { return sockfd != -1; }
};

class UnixSocket final {
private:
	int sockfd;

public:
	UnixSocket(const std::filesystem::path& path) : sockfd(socket(AF_UNIX, SOCK_STREAM, 0)) {
		if (sockfd == -1)
			return;
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		/** GCC does not have strcpy_s :( **/
		/** \todo bounds checking **/
		std::strcpy(addr.sun_path, path.c_str());
		if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
			close();
			return;
		}
	}
	~UnixSocket() { close(); }

	void close() {
		::close(sockfd);
		sockfd = -1;
	}

	operator bool() const noexcept { return sockfd != -1; }
};

static auto toUser(const gitlab::User& user) {
	::capnp::MallocMessageBuilder message;
	auto builder = message.initRoot<User>();
	builder.setId(user.id);
	builder.setName(user.name);
	builder.setUsername(user.username);
	return builder;
}

class GitLabDaemonImpl final : public GitLabDaemon::Server {
private:
	Config config;
	gitlab::GitLab gitlab;

public:
	GitLabDaemonImpl(Config config) : config(config), gitlab(config) {}

	virtual ::kj::Promise<void> getUserByID(GetUserByIDContext context) override {
		if (gitlab::User user; gitlab.fetchUserByID(context.getParams().getId(), user) == gitlab::Error::Ok) {
			auto output = context.getResults().initUser();
			output.setId(user.id);
			output.setName(user.name);
			output.setUsername(user.username);
		}
		return kj::READY_NOW;
	}
	virtual ::kj::Promise<void> getUserByName(GetUserByNameContext context) override {
		if (gitlab::User user;
			gitlab.fetchUserByUsername(context.getParams().getName().cStr(), user) == gitlab::Error::Ok) {
			auto output = context.getResults().initUser();
			output.setId(user.id);
			output.setName(user.name);
			output.setUsername(user.username);
		}
		return kj::READY_NOW;
	}
};

int main(int argc, char* argv[]) {
	auto configPath = fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf";
	auto socketPath = fs::current_path().root_path() / "var" / "run" / "gitlabnss.sock";
	initLogger();
	spdlog::info("Starting the GitLab NSS daemon...");
	spdlog::info("Reading config from {}", configPath.string());
	auto config = Config::fromFile(fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf");
	spdlog::info("Success! Will use {} to communicate with GitLab", config.gitlabapi.baseUrl);
	spdlog::info("Binding socket to {}", socketPath.string());
	/*UnixListenSocket socket{socketPath};
	if (!socket) {
		spdlog::critical("Failed to create socket");
		return 1;
	}*/
	capnp::Capability::Client heap{kj::heap<GitLabDaemonImpl>(config)};
	auto addr = std::format("unix:{}", socketPath.string());
	kj::StringPtr bind = addr.c_str();
	capnp::EzRpcServer server{heap, bind};
	auto& waitScope = server.getWaitScope();

	// Run forever, accepting connections and handling requests.
	kj::NEVER_DONE.wait(waitScope);
	return 0;
}