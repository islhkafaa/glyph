#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include "persistence/engine.hpp"
#include "server/grpc_service.hpp"
#include "server/handler.hpp"
#include "server/http_service.hpp"

namespace {
std::atomic<bool> keep_running{true};

void signal_handler(int) { keep_running = false; }
}  // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::filesystem::path data_dir = "./data";
    const char* env_dir = std::getenv("GLYPH_DATA_DIR");
    if (env_dir) {
        data_dir = env_dir;
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data-dir" && i + 1 < argc) {
            data_dir = argv[i + 1];
            break;
        }
    }

    try {
        StorageEngine engine(data_dir);
        CommandHandler handler(engine);

        GlyphServiceImpl grpc_impl(handler);
        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
        builder.RegisterService(&grpc_impl);
        std::unique_ptr<grpc::Server> grpc_server(builder.BuildAndStart());

        HttpService http_service(handler, "0.0.0.0", 8080);
        http_service.start();

        while (keep_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        http_service.stop();
        if (grpc_server) {
            grpc_server->Shutdown();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
