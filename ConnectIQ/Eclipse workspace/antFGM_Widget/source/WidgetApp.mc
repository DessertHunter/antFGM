//!
//! Copyright 2016 DessertHunter
//!
using Toybox.Application as App;
using Toybox.System as Sys;


class WidgetApp extends App.AppBase {

    var mSensor; // class ANT_FGM_Sensor

    function initialize() {
        AppBase.initialize();
    }

    //! onStart() is called on application start up
    function onStart(state) {
        try
        {
            // Create the sensor object and open it
            mSensor = new ANT_FGM_Sensor();
            mSensor.open();
        }
        catch(ex instanceof Ant.UnableToAcquireChannelException)
        {
            Sys.println( ex.getErrorMessage() );
            ex.printStackTrace();
            mSensor = null;
        }
    }

    //! onStop() is called when your application is exiting
    function onStop(state) {
        if (null != mSensor)
        {
            mSensor.release(); // Release the generic ANT Channel back to the system. If it is open it will be automatically closed.
        }
    }

    //! Return the initial view of your application here
    function getInitialView() {
        return [ new WidgetMainView(mSensor), new WidgetBehavior(mSensor) ];
    }

}