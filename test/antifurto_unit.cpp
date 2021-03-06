#include <stdexcept>
#include <cassert>
#include <functional>
#include <thread>
#include <fstream>
#include <future>

#include "CvCamera.hpp"
#include "MotionDetector.hpp"
#include "PictureArchive.hpp"
#include "RecordingController.hpp"
#include "Config.hpp"
#include "CameraController.hpp"
#include "LiveView.hpp"
#include "concurrency/TaskScheduler.hpp"
#include "concurrency/TimeUtility.hpp"
#include "text/ToString.hpp"
#include "ipc/NamedPipe.hpp"
#include "meta/Observer.hpp"


#define BOOST_TEST_MODULE unit
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using namespace antifurto;
namespace bfs = boost::filesystem;


std::size_t refcount(cv::Mat const& m) {
  return m.u ? (m.u->refcount) : 0;
}

struct test_error : public std::runtime_error
{ using std::runtime_error::runtime_error; };

cv::Mat makeBlackImg()
{
    return { cv::Mat::zeros(10, 10, CV_8U) };
}

cv::Mat makeWhiteImg()
{
    cv::Mat result = makeBlackImg();
    cv::bitwise_not(result, result);
    return result;
}


BOOST_AUTO_TEST_CASE(cameraTakePicture)
{
    Picture p;
    CvCamera camera;
    camera.takePicture(p);
}

BOOST_AUTO_TEST_CASE(pictureRefcount)
{
    Picture z { makeBlackImg() };
    Picture o { makeWhiteImg() };

    auto checkRefcount = [](cv::Mat const& m) { BOOST_CHECK_EQUAL(refcount(m), 1); };
    auto checkEqual = [](cv::Mat const& a, cv::Mat const& b) {
        BOOST_CHECK(std::equal(a.begin<uchar>(), a.end<uchar>(), b.begin<uchar>()));
    };
    auto checkNotEqual = [](cv::Mat const& a, cv::Mat const& b) {
        BOOST_CHECK(!std::equal(a.begin<uchar>(), a.end<uchar>(), b.begin<uchar>()));
    };

    // test value semantics
    Picture def;
    Picture r1 = z;
    Picture r2 = z;

    checkRefcount(z);
    checkRefcount(r1);
    checkRefcount(r2);

    checkEqual(r1, r2);
    checkEqual(z, r1);

    checkNotEqual(z, o);
    checkNotEqual(z, o);
    checkNotEqual(r1, o);

    r2 = o;
    checkEqual(r2, o);
    checkNotEqual(r2, z);

    checkRefcount(o);
}


struct MotionDetectorChecker
{
    MotionDetector::State expected;
    bool called = false;

    void setExpected(MotionDetector::State newExpected)
    {
        called = false;
        expected = newExpected;
    }

    void onStateChange(MotionDetector::State s)
    {
        BOOST_CHECK_EQUAL(s, expected);
        called = true;
    }
};

BOOST_AUTO_TEST_CASE(motionDetection)
{
    using std::placeholders::_1;
    using State = MotionDetector::State;

    Picture w { makeWhiteImg() };
    Picture b { makeBlackImg() };
    MotionDetector detector(3, 4);
    MotionDetectorChecker checker;
    detector.addObserver(std::bind(&MotionDetectorChecker::onStateChange, &checker, _1));

    for (uchar a: w)
        BOOST_CHECK_EQUAL(a, 255);
    for (uchar a: b)
        BOOST_CHECK_EQUAL(a, 0);

    // initialize
    for (int i = 0; i < 5; ++i)
        detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, false);

    // pre alarm after one different frame
    checker.setExpected(State::PRE_ALARM);
    detector.examinePicture(b); // the first difference is discarded by the algorithm
    BOOST_CHECK_EQUAL(checker.called, false);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, true);

    // no alarm after one not different
    checker.setExpected(State::NO_ALARM);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, true);

    // pre alarm after one other different frame
    checker.setExpected(State::PRE_ALARM);
    detector.examinePicture(b);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, true);

    // alarm after 3 different frames
    checker.setExpected(State::ALARM);
    detector.examinePicture(b);
    BOOST_CHECK_EQUAL(checker.called, false);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, true);

    // no alarm after 4 equal frames
    checker.setExpected(State::NO_ALARM);
    detector.examinePicture(w);
    detector.examinePicture(w);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, false);
    detector.examinePicture(w);
    BOOST_CHECK_EQUAL(checker.called, true);
}

