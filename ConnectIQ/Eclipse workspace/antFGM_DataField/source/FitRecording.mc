//!
//! Copyright 2016 @DessertHunter
//!
using Toybox.Application as App;
using Toybox.WatchUi as Ui;
using Toybox.System as Sys;
using Toybox.FitContributor as Fit;


//! Recording to FIT-File
class FitRecording {

	const UINT16_MAX = 0xFFFF;

    // FIT field IDs, type=Number; The unique Field Identifier for the Field
    const GLUCOSE_FIT_FIELD_ID = 0; //type=Number; The unique Field Identifier for the Field

    // @see: http://developer.garmin.com/index.php/blog/post/connect-iq-2-the-full-circle
    hidden var mEnableFitRecording = true;

    // Variables for computing averages
    hidden var mLapRecordCount = 0;
    hidden var mSessionRecordCount = 0;
    hidden var mTimerRunning = false; // Boolean

    // FIT Contributions variables (class Toybox::FitContributor::Field)
    hidden var mGlucoseFitField;

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
            mGlucoseFitField = dataField.createField("current_glucose", GLUCOSE_FIT_FIELD_ID, Fit.DATA_TYPE_UINT16, {
                :count => 1, // The number of elements to add to the field if it is an array (Default 1)
                :mesgType => Fit.MESG_TYPE_RECORD, // The message type that this field should be added to. Defaults to MESG_TYPE_RECORD if not provided.
                :units => Ui.loadResource(Rez.Strings.fit0_units) // The display units as a String. This should use the current device language.
                });
            mGlucoseFitField.setData(0); // Default-Wert
        }
        else
        {
            mGlucoseFitField = null;
        }
    }

    function compute(sensor) {

        if (mEnableFitRecording && (sensor instanceof ANT_FGM_Sensor)) {
			var glucose_value = toUINT16(sensor.data.glucoseValue);
            mGlucoseFitField.setData(glucose_value);

            if( mTimerRunning ) {
                // Update lap/session data and record counts
                mLapRecordCount++;
                mSessionRecordCount++;
            }
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