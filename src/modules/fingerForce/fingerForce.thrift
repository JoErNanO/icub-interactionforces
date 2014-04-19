#fingerForce.thrift


/**
 * fingerForce_IDLServer
 *
 * IDL Interface to \ref fingerForce services.
 */
service fingerForce_IDLServer
{
    /**
     * Opens the robot hand.
     * @return true/false on success/failure
     */
    bool open();

    /**
     * Grasp an object using a pinch grasp.
     * The grasping movement is controlled in position mode.
     * @return true/false on success/failure
     */
    bool pinch();

    /**
     * Perform a sequence of pinch grasps.
     * @return true/false on success/failure
     */
    bool pinchseq();

    /**
     * Reset the pinch counter.
     * @return true/false on success/failure
     */
    bool resetC();
    
    /**
     * Quit the module.
     * @return true/false on success/failure
     */
    bool quit();
}