BOOST_AUTO_TEST_CASE(pictureArchive)
{
    PictureArchive archive("/tmp", 3);
    CvCamera camera;
    Picture p;
    for (int i = 0; i < 5; ++i)
    {
        camera.takePicture(p);
        archive.addPicture(p);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    archive.startSaving();
    camera.takePicture(p);
    archive.addPicture(p);
    archive.stopSaving();
}

BOOST_AUTO_TEST_CASE(recordingController)
{
    MotionDetector detector;
    Configuration cfg;
    concurrency::TaskScheduler scheduler;
    RecordingController controller{ cfg, detector, scheduler };
}

BOOST_AUTO_TEST_CASE(taskScheduler)
{
    using namespace std::chrono;

    int counter = 0;
    {
        concurrency::TaskScheduler sched;
        sched.scheduleEvery(seconds(1), [&]{ std::cout << "tick\n"; ++counter; });
        std::this_thread::sleep_for(seconds(5) + milliseconds(10));
    }
    BOOST_CHECK_EQUAL(counter, 5);

    counter = 0;
    {
        concurrency::TaskScheduler sched;
        sched.scheduleEvery(seconds(1), [&]{
            std::this_thread::sleep_for(seconds(1) + milliseconds(500));
            std::cout << "tick\n";
            ++counter; });
        std::this_thread::sleep_for(seconds(3) + milliseconds(10));
    }
    BOOST_CHECK_EQUAL(counter, 2);

    counter = 0;
    {
        concurrency::TaskScheduler sched;
        sched.scheduleAfter(seconds(1), [&]{
            counter = 1;
        });
        std::this_thread::sleep_for(seconds(1) - milliseconds(30));
        BOOST_CHECK_EQUAL(counter, 0);
        std::this_thread::sleep_for(milliseconds(35));
        BOOST_CHECK_EQUAL(counter, 1);
    }

    counter = 0;
    {
        concurrency::TaskScheduler sched;
        sched.scheduleAfter(seconds(2), [&]{
            std::cout << 2 << std::endl;
            counter = 2;
        });
        sched.scheduleAfter(seconds(1), [&]{
            std::cout << 1 << std::endl;
            counter = 1;
        });
        BOOST_CHECK_EQUAL(counter, 0);
        std::this_thread::sleep_for(seconds(1) + milliseconds(50));
        BOOST_CHECK_EQUAL(counter, 1);
        std::this_thread::sleep_for(seconds(1) + milliseconds(50));
        BOOST_CHECK_EQUAL(counter, 2);
    }
}

BOOST_AUTO_TEST_CASE(timeOps)
{
    std::cout << "Now: " << text::toString(std::chrono::system_clock::now())
              << "\nTomorrow: " << text::toString(concurrency::tomorrow())
              << std::endl;
}

std::string readFileContents(const char* name)
{
    std::ifstream f{name};
    std::string result;
    std::getline(f, result);
    return result;
}

void writeToFile(const char* name, const char* contents)
{
    std::ofstream f{name};
    f << contents << std::endl;
}

BOOST_AUTO_TEST_CASE(namedPipe)
{
    const char* filename = "/tmp/tmppipe";
    BOOST_CHECK(!bfs::exists(filename));
    {
        ipc::NamedPipe pipe{filename};
        BOOST_CHECK(bfs::exists(filename));
        auto res = std::async(std::launch::async, readFileContents, filename);
        writeToFile(filename, "try to write");
        BOOST_CHECK_EQUAL(res.get(), "try to write");
    }
    BOOST_CHECK(!bfs::exists(filename));
}

BOOST_AUTO_TEST_CASE(observer)
{
    meta::Subject<int, int> subject;
    int a = 0, b = 0;
    {
        auto reg = subject.registerObserver([&](int x, int y){ a = x; b = y; });
        subject.notify(5, 7);
        BOOST_CHECK_EQUAL(a, 5);
        BOOST_CHECK_EQUAL(b, 7);
    }
    subject.notify(1, 2);
    BOOST_CHECK_EQUAL(a, 5);
    BOOST_CHECK_EQUAL(b, 7);

    {
        decltype(subject)::Registration reg;
        int notifications = 0;

        reg = subject.registerObserver([&](int, int){
            ++notifications;
            BOOST_CHECK(subject.hasObservers());
            bool clearedNow = reg.clear();
            BOOST_CHECK(!clearedNow);
            // the registration is delayed
            BOOST_CHECK(subject.hasObservers());
        });

        subject.notify(1, 1);
        BOOST_CHECK_EQUAL(notifications, 1);
        // registration cleared here
        BOOST_CHECK(!subject.hasObservers());
        subject.notify(1, 1);
        BOOST_CHECK_EQUAL(notifications, 1);
    }
}

struct CameraObserver
{
    CameraObserver()
        : start(std::chrono::system_clock::now())
    { }

    void operator()(const Picture&) {
        using namespace std::chrono;
        called = true;
        auto now = system_clock::now();
        lastPeriod = duration_cast<milliseconds>(now - start);
        std::cout << "last period: " << lastPeriod.count() << std::endl;
        start = now;
    }

    std::chrono::system_clock::time_point start;
    std::chrono::milliseconds lastPeriod;
    bool called = false;
};

template <typename T>
bool checkDuration(T actual, T expected, T tolerance = T{30})
{
    return expected - tolerance < actual
            && actual < expected + tolerance;
}

BOOST_AUTO_TEST_CASE(cameraController)
{
    using Milli = std::chrono::milliseconds;
    Milli max_milli{std::numeric_limits<int>::max()};
    BOOST_CHECK(std::min(max_milli, Milli(100)) == Milli(100));

    CameraController controller;
    CameraObserver o1;
    auto reg = controller.addObserver(std::ref(o1), Milli(100));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    BOOST_CHECK(o1.called);
    BOOST_CHECK(checkDuration(o1.lastPeriod, Milli(100)));
}

// void liveViewTest(int nreads)
// {
// #define PREFIX "/tmp/testlive"
//     std::future<void> res;
//     const char* filenames[] {
//         PREFIX "0.jpg",
//         PREFIX "1.jpg",
//         PREFIX "2.jpg",
//     };
//     {
//         LiveView liveView{PREFIX, 3};
//         res = std::async(std::launch::async, [&] {
//             std::vector<uint8_t> img;
//             for (int i = 0; i < nreads; ++i)
//             {
//                 std::ifstream f{filenames[i % 3]};
//                 std::istream_iterator<uint8_t> it(f), end;
//                 img.assign(it, end);
//             }
//         });
//
//         Picture p{makeWhiteImg()};
//         for (int i = 0; i < 5; ++i) {
//             liveView.addPicture(p);
//         }
//     }
//     res.get();
// #undef PREFIX
// }
//
// BOOST_AUTO_TEST_CASE(liveView)
// {
//     liveViewTest(3);
//     liveViewTest(2);
//     liveViewTest(0);
// }
