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

using yarp::os::Network;
using yarp::os::Property;
using yarp::os::ResourceFinder;
using yarp::os::Value;
using yarp::os::Bottle;
using yarp::sig::Vector;

/* *********************************************************************************************************************** */
/* ******* Constructor                                                      ********************************************** */   
FingerForceModule::FingerForceModule() 
    : RFModule(), fingerForce_IDLServer() {
    dbgTag = "FingerForceModule: ";

    closing = false;
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
bool FingerForceModule::configure(ResourceFinder &rf){
    cout << dbgTag << "Starting.";

    /* ****** Configure the Module                            ****** */
    // Get resource finder and extract properties
    // Module properties
    moduleName = rf.check("name", Value("fingertips"), "The module name.").asString().c_str();
    period = rf.check("period", 1.0, "The module period").asDouble();
    robotName = rf.check("robot", Value("icub"), "The robot name.").asString().c_str();
    whichArm = rf.check("whichArm", Value("right"), "The arm to use.").asString().c_str();
    string portNameRoot = "/" + moduleName + "/";

    // Experiment configuration
    Bottle parGroup = rf.findGroup("experiment");
    if (!parGroup.isNull()) {
        nPinches = parGroup.check("nPinches", 10, "Number of pinchings per sequence.").asInt();
        pinchIncrement = parGroup.check("pinchIncrement", 1, "Position increment for each pinch.").asInt();
        pinchDuration = parGroup.check("pinchDuration", 5, "Duration of a single pinch.").asInt();
        pinchDelay = parGroup.check("pinchDelay", 5, "Delay between pinches.").asInt();
        progressiveDepth = parGroup.check("progressiveDepth", false, "Set to true to progressively increase pinching depth.").asBool();
    } else {
        nPinches = 10;
        pinchIncrement = 1;
        pinchDuration = 5;
        pinchDelay = 5;
        progressiveDepth = false;
    }
    
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
    clientPos.view(iEnc);
    if (iEnc == 0) {
        return false;
    }
    int jnts = 0;
    iPos->getAxes(&jnts);
    std::vector<double> refAccels(jnts, 10e6);
    iPos->setRefAccelerations(&refAccels[0]);


    // Put arm in position
    reachArm();
    
    cout << dbgTag << "Started correctly.";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Close module                                                     ********************************************** */   
bool FingerForceModule::close(){
    // Close the module
    cout << dbgTag << "Closing.";

    // Close ports
    skinManagerHandL.close();
    skinManagerHandR.close();
    RPCFingertipsCmd.close();

    // Cartesian controller
    clientPos.close();

    cout << dbgTag << "Closed. Goodbye.";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Interrupt module                                                 ********************************************** */   
bool FingerForceModule::interruptModule() {
    // Interrupt the module
    cout << dbgTag << "Interrupting.";

    // Interrupt ports
    skinManagerHandL.interrupt();
    skinManagerHandR.interrupt();
    RPCFingertipsCmd.interrupt();

    cout << dbgTag << "Interrupted.";

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Place arm in grasping position                                   ********************************************** */ 
bool FingerForceModule::reachArm(void) {
    cout << dbgTag << "Reaching for pinch ... \t";
    
    iPos->stop();

    // Set the arm in the starting position
    // Arm
    iPos->positionMove(0 ,-25);
    iPos->positionMove(1 , 35);
    iPos->positionMove(2 , 18);
    iPos->positionMove(3 , 65);
    iPos->positionMove(4 ,-32);
    iPos->positionMove(5 , 9);
    iPos->positionMove(6 , -5);
    iPos->positionMove(7 , 20);
    // Hand
    iPos->positionMove(8 , 90);
    iPos->positionMove(9 , 30);
    iPos->positionMove(10, 30);
    iPos->positionMove(11, 5);
    iPos->positionMove(12, 0);
    iPos->positionMove(13, 15);
    iPos->positionMove(14, 0);
    iPos->positionMove(15, 40);

    // Check motion done
    waitMoveDone(10, 1);
    cout << "Done. \n";

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
    iEnc->getEncoders(position.data());

#ifndef NODEBUG
    cout << dbgTag << "Hand joint position is: \t";
    for (size_t i = 0; i < position.size(); ++i) {
        cout << position[i] << " ";
    }
    cout << "\n";
#endif

    // Close hand
    position[7] = 60; 
    position[8] = 40;
    position[9] = 20;
    position[10] = 180;
    for (int i = 11; i < 15; ++i) {
        if (i == finger.joint) {
            position[i] = finger.startPos;
        } else {
            position[i] = 0;
        }
    } 
    position[15] = 0;

    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);

#ifndef NODEBUG
    cout << "Finger position reached: (" << position.toString() << ")";
#endif

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Execute a pinching.                                               ********************************************** */
bool FingerForceModule::pinch(void) {
    using yarp::os::Time;

    // Get current limb position
    Vector position;
    int njoints;
    iPos->getAxes(&njoints);
    iEnc->getEncoders(position.data());

    // Check for progressive pinching depth
    if (progressiveDepth) {
        if (pinchCounter == 0) {
            previousDepth = position[finger.joint];        // Store previous depth
        } else if ((pinchCounter > 0) && (pinchCounter < nPinches/2)) {
            // First half of pinching sequence 
            position[finger.joint] = previousDepth + 1;      // Increment depth by 1' wrt previous depth
            previousDepth = position[finger.joint];        // Store previous depth
        } else if (pinchCounter == nPinches/2) {
            // Midpoint of sequence
            position[finger.joint] = previousDepth;
        } else if ((pinchCounter >= nPinches/2) && (pinchCounter < nPinches-1)) {
            // Second half of pinching sequence
            position[finger.joint] = previousDepth - 1;      // Decrement depth by 1' wrt previous depth
            previousDepth = position[finger.joint];        // Store previous depth
        }

        pinchCounter++;       // Increment pinchcounter
    }
    
    // Move
    cout << dbgTag << "Pinching ... \n ";
    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);
  
    // 1s pinch
    Time::delay(pinchDuration);

    cout << dbgTag << "Limb position reached: " << position[finger.joint];
    
    
    // Raise -- move back to pre-pinching position
    cout << dbgTag << "Raising. ";
    position[finger.joint] = finger.startPos;       // Move finger finger

    // Move
    iPos->positionMove(position.data());
    // Check motion done
    waitMoveDone(10, 1);

    // Build reply
    cout << dbgTag << "Limb position reached: " << position[finger.joint];
   
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

    cout << dbgTag << "Done. \n";

    disconnectDataDumper();

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

    return ok;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Connect the data dumper.                                         ********************************************** */
bool FingerForceModule::connectDataDumper(void) {
    bool ok = true;
    
    // Connect data dumper
    ok &= Network::connect("/" + robotName + "/" + whichArm + "_arm/state:o", "/dump_" + whichArm + "_pos");
    ok &= Network::connect("/NIDAQmxReader/data/real:o", "/dump_" + whichArm + "_nano17");
    ok &= Network::connect("/" + robotName + "/" + whichArm + "_arm/analog:o", "/dump_" + whichArm + "_ft");
    ok &= Network::connect("/wholeBodyDynamics/" + whichArm + "_arm/cartesianEndEffectorWrench:o", "/dump_" + whichArm + "_wbd");
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
    ok &= Network::disconnect("/" + robotName + "/" + whichArm + "_arm/analog:o", "/dump_" + whichArm + "_ft");
    ok &= Network::disconnect("/wholeBodyDynamics/" + whichArm + "_arm/cartesianEndEffectorWrench:o", "/dump_" + whichArm + "_wbd");
    ok &= Network::disconnect("/" + robotName + "/skin/" + whichArm + "_hand", "/dump_" + whichArm + "_skin_raw");
    ok &= Network::disconnect("/" + robotName + "/skin/" + whichArm + "_hand_comp", "/dump_" + whichArm + "_skin_comp");

    return ok;
}
/* *********************************************************************************************************************** */