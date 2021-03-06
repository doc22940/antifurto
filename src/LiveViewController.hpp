#pragma once

#include <future>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace antifurto {

// fwd reference
class Configuration;
class Picture;
class LiveView;

/// This class is responsible in starting and stopping LiveView in a
/// gracefully way
class LiveViewController
{
public:
    /// This callback is called with a parameter 'true' when a registration
    /// is needed, 'false' when it is no more needed
    using PictureRegistrationRequest = std::function<void(bool)>;

    LiveViewController(const Configuration& c, PictureRegistrationRequest regF);
    ~LiveViewController();

    void addPicture(const Picture& p);
    void start();
    void stop();

private:
    enum class Status {
        STARTED,
        STOPPING,
        STOPPED
    };

    PictureRegistrationRequest regFunc_;
    std::string socketPath_;
    std::chrono::seconds timeout_;
    std::unique_ptr<LiveView> liveView_;
    std::chrono::system_clock::time_point lastPictureWrittenTime_;
    Status status_ = Status::STOPPED;
    std::future<void> stopWork_;
};

} // namespace antifurto
