// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#ifndef YARP_THRIFT_GENERATOR_fingerForce_IDLServer
#define YARP_THRIFT_GENERATOR_fingerForce_IDLServer

#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>

class fingerForce_IDLServer;


/**
 * fingerForce_IDLServer
 * IDL Interface to \ref fingerForce services.
 */
class fingerForce_IDLServer : public yarp::os::Wire {
public:
  fingerForce_IDLServer() { yarp().setOwner(*this); }
/**
 * Opens the robot hand.
 * @return true/false on success/failure
 */
  virtual bool open();
/**
 * Grasp an object using a pinch grasp.
 * The grasping movement is controlled in position mode.
 * @return true/false on success/failure
 */
  virtual bool pinch();
/**
 * Perform a sequence of pinch grasps.
 * @return true/false on success/failure
 */
  virtual bool pinchseq();
/**
 * Reset the pinch counter.
 * @return true/false on success/failure
 */
  virtual bool resetC();
/**
 * Quit the module.
 * @return true/false on success/failure
 */
  virtual bool quit();
  virtual bool read(yarp::os::ConnectionReader& connection);
  virtual std::vector<std::string> help(const std::string& functionName="--all");
};

#endif

