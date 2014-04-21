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



/**
*
@ingroup icub_module
\defgroup icub_FingertipsModule FingerForceModule

The FingerForceModule is a

\section intro_sec Description
Description here...


\section lib_sec Libraries
YARP


\section parameters_sec Parameters
<b>Command-line Parameters</b> 
<b>Configuration File Parameters </b>
 

\section portsa_sec Ports Accessed


\section portsc_sec Ports Created
<b>Output ports </b>
<b>Input ports</b>


\section in_files_sec Input Data Files


\section out_data_sec Output Data Files

 
\section conf_file_sec Configuration Files


\section tested_os_sec Tested OS


\section example_sec Example Instantiation of the Module


\author Francesco Giovannini (francesco.giovannini@iit.it)

Copyright (C) 2013 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia

CopyPolicy: Released under the terms of the GNU GPL v2.0.

This file can be edited at .... .
**/

#ifndef __FINGERFORCE_MODULE_H__
#define __FINGERFORCE_MODULE_H__

#include "fingerForce_IDLServer.h"
#include "GazeThread.h"

#include <yarp/os/RFModule.h>
#include <yarp/sig/Vector.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RpcServer.h>
#include <yarp/os/RpcClient.h>
#include <yarp/dev/CartesianControl.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/PolyDriver.h>

#include <string>
#include <map>
#include <vector>


namespace iCub {
    namespace interactionForces {

        /**
         * The PinchingLimb is the limb used to complete the pinching action.
         * This is a single joint such as index distal/proximal, etc.
         */
        struct PinchingLimb {
            /**
             * The joint number.
             */
            int joint;

            /**
             * The joint starting position.
             */
            double startPos;

            /**
             * The joint pinching position.
             */
            double pinchPos;
        };


        class FingerForceModule : public yarp::os::RFModule, public fingerForce_IDLServer {
            private:
                /* ****** Module attributes                             ****** */
                double period;
                std::string moduleName; 
                
                std::string robotName;
                std::string whichArm;

                /** Robot position prior to running module. */
                yarp::sig::Vector startPos;

                /** Robot position to be reached when module starts. */
                std::vector<double> homePos;

                /** Module closing flag used by RPC::quit(). */
                bool closing;

                /* ****** Experiment parameters                         ****** */
                /**
                 * The finger used for the pinching action.
                 */
                PinchingLimb finger;
                
                /**
                 * The number of pinches in the sequence.
                 */ 
                int nPinches;

                /**
                 * The duration of each pinch in seconds.
                 */
                int pinchDuration;

                /**
                 * The time delay between each pinch.
                 */
                int pinchDelay;

                /**
                 * The depth (position) increment between each pinch.
                 */
                int pinchIncrement;

                /**
                 * Set to true if the experiment is a progressive depth pinch experiment.
                 */
                bool progressiveDepth;

                /* ******* Experiment execution                         ******* */
                /**
                 * The pinch sequence counter.
                 */
                int pinchCounter;

                /**
                 * The previous pinch depth.
                 */
                std::vector<double> previousDepth;

                /** 
                 * Set to true if the thumb is to be used in the pinching motion. 
                 */
                bool useThumb;


                /* *******  Threads                                 ******* */
                iCub::interactionForces::GazeThread *thGaze;

                
                /* ****** Ports                                      ****** */
                yarp::os::RpcServer RPCFingertipsCmd;
                yarp::os::BufferedPort<yarp::sig::Vector> skinManagerHandL;
                yarp::os::BufferedPort<yarp::sig::Vector> skinManagerHandR;

                /* ****** RPC commands                                  ****** */
                std::map<std::string, int> RPCCommands;

                
                /* ****** Position Controller                           ****** */
                yarp::dev::PolyDriver clientPos;
                yarp::dev::IPositionControl *iPos;
                yarp::dev::IEncoders *iEncs;
                
                /* ****** Debug Attributes                           ****** */
                std::string dbgTag;
            
            public:
                FingerForceModule();
                virtual ~FingerForceModule();
                virtual double getPeriod();
                virtual bool updateModule();
                virtual bool configure(yarp::os::ResourceFinder &rf);
                virtual bool interruptModule();
                virtual bool close();
                virtual bool attach(yarp::os::RpcServer &source);

            private:
                /**
                 * Put arm in experiment position.
                 */
                bool reachArm(void);
                bool waitMoveDone(const double &i_timeout, const double &i_delay);
                void checkUseThumb(const bool increment, yarp::sig::Vector &o_positions);
                
                bool connectDataDumper(void);
                bool disconnectDataDumper(void);

                // RPC Methods
                virtual bool open(void);
                virtual bool pinch(void);
                virtual bool pinchseq(void);
                virtual bool resetC(void);
                virtual bool quit(void);
        };
    }
}

#endif

