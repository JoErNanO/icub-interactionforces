/* 
 * Copyright (C) 2013 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia
 * Authors: Francesco Giovannini
 * email:   francesco.giovannini@iit.it
 * website: www.robotcub.org 
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */



#include "FingerForceModule.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/os/Vocab.h>
#include <yarp/os/Time.h>


using iCub::interactionForces::FingerForceModule;

using std::stringstream;
using std::string;
using std::cout;
using std::cerr;

using yarp::os::Network;
using yarp::os::Property;
using yarp::os::ResourceFinder;
using yarp::os::Value;
using yarp::os::Bottle;
using yarp::sig::Vector;


#define FINGERFORCE_DEBUG 1

/* *********************************************************************************************************************** */
/* ******* Constructor                                                      ********************************************** */   
FingerForceModule::FingerForceModule() 
    : RFModule(), fingerForce_IDLServer() {
    dbgTag = "FingerForceModule: ";

    closing = false;
    
    // Experiment parameters
    pinchCounter = 0;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Destructor                                                       ********************************************** */   
FingerForceModule::~FingerForceModule() {}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Get Period                                                       ********************************************** */   
double FingerForceModule::getPeriod() { return period; }
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Update    module                                                 ********************************************** */   
bool FingerForceModule::updateModule() { return true; }
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Configure module                                                 ********************************************** */   
bool FingerForceModule::configure(ResourceFinder &rf) {
    using std::vector;
    using yarp::os::Time;

    cout << dbgTag << "Starting. \n";

    /* ****** Configure the Module                            ****** */
    // Get resource finder and extract properties
    // Module properties
    moduleName = rf.check("name", Value("fingertips"), "The module name.").asString().c_str();
    period = rf.check("period", 1.0, "The module period").asDouble();
    robotName = rf.check("robot", Value("icub"), "The robot name.").asString().c_str();
    whichArm = rf.check("whichArm", Value("right"), "The arm to use.").asString().c_str();
    string portNameRoot = "/" + moduleName + "/";

    // Robot home position
    Bottle parGroup = rf.findGroup("home");
    homePos.resize(16, 0.0);
    if (!parGroup.isNull()) {
        // Arm position
        Bottle *armPos = parGroup.find("arm").asList();
        if (armPos != NULL) {
            if (armPos->size() == 7) {
                for (int i = 0; i < armPos->size(); ++i) {
                    homePos[i] = armPos->get(i).asDouble();
                }
            } else {
                cerr << dbgTag << "Invalid arm position parameter size. Expecting a list of size 7. \n";
                return false;
            }
        } else {
            cerr << dbgTag << "Cannot find arm home position (arm) in parameter group [home] in the specified configuration file. \n";
            return false;
        }

        // Hand position
        Bottle *handPos = parGroup.find("hand").asList();
        if (handPos != NULL) {
            if (handPos->size() == 9) {
                for (int i = 0; i < handPos->size(); ++i) {
                    homePos[i + 7] = handPos->get(i).asDouble();
                }
            } else {
                cerr << dbgTag << "Invalid hand position parameter size. Expecting a list of size 9. \n";
                return false;
            }
        } else {
            cerr << dbgTag << "Cannot find hand home position (hand) in parameter group [home] in the specified configuration file. \n";
            return false;
        }
    } else {
        cerr << dbgTag << "Cannot find robot home position parameter group [home] in the specified configuration file. \n";
        return false;
    }

#ifndef NODEBUG
    cout << "\n";
    cout << "DEBUG: " << dbgTag << "Experiment home position for the arm is: ";
    for (size_t i = 0; i < homePos.size(); ++i) {
        cout << homePos[i] << " ";
    }
    cout << "\n";
#endif


    // Experiment configuration
    parGroup = rf.findGroup("experiment");
    if (!parGroup.isNull()) {
        nPinches = parGroup.check("nPinches", 10, "Number of pinchings per sequence.").asInt();
        pinchIncrement = parGroup.check("pinchIncrement", 1, "Position increment for each pinch.").asInt();
        pinchDuration = parGroup.check("pinchDuration", 5, "Duration of a single pinch.").asInt();
        pinchDelay = parGroup.check("pinchDelay", 5, "Delay between pinches.").asInt();
        progressiveDepth = parGroup.check("progressiveDepth", false, "Set to true to progressively increase pinching depth.").asBool();
        useThumb = parGroup.check("useThumb", false, "Set to true to use the thumb when pinching.").asBool();
    } else {
        nPinches = 10;
        pinchIncrement = 1;
        pinchDuration = 5;
        pinchDelay = 5;
        progressiveDepth = false;
        useThumb = false;
    }

#ifndef NODEBUG
    cout << "\n";
    cout << "DEBUG: " << dbgTag << "Experiment parameters are: \n";
    cout << "DEBUG: " << dbgTag << "\t" << "nPinches: " << nPinches << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "pinchIncrement " << pinchIncrement << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "pinchDuration " << pinchDuration << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "pinchDelay " << pinchDelay << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "progressiveDepth "  << std::boolalpha << progressiveDepth << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "useThumb " << useThumb << std::noboolalpha << "\n";
    cout << "\n";
#endif
    

    // Pinching parameters
    parGroup = rf.findGroup("finger");
    if (!parGroup.isNull()) {
        finger.joint = parGroup.check("joint", 11).asInt();
        finger.startPos = parGroup.check("startPos", 0.0).asDouble();
        finger.pinchPos = parGroup.check("pinchPos", 20.0).asDouble();
    } else {
        finger.joint = 11;
        finger.startPos = 0;
        finger.pinchPos = 20;
    }

    // Initialise previous depth
    previousDepth.resize(2, 0.0);
    previousDepth[0] = homePos[9];
    previousDepth[1] = finger.startPos;

#ifndef NODEBUG
    cout << "DEBUG: " << dbgTag << "Pinching parameters are: \n";
    cout << "DEBUG: " << dbgTag << "\t" << "finger.joint: " << finger.joint << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "finger.startPos " << finger.startPos << "\n";
    cout << "DEBUG: " << dbgTag << "\t" << "finger.pinchPos " << finger.pinchPos << "\n";
    cout << "\n";
#endif


    /* ****** Open ports                                      ****** */
    skinManagerHandL.open((portNameRoot + "handL/finger:i").c_str());
    skinManagerHandR.open((portNameRoot + "handR/finger:i").c_str());
    RPCFingertipsCmd.open((portNameRoot + "cmd:io").c_str());
    attach(RPCFingertipsCmd);
    
        
    /* ****** Position control stuff for hand                       ****** */
    Property options;
    options.put("device", "remote_controlboard");
    options.put("local", (portNameRoot + "position_client/" + whichArm + "_arm").c_str());               
    options.put("remote", ("/" + robotName + "/" + whichArm + "_arm").c_str());
    if (!clientPos.open(options)) {
        return false;
    }
    // Open the views
    clientPos.view(iPos);
    if (iPos == 0) {
        return false;
    }
    clientPos.view(iEncs);
    if (iEncs == 0) {
        return false;
    }
    int jnts = 0;
    iPos->getAxes(&jnts);
    // Set reference accelerations
    std::vector<double> refAccels(jnts, 10e6);
    iPos->setRefAccelerations(&refAccels[0]);
    // Set reference speeds
    std::vector<double> refSpeeds(jnts, 0);
    iPos->getRefSpeeds(&refSpeeds[0]);
    for (int i = 11; i < 15; ++i) {
        refSpeeds[i] = 50;
    }
    iPos->setRefSpeeds(&refSpeeds[0]);

    
    /* ******* Store position prior to acquiring control.           ******* */
    startPos.resize(jnts);
    bool ok = false;
    while(!ok) {
        ok = iEncs->getEncoders(startPos.data());
#ifndef NODEBUG
        cout << "DEBUG: " << dbgTag << "Encoder data is not available yet. \n";
#endif
        Time::delay(0.1);
    }

    // Put arm in position
    reachArm();


    /* ******* Start threads.                                       ******* */
    // Gaze thread
    thGaze = new GazeThread(100, rf);
    if (!thGaze->start()) {
        cout << dbgTag << "Could not start the gaze thread. \n";
        return false;
    }
    
    cout << dbgTag << "Started correctly. \n";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Close module                                                     ********************************************** */   
bool FingerForceModule::close() {
    // Close the module
    cout << dbgTag << "Closing. \n";

    // Close ports
    skinManagerHandL.close();
    skinManagerHandR.close();
    RPCFingertipsCmd.close();

    // Stop threads
    thGaze->stop();
    
    // Restore initial robot position
    if (iPos) {
        iPos->stop();
        // Restore initial robot position
        iPos->positionMove(startPos.data());
    }

    // Cartesian controller
    clientPos.close();

    cout << dbgTag << "Closed. \n";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Interrupt module                                                 ********************************************** */   
bool FingerForceModule::interruptModule() {
    // Interrupt the module
    cout << dbgTag << "Interrupting. \n";

    // Interrupt ports
    skinManagerHandL.interrupt();
    skinManagerHandR.interrupt();
    RPCFingertipsCmd.interrupt();

    cout << dbgTag << "Interrupted. \n";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Place arm in grasping position                                   ********************************************** */ 
bool FingerForceModule::reachArm(void) {
    cout << dbgTag << "Reaching for pinch ... \n";
    
    iPos->stop();

    // Set the arm in the starting position
    // Arm
    iPos->positionMove(&homePos[0]);
    // Hand
    open();

    // Check motion done
    waitMoveDone(10, 1);
    cout << dbgTag << "Done. \n";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Open the hand.                                                   ********************************************** */
bool FingerForceModule::open(void) {
    
    // Create position vector
    int joints;
    iPos->getAxes(&joints);
    Vector position(joints);
    iEncs->getEncoders(position.data());

#ifndef NODEBUG
    cout << "DEBUG: " << dbgTag << "Hand joint position is: \t";
    for (size_t i = 0; i < position.size(); ++i) {
        cout << position[i] << " ";
    }
    cout << "\n";
#endif

    // Open hand
    for (size_t i = 7; i < homePos.size(); ++i) {
        if ((int) i == finger.joint) {
            position[i] = finger.startPos;
        } else {
            position[i] = homePos[i];
        }
    }
    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);

#ifndef NODEBUG
    iEncs->getEncoders(position.data());
    cout << "DEBUG: " << dbgTag << "Hand joint position reached: \t";
    for (size_t i = 0; i < position.size(); ++i) {
        cout << position[i] << " ";
    }
    cout << "\n";
#endif

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Execute a pinching.                                               ********************************************** */
bool FingerForceModule::pinch(void) {
    using yarp::os::Time;

    // Get current limb position
    int njoints;
    iPos->getAxes(&njoints);
    Vector position(njoints);
    iEncs->getEncoders(position.data());

#if !defined(NODEBUG) || (FINGER_FORCE_DEBUG)
    cout << "DEBUG: " << dbgTag << "Starting limb position: " << position[finger.joint] << ", "
        << "Previous depth: ";
    cout << "Thumb (" << previousDepth[0] << ")\t Finger: (" << previousDepth[1] << ") \n";
#endif

    // Check for progressive pinching depth
    if (progressiveDepth) {
#if !defined(NODEBUG) || (FINGER_FORCE_DEBUG)
        cout << "DEBUG: " << dbgTag << "Performing pinch number: " << pinchCounter << "\n";
#endif
        if ((pinchCounter >= 0) && (pinchCounter < nPinches/2)) {
            // First half of pinching sequence 
            position[finger.joint] = previousDepth[1] + pinchIncrement;    // Increment depth wrt previous depth
            previousDepth[1] = position[finger.joint];                  // Store previous depth
            checkUseThumb(true, position);
        } else if (pinchCounter == nPinches/2) {
            // Midpoint of sequence
            position[finger.joint] = previousDepth[1];
        } else if ((pinchCounter >= nPinches/2) && (pinchCounter < nPinches)) {
            // Second half of pinching sequence
            position[finger.joint] = previousDepth[1] - pinchIncrement;    // Decrement depth wrt previous depth
            previousDepth[1] = position[finger.joint];                  // Store previous depth
            checkUseThumb(true, position);
        }

        pinchCounter++;       // Increment pinchcounter
    } else {
        position[finger.joint] = finger.startPos + pinchIncrement;      // Increment depth wrt previous depth
    }

    
    cout << dbgTag << "Pinching depth is: " << position[finger.joint] << "\n";

    // Pinch
    cout << dbgTag << "Pinching ...... ";
    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);
    iEncs->getEncoders(position.data());
    cout << "Limb position reached: " << position[finger.joint] << "\n";
    
    // dt pinch
    Time::delay(pinchDuration);

    // Raise -- move back to pre-pinching position
    cout << dbgTag << "Raising ...... ";
    position[finger.joint] = finger.startPos;       // Move finger
    if (useThumb) {
        position[9] = homePos[9];
    }

    // Move
    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);

    iEncs->getEncoders(position.data());
    cout << "Limb position reached: " << position[finger.joint] << "\n";
   
    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Execute a pinching.                                               ********************************************** */
bool FingerForceModule::pinchseq() {
    using yarp::os::Time;

    // Sequence of pinchings
    cout << dbgTag << "Executing a series of " << nPinches << " pinchings. \n";
 
    connectDataDumper();

    // Reset pinchcounter
    pinchCounter = 0;
    for (int i = 0; i < nPinches; ++i) {
        // Execute pinch
        pinch();
        
        // Wait between taps
        Time::delay(pinchDelay);
    }

    cout << dbgTag << "Pinching sequence complete. \n";

    disconnectDataDumper();

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Reset the pinch counter.                                         ********************************************** */
bool FingerForceModule::resetC(void) {
    cout << dbgTag << "Resetting the pinch counter. \n";
    pinchCounter = 0;
    
    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* RPC Quit module                                                  ********************************************** */
bool FingerForceModule::quit(void) {
        return closing = true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Attach RPC port.                                                 ********************************************** */
bool FingerForceModule::attach(yarp::os::RpcServer &source) {
        return this->yarp().attachAsServer(source);
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Wait for motion to be completed.                                 ********************************************** */
bool FingerForceModule::waitMoveDone(const double &i_timeout, const double &i_delay) {
    using yarp::os::Time;
    
    bool ok = false;
    
    double start = Time::now();
    while (!ok && (start - Time::now() <= i_timeout)) {
        iPos->checkMotionDone(&ok);
        Time::delay(i_delay);
    }

#ifndef NODEBUG
    if (!ok) {
        cout << dbgTag << "Timeout expired while waiting for motion to complete. \n";
    }
#endif

    return ok;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Check if the thumb is to be used for the pinching motion.        ********************************************** */
void FingerForceModule::checkUseThumb(const bool increment, yarp::sig::Vector &o_positions) {
    // Check for thumb
    if (useThumb) {
        if (increment) {
            o_positions[9] = previousDepth[0] + pinchIncrement;
        } else {
            o_positions[9] = previousDepth[0] - pinchIncrement;
        }

        // Store previous depth
        previousDepth[0] = o_positions[9];
    }
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Connect the data dumper.                                         ********************************************** */
bool FingerForceModule::connectDataDumper(void) {
    bool ok = true;
    
    // Connect data dumper
    ok &= Network::connect("/" + robotName + "/" + whichArm + "_arm/state:o", "/dump_" + whichArm + "_pos");
    ok &= Network::connect("/NIDAQmxReader/data/real:o", "/dump_" + whichArm + "_nano17");
    ok &= Network::connect("/" + robotName + "/skin/" + whichArm + "_hand", "/dump_" + whichArm + "_skin_raw");
    ok &= Network::connect("/" + robotName + "/skin/" + whichArm + "_hand_comp", "/dump_" + whichArm + "_skin_comp");

    return ok;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Disconnect the data dumper.                                      ********************************************** */
bool FingerForceModule::disconnectDataDumper(void) {
    bool ok = true;

    // Disconnect data dumper
    ok &= Network::disconnect("/" + robotName + "/" + whichArm + "_arm/state:o", "/dump_" + whichArm + "_pos");
    ok &= Network::disconnect("/NIDAQmxReader/data/real:o", "/dump_" + whichArm + "_nano17");
    ok &= Network::disconnect("/" + robotName + "/skin/" + whichArm + "_hand", "/dump_" + whichArm + "_skin_raw");
    ok &= Network::disconnect("/" + robotName + "/skin/" + whichArm + "_hand_comp", "/dump_" + whichArm + "_skin_comp");

    return ok;
}
/* *********************************************************************************************************************** */
