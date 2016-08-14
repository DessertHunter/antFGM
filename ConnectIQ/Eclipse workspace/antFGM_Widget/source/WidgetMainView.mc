//!
//! Copyright 2016 DessertHunter
//!
using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;
using Toybox.Test as Test;
using Toybox.Timer as Timer;


class WidgetMainView extends Ui.View {

    hidden var mSensor; // class ANT_FGM_Sensor

    const ALERTING_LOWER_THRESHOLD = 80; // Unterer Schwellwert
    const ALERTING_UPPER_THRESHOLD = 200; // Oberer Schwellwert
    hidden var mAlerting; // class Alerting
    hidden var mUpdateTimer; // class Toybox::Timer

    hidden var mTrend; // class TrendDrawable

    enum {
        GLUCOSE_PREDICTION_UNKNOWN,
        GLUCOSE_PREDICTION_FALLING,
        GLUCOSE_PREDICTION_FALLING_SLOW,
        GLUCOSE_PREDICTION_CONSTANT,
        GLUCOSE_PREDICTION_RISING_SLOW,
        GLUCOSE_PREDICTION_RISING,
        GLUCOSE_PREDICTION_SIZE
    }
    hidden var mArrowIndex = GLUCOSE_PREDICTION_CONSTANT;

    // NFC_STATES
    enum {
        NFC_STATE_UNKNOWN   = 0, // initial condition
        NFC_STATE_SLEEPING  = 1, // CR95HF is in sleep mode. Wakeup required
        NFC_STATE_ANSWERING = 2, // if any communication has been successful
        NFC_STATE_PROTOCOL  = 3, // a protocol (other then off) has been set
        NFC_STATE_TAG_IN_RANGE = 4 // a tag is in range and is responding without error (e.g. to inventory command). Now or read/write possible
    }

    //! Constructor
    function initialize(sensor) {
        View.initialize(); // You should always call the parent's initializer

        mSensor = sensor;

        mAlerting = new Alerting(ALERTING_LOWER_THRESHOLD, ALERTING_UPPER_THRESHOLD);

        mUpdateTimer = new Timer.Timer();
    }

    //! Load your resources here
    function onLayout(dc) {
        View.setLayout(Rez.Layouts.main_layout(dc));

        mTrend = View.findDrawableById("trend");
        
        // TODO: DEBUG
        Sys.print("dc Width="); Sys.print(dc.getWidth()); Sys.print("; Height="); Sys.println(dc.getHeight()); 
    }

    //! Called when this View is brought to the foreground. Restore
    //! the state of this View and prepare it to be shown. This includes
    //! loading resources into memory.
    function onShow() {
        Test.assertMessage(mUpdateTimer instanceof Timer.Timer, "onShow: Timer ist nicht initialisiert!");
        mUpdateTimer.start( method(:callbackUpdateTimer), 3000, true ); // alle 3s wiederholen
    }

    function callbackUpdateTimer()
    {
        Ui.requestUpdate();
    }

    //! Update the view
    function onUpdate(dc) {
        // First update View:

        // Update status
        var new_status_text = Rez.Strings.status_unknown;
        try {
            if(mSensor == null)
            {
                new_status_text = Rez.Strings.status_no_channel;
            }
            else
            {
                var rest = Time.now().subtract(mSensor.data.momentLastMeasurement); // Moment - Moment = Duration
                var measurement_age_in_seconds = rest.value();
                if (measurement_age_in_seconds > Toybox.Time.Gregorian.SECONDS_PER_HOUR)
                {
                    View.findDrawableById("measurement_age").setText(Rez.Strings.outdated_value);
                }
                else
                {
                    View.findDrawableById("measurement_age").setText(measurement_age_in_seconds.format("%u") + " [s]");
                }

                if (mSensor.searching)
                {
                    new_status_text = Rez.Strings.status_searching;
                    View.findDrawableById("value").setText(Rez.Strings.invalid_value);
                    mArrowIndex = GLUCOSE_PREDICTION_UNKNOWN;
                }
                else // !mSensor.searching
                {
                    new_status_text = Rez.Strings.status_connected;

                    // NFC Zustand:
                    if (NFC_STATE_UNKNOWN == mSensor.data.nfcState)
                    {
                        View.findDrawableById("nfc_state").setText("unkown");
                    }
                    else if (NFC_STATE_SLEEPING == mSensor.data.nfcState)
                    {
                        View.findDrawableById("nfc_state").setText("sleep");
                    }
                    else if (NFC_STATE_ANSWERING == mSensor.data.nfcState)
                    {
                        View.findDrawableById("nfc_state").setText("answer");
                    }
                    else if (NFC_STATE_PROTOCOL == mSensor.data.nfcState)
                    {
                        View.findDrawableById("nfc_state").setText("protocol");
                    }
                    else if (NFC_STATE_TAG_IN_RANGE == mSensor.data.nfcState)
                    {
                        View.findDrawableById("nfc_state").setText("tag");
                    }
                    else
                    {
                        View.findDrawableById("nfc_state").setText("???");
                    }

                    var glucose_value = mSensor.data.glucoseValue;
                    View.findDrawableById("value").setText(glucose_value.format("%u")); // "%[flags][width][.precision]specifier" The supported specifiers are: d, i, u, o, x, X, f, e, E, g, G.

                    mAlerting.doUpdate(glucose_value);

                    mArrowIndex =  mSensor.data.glucosePrediction;

                    mTrend.setArrow(mArrowIndex);
                    // TODO: Farben:
                    // COLOR_LT_GRAY = 0xAAAAAA Light Gray
                    // COLOR_DK_GRAY = 0x555555 Dark Gray
                    // COLOR_RED = 0xFF0000 Red
                    // COLOR_DK_RED = 0xAA0000 Dark Red
                    // COLOR_ORANGE = 0xFF5500 Orange
                    // COLOR_YELLOW = 0xFFAA00 Yellow
                    // COLOR_GREEN = 0x00FF00 Green
                    // COLOR_DK_GREEN = 0x00AA00 Dark Green
                    // COLOR_BLUE = 0x00AAFF Blue
                    // COLOR_DK_BLUE = 0x0000FF Dark Blue
                    // COLOR_PURPLE = 0xAA00FF Purple. Not valid on Fenix 3 or D2 Bravo. Use 0x5500AA instead.
                    // COLOR_PINK = 0xFF00FF Pink

                    if (glucose_value > 200)
                    {
                        mTrend.setColor(Gfx.COLOR_RED);
                    }
                    else if (glucose_value > 160)
                    {
                        mTrend.setColor(Gfx.COLOR_ORANGE);
                    }
                    else if (glucose_value > 120)
                    {
                        mTrend.setColor(Gfx.COLOR_GREEN);
                    }
                    else if (glucose_value > 100)
                    {
                        mTrend.setColor(Gfx.COLOR_DK_GREEN);
                    }
                    else if (glucose_value > 80)
                    {
                        mTrend.setColor(Gfx.COLOR_ORANGE);
                    }
                    else if (glucose_value > 80)
                    {
                        mTrend.setColor(Gfx.COLOR_RED);
                    }
                    else
                    {
                        mTrend.setColor(Gfx.COLOR_PINK); // Fall-Throw
                    }
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
            new_status_text = Rez.Strings.status_error;
            mArrowIndex = GLUCOSE_PREDICTION_UNKNOWN;

            Sys.println("UnexpectedTypeException in onUpdate");
            ex.printStackTrace();
        }
        finally {
            View.findDrawableById("status").setText(new_status_text);
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
        mUpdateTimer.stop();
    }

}
