//!
//! Copyright 2016 DessertHunter
//!
using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;
using Toybox.Test as Test;


class WidgetInfoView extends Ui.View {

    hidden var mSensor; // class ANT_FGM_Sensor

    //! Constructor
    function initialize(sensor) {
        View.initialize(); // You should always call the parent's initializer
        
        mSensor = sensor;
    }

    //! Load your resources here
    function onLayout(dc) {
        setLayout(Rez.Layouts.info_layout(dc));
        
        // Default-Werte setzen
        View.findDrawableById("line1").setText(Rez.Strings.status_default);
        View.findDrawableById("line2").setText(Rez.Strings.status_default);
        View.findDrawableById("line3").setText(Rez.Strings.status_default);
        View.findDrawableById("line4").setText(Rez.Strings.status_default);
    }

    //! Called when this View is brought to the foreground. Restore
    //! the state of this View and prepare it to be shown. This includes
    //! loading resources into memory.
    function onShow() {

    }

    //! Update the view
    function onUpdate(dc) {
        // Zuerst die View aktualiesieren:
        try {
            if(mSensor == null)
            {
                //
            }
            else
            {
                //
                
                if (mSensor.searching)
                {
                    //
                }
                else // !mSensor.searching
                {
                    View.findDrawableById("line1").setText("#" + mSensor.data.serialNumber.format("%4X"));
                    View.findDrawableById("line2").setText(mSensor.data.hardwareVersion.format("%u"));
                    View.findDrawableById("line3").setText(mSensor.data.softwareVersion.format("%u"));
                    View.findDrawableById("line4").setText(mSensor.data.manufacturerID.format("%u"));
                }   
            } // mSensor != null
  
        }
        catch( ex instanceof UnexpectedTypeException ) {
            // Code to handle the throw of UnexpectedTypeException
            Sys.println("UnexpectedTypeException in onUpdate");
            ex.printStackTrace();
        }
        catch( ex ) {
            // Code to catch all execeptions
            Sys.println("UnexpectedTypeException in onUpdate");
            ex.printStackTrace();
        }
        finally {
            
        }

        // Call the parent onUpdate function to redraw the layout
        View.onUpdate(dc);
        
        /**********************************************************************************/
        // Ab hier Überzeichnen wir das Layout
    }
    
    //! Called when this View is removed from the screen. Save the
    //! state of this View here. This includes freeing resources from
    //! memory.
    function onHide() {

    }
}
