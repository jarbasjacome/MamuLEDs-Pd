/*
 * This code get Leap Motion information and send to MamuLEDs-Pd patch via tcp-ip, port 17780
 * It was based on this Miller Puckette's solution sent me by Katja Vetter:
 * https://lists.puredata.info/pipermail/pd-list/2019-03/124874.html
 */

#include "Leap.h"
#include <boost/asio.hpp>

using namespace std;
using namespace Leap;
using namespace boost::asio::ip;

class SampleListener : public Listener
{
public:

    SampleListener();

    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
    virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
    virtual void onDeviceChange(const Controller&);
    virtual void onServiceConnect(const Controller&);
    virtual void onServiceDisconnect(const Controller&);

private:
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket_;

    int framesCount = 0;

    void getHandInfo(Hand hand, std::stringstream& msgToPd);
};

const std::string fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const std::string boneNames[] = {"Metacarpal", "Proximal", "Middle", "Distal"};
const std::string stateNames[] = {"STATE_INVALID", "STATE_START", "STATE_UPDATE", "STATE_END"};

void SampleListener::onInit(const Controller& controller)
{
    std::cout << "Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller)
{
    std::cout << "Connected" << std::endl;

    controller.config().setBool("robust_mode_enabled", true);
    controller.config().setBool("avoid_poor_performance", true);
    controller.config().save();
}

void SampleListener::onDisconnect(const Controller& controller)
{
    // Note: not dispatched when running in a debugger.
    std::cout << "Disconnected" << std::endl;
}

void SampleListener::onExit(const Controller& controller)
{
    std::cout << "Exited" << std::endl;
}

void SampleListener::onFrame(const Controller& controller)
{
    // Get the most recent frame and report some basic information
    const Frame frame = controller.frame();

    HandList hands = frame.hands();

    if (hands.count() > 0)
    {
        std::stringstream msgToPd;

        msgToPd << " " << frame.hands().count();
        std::cout << "Num of hands: " << frame.hands().count() << std::endl;

        Hand leftMostHand = frame.hands().leftmost();

        std::stringstream leftMostHandInfo;

        getHandInfo(leftMostHand, leftMostHandInfo);

        Hand rightMostHand = frame.hands().rightmost();

        std::stringstream rightMostHandInfo;

        if (hands.count() > 1)
        {
            if (leftMostHand.isLeft())
            {
                getHandInfo(rightMostHand, rightMostHandInfo);
                msgToPd << leftMostHandInfo.str() << rightMostHandInfo.str();
            }
            else
            {
                getHandInfo(rightMostHand, rightMostHandInfo);
                msgToPd << rightMostHandInfo.str() << leftMostHandInfo.str();
            }
        }
        else
        {
            if (leftMostHand.isLeft())
            {
                msgToPd  << leftMostHandInfo.str() << " 555 0 0 0 0 0 0 0 0 0";
            }
            else
            {
                msgToPd << " 555 0 0 0 0 0 0 0 0 0" << leftMostHandInfo.str();
            }
        }

        msgToPd << ";";

        boost::system::error_code err;
        boost::asio::write(socket_, boost::asio::buffer(msgToPd.str()), err);
        if (err)
            std::cout << err.message() << std::endl;
    }
}

void SampleListener::getHandInfo(Hand hand, std::stringstream& handInfoToPd)
{
    const Vector normal = hand.palmNormal();
    const Vector direction = hand.direction();

    if (hand.isLeft())
        handInfoToPd << " 1";
    else
        handInfoToPd << " 0";

    //*
    // Calculate the hand's pitch, roll, and yaw angles
    std::cout << "Hand direction: " <<  "pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, " << std::endl;
    std::cout << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, " << std::endl;
    std::cout << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl;
    //*/

    FingerList fingers = hand.fingers();

    if (fingers.count() > 4)
    {
        handInfoToPd << " " << direction.pitch() * RAD_TO_DEG
                     << " " << normal.roll() * RAD_TO_DEG
                     << " " << direction.yaw() * RAD_TO_DEG;

        Vector thumbBone2Dir = fingers[0].bone(static_cast<Bone::Type>(3)).direction();
        Vector indexBone2Dir = fingers[1].bone(static_cast<Bone::Type>(2)).direction();
        Vector middleBone2Dir = fingers[2].bone(static_cast<Bone::Type>(2)).direction();

        float thumbZ = (Leap::PI/2 + thumbBone2Dir.yaw() - direction.yaw()) * RAD_TO_DEG;

        float indexX = (Leap::PI + direction.pitch() - indexBone2Dir.pitch()) * RAD_TO_DEG;

        float middleX = (Leap::PI + middleBone2Dir.pitch() - direction.pitch()) * RAD_TO_DEG;

        handInfoToPd << " " << thumbZ << " " << indexX << " " << middleX;

        handInfoToPd << " " << hand.wristPosition().x/100.0f
                     << " " << - hand.wristPosition().z/100.0f
                     << " " << - hand.wristPosition().y/100.0f;

        std::cout << "thumb: " << thumbZ << std::endl;
        std::cout << "index: " << indexX << std::endl;
        std::cout << "middle: " << middleX << std::endl;

    }
}

void SampleListener::onFocusGained(const Controller& controller)
{
    std::cout << "Focus Gained" << std::endl;
}

void SampleListener::onFocusLost(const Controller& controller)
{
    std::cout << "Focus Lost" << std::endl;
}

void SampleListener::onDeviceChange(const Controller& controller)
{
    std::cout << "Device Changed" << std::endl;
    const DeviceList devices = controller.devices();

    for (int i = 0; i < devices.count(); ++i)
    {
        std::cout << "id: " << devices[i].toString() << std::endl;
        std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
    }
}

void SampleListener::onServiceConnect(const Controller& controller)
{
    std::cout << "Service Connected" << std::endl;
}

void SampleListener::onServiceDisconnect(const Controller& controller)
{
    std::cout << "Service Disconnected" << std::endl;
}

SampleListener :: SampleListener () : socket_(this->io_service)
{
    try
    {
        tcp::resolver resolver(io_service);
        tcp::resolver::query query("localhost", "17780");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::asio::connect(socket_, endpoint_iterator);
    }

    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    std::cout << "\nSocket constructed." << std::endl;

}

int main(int argc, char** argv)
{
    // Create a sample listener and controller
    SampleListener listener;
    Controller controller;

    // Have the sample listener receive events from the controller
    controller.addListener(listener);

    if (argc > 1 && strcmp(argv[1], "--bg") == 0)
        controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

    // Keep this process running until Enter is pressed
    std::cout << "Press Enter to quit..." << std::endl;
    std::cin.get();

    // Remove the sample listener when done
    controller.removeListener(listener);

    return 0;
}
