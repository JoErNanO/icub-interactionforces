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
 * This program is distributed #include the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */



#include "FingerForceModule.h"

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>

YARP_DECLARE_DEVICES(icubmod);



int main(int argc, char *argv[]) {
    using yarp::os::Network;
    using yarp::os::ResourceFinder;
    using iCub::interactionForces::FingerForceModule;

    // Initialise device driver list
    YARP_REGISTER_DEVICES(icubmod);

    // Open network
    Network yarp;
    if (!yarp.checkNetwork()) {
         fprintf(stdout, "Error: yarp server is not available.\n");
         return -1;
    }     

    // Using modules
    FingerForceModule mod;

    // Create resource finder
    ResourceFinder rf;
    rf.setVerbose();
    rf.setDefaultConfigFile("confFingertipsRight.ini");
    rf.setDefaultContext("fingerForce");
    rf.configure("ICUB_ROOT", argc, argv);

    // Configure and run module
    mod.configure(rf);
    mod.runModule();

    return 0;
}
