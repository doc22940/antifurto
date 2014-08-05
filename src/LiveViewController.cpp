#include "LiveViewController.hpp"
#include "LiveView.hpp"
#include "Config.hpp"
#include "Picture.hpp"
#include "Log.hpp"

#include <fstream>
#include <iterator>

namespace antifurto {

LiveViewController::
LiveViewController(Configuration& c, PictureRegistrationRequest regF)
    : regFunc_(regF), timeout_(c.liveView.inactivityTimeout)
    , liveView_(new LiveView{c.liveView.filePrefix, c.liveView.numPictures})
{ }

LiveViewController::~LiveViewController()
{
    try {
        stop();
    }
    catch (...) { }
    stopWork_.get(); // wait the stop
}

void LiveViewController::addPicture(const Picture& p)
{
    using namespace std::chrono;

    if (liveView_->addPicture(p))
        lastPictureWrittenTime_ = system_clock::now();
    else if (system_clock::now() - lastPictureWrittenTime_ > timeout_)
        stop();
}

void LiveViewController::stop()
{
    if (!running_) return;
    running_ = false;

    LOG_INFO << "Stopping live view";
    stopWork_ = std::async(std::launch::async, [this] {
        regFunc_(false); // call this inside async to avoid deadlock
        liveView_.reset();
        LOG_INFO << "Live view stopped";
    });
}

} // namespace antifurto