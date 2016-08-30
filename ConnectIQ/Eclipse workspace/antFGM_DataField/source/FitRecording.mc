//!
//! Copyright 2016 @DessertHunter
//!
using Toybox.Application as App;
using Toybox.WatchUi as Ui;
using Toybox.System as Sys;
using Toybox.FitContributor as Fit;


//! Recording to FIT-File
class FitRecording {

	const SINT8_MIN = -128;
	const SINT8_MAX = 127;
	const UINT16_MAX = 0xFFFF;

    // FIT field IDs, type=Number; The unique Field Identifier for the Field
    const CURRENT_GLUCOSE_FIT_FIELD_ID = 0;
    const GLUCOSE_CLIMBSINK_RATE_FIT_FIELD_ID = 1;

    // @see: http://developer.garmin.com/index.php/blog/post/connect-iq-2-the-full-circle
    hidden var mEnableFitRecording = true;

    // Variables for computing averages
    hidden var mLapRecordCount = 0;
    hidden var mSessionRecordCount = 0;
    hidden var mTimerRunning = false; // Boolean

    // FIT Contributions variables (class Toybox::FitContributor::Field)
    hidden var mCurrentGlucoseFitField;
    hidden var mGlucoseClimbSinkRateFitField;

    //! Constructor
    function initialize(dataField) {
        var app = App.getApp(); // class Application

        mEnableFitRecording = app.getProperty("enableRecording");
        if (null == mEnableFitRecording)
        {
            // Einstellung noch nicht gesetzt
            mEnableFitRecording = true;
            Sys.println("Setting enableRecording to Default");
            app.setProperty("enableRecording", mEnableFitRecording);
        }

        // Used to create a new field. Field is updated in the FIT file by changing the the value of the data within the Field. This method is to allow data fields access to FIT recording without giving them access to the session.
        if (mEnableFitRecording)
        {
            // ab ConnecIQ 1.3.0

            // Create a new field in the session.
            // Current namastes provides an file internal definition of the field
            // Field id _must_ match the fitField id in resources or your data will not display!
            // The field type specifies the kind of data we are going to store. For Record data this must be numeric, for others it can also be a string.
            // The mesgType allows us to say what kind of FIT record we are writing.
            //    FitContributor.MESG_TYPE_RECORD for graph information
            //    FitContributor.MESG_TYPE_LAP for lap information
            //    FitContributor.MESG_TYPE_SESSION` for summary information.
            // Units provides a file internal units field.
            mCurrentGlucoseFitField = dataField.createField("current_glucose", CURRENT_GLUCOSE_FIT_FIELD_ID, Fit.DATA_TYPE_UINT16, {
                :count => 1, // The number of elements to add to the field if it is an array (Default 1)
                :mesgType => Fit.MESG_TYPE_RECORD, // The message type that this field should be added to. Defaults to MESG_TYPE_RECORD if not provided.
                :units => Ui.loadResource(Rez.Strings.fit0_units) // The display units as a String. This should use the current device language.
                });
            mCurrentGlucoseFitField.setData(0); // Default-Wert
            
            mGlucoseClimbSinkRateFitField = dataField.createField("glucose_climbsink_rate", GLUCOSE_CLIMBSINK_RATE_FIT_FIELD_ID, Fit.DATA_TYPE_SINT8, {
                :count => 1, // The number of elements to add to the field if it is an array (Default 1)
                :mesgType => Fit.MESG_TYPE_RECORD, // The message type that this field should be added to. Defaults to MESG_TYPE_RECORD if not provided.
                :units => Ui.loadResource(Rez.Strings.fit1_units) // The display units as a String. This should use the current device language.
                });
            mGlucoseClimbSinkRateFitField.setData(0); // Default-Wert 
            
        }
        else
        {
            mCurrentGlucoseFitField = null;
            mGlucoseClimbSinkRateFitField = null;
            Sys.println("Could not create FIT fields! Disable recording!");
            mEnableFitRecording = false;
        }
    }

    function compute(sensor) {

        if (mEnableFitRecording && (sensor instanceof ANT_FGM_Sensor)) {
			var glucose_value = toUINT16(sensor.data.glucoseValue);
            mCurrentGlucoseFitField.setData(glucose_value);

			var glucose_climbsink_rate = toSINT8(sensor.data.glucoseClimbSinkRate);
            mGlucoseClimbSinkRateFitField.setData(glucose_climbsink_rate);

            if( mTimerRunning ) {
                // Update lap/session data and record counts
                mLapRecordCount++;
                mSessionRecordCount++;
            }
        }
    }

    hidden function toSINT8(value) {

        if (value instanceof Toybox.Lang.Number)
        {
            if (value < SINT8_MIN) {
	            return SINT8_MIN; // limit to min
            else if (value < SINT8_MAX) {
	            return SINT8_MAX; // limit to max
	        } else {
	            return value;
	        }
        }
        else
        {
            Sys.print("RecordFitData ERROR, value is no Number! value was "); Sys.println(value);
            mEnableFitRecording = false;
            return 0;
        }
    }
    
    hidden function toUINT16(value) {

        if (value instanceof Toybox.Lang.Number)
        {
            if (value > UINT16_MAX) {
	            return UINT16_MAX; // limit to max
	        } else {
	            return value;
	        }
        }
        else
        {
            Sys.print("RecordFitData ERROR, value is no Number! value was "); Sys.println(value);
            mEnableFitRecording = false;
            return 0;
        }
    }

    function setTimerRunning(state) {
        mTimerRunning = state;
    }

    function onTimerLap() {
        mLapRecordCount = 0;
    }

    function onTimerReset() {
        mSessionRecordCount = 0;
    }

    function getIsRecording() {
        return (mTimerRunning && mEnableFitRecording);
    }
}