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

#include <csignal>
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

#include <sys/stat.h>

namespace fs = std::filesystem;

static void initLogger() {
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto basic_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/var/log/gitlabnss.log");
	std::vector<spdlog::sink_ptr> sinks{console_sink, basic_sink};
	auto logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
	logger->flush_on(spdlog::level::info);
	spdlog::set_default_logger(logger);
}

class GitLabDaemonImpl final : public GitLabDaemon::Server {
private:
	Config config;
	gitlab::GitLab gitlab;

public:
	GitLabDaemonImpl(Config config) : config(config), gitlab(this->config) {}

	virtual ::kj::Promise<void> getUserByID(GetUserByIDContext context) override {
		spdlog::info("getUserByID({})", context.getParams().getId());
		gitlab::User user;
		Error err;
		if ((err = gitlab.fetchUserByID(context.getParams().getId(), user)) == Error::Ok) {
			spdlog::debug("Found");
			auto output = context.getResults().initUser();
			output.setId(user.id);
			output.setName(user.name);
			output.setUsername(user.username);
		}
		context.getResults().setErrcode(static_cast<uint32_t>(err));
		return kj::READY_NOW;
	}
	virtual ::kj::Promise<void> getUserByName(GetUserByNameContext context) override {
		spdlog::info("getUserByName({})", context.getParams().getName().cStr());
		gitlab::User user;
		Error err;
		if ((err = gitlab.fetchUserByUsername(context.getParams().getName().cStr(), user)) == Error::Ok) {
			spdlog::debug("Found");
			auto output = context.getResults().initUser();
			output.setId(user.id);
			output.setName(user.name);
			output.setUsername(user.username);
		}
		context.getResults().setErrcode(static_cast<uint32_t>(err));
		return kj::READY_NOW;
	}

	virtual ::kj::Promise<void> getSSHKeys(GetSSHKeysContext context) {
		spdlog::info("getSSHKeys({})", context.getParams().getId());
		std::vector<std::string> keys;
		Error err;
		if ((err = gitlab.fetchAuthorizedKeys(context.getParams().getId(), keys)) == Error::Ok) {
			spdlog::debug("Found");
			// When std::ranges::to is finally implemented by GCC:
			// std::string joined = keys | std::views::join | std::ranges::to<std::string>();
			std::string joined;
			for (auto&& key : keys)
				joined += key;

			context.getResults().setKeys(joined);
		}
		context.getResults().setErrcode(static_cast<uint32_t>(err));
		return kj::READY_NOW;
	}
};

static auto [promise, fulfiller] = kj::newPromiseAndFulfiller<void>();
int main(int argc, char* argv[]) {
	auto configPath = fs::current_path().root_path() / "etc" / "gitlabnss" / "gitlabnss.conf";
	initLogger();
	spdlog::info("Starting the GitLab NSS daemon...");
	spdlog::info("Reading config from {}", configPath.string());
	auto config = Config::fromFile(configPath);
	auto socketPath = config.general.socketPath;
	spdlog::info("Success! Will use {} to communicate with GitLab", config.gitlabapi.baseUrl);
	spdlog::info("Binding socket to {}", socketPath.string());
	capnp::Capability::Client heap{kj::heap<GitLabDaemonImpl>(config)};
	auto addr = std::format("unix:{}", socketPath.string());
	kj::StringPtr bind = addr.c_str();
	capnp::EzRpcServer server{heap, bind};
	auto& waitScope = server.getWaitScope();

	waitScope.poll(); /* Poll once to create the socket file. */
	spdlog::info("Setting socket permissions for {} to 0o{:o}", socketPath.c_str(), config.general.socketPerms);
	if (chmod(socketPath.c_str(), static_cast<mode_t>(config.general.socketPerms)) != 0)
		spdlog::warn("Failed to change permissions with errno {}", errno);

	spdlog::info("Instantiating SIGINT handler");
	std::signal(SIGINT, +[](int signal) { fulfiller->fulfill(); });

	// Run until SIGINT is signaled; accept connections and handle requests.
	spdlog::info("Listening...");
	promise.wait(waitScope);

	// A shame that EzRpcServer does not clean up after itself :(
	unlink(socketPath.string().c_str());
	spdlog::info("Good bye!");
	return 0;
}