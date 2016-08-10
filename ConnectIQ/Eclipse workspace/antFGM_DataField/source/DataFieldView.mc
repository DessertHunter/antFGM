using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;
using Toybox.Math as Math;
using Toybox.ActivityMonitor as Act;


class DataFieldView extends Ui.DataField {

    hidden var mSensor; // class ANT_FGM_Sensor
    hidden var mValue;
    
    const ALERTING_LOWER_THRESHOLD = 80; // Unterer Schwellwert
    const ALERTING_UPPER_THRESHOLD = 200; // Oberer Schwellwert
    hidden var mAlerting; // class Aletring
    
    // @see: http://developer.garmin.com/index.php/blog/post/connect-iq-2-the-full-circle
    const ENABLE_FIT_RECORDING = true;

    hidden var mGlucoseFitField; // class Toybox::FitContributor::Field
    const GLUCOSE_FIT_FIELD_ID = 0; //type=Number; The unique Field Identifier for the Field

    enum
    {
        STOPPED,
        PAUSED,
        RECORDING
    }
    hidden var mRecordFitState = STOPPED;


    enum {
        GLUCOSE_PREDICTION_FALLING, 
        GLUCOSE_PREDICTION_FALLING_SLOW, 
        GLUCOSE_PREDICTION_CONSTANT, 
        GLUCOSE_PREDICTION_RISING_SLOW, 
        GLUCOSE_PREDICTION_RISING,
        GLUCOSE_PREDICTION_SIZE = 5,
        GLUCOSE_PREDICTION_UNKOWN = -1
    }
    hidden var mArrows = new [GLUCOSE_PREDICTION_SIZE];
    hidden var mArrowIndex = GLUCOSE_PREDICTION_CONSTANT;

    //! Constructor
    function initialize(sensor)
    {
        DataField.initialize();

        mSensor = sensor;
        mValue = 0.0;
        
        mAlerting = new Alerting(ALERTING_LOWER_THRESHOLD, ALERTING_UPPER_THRESHOLD);
        
        // Used to create a new field. Field is updated in the FIT file by changing the the value of the data within the Field. This method is to allow data fields access to FIT recording without giving them access to the session.
        if (ENABLE_FIT_RECORDING)
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
            mGlucoseFitField = DataField.createField("current_glucose", GLUCOSE_FIT_FIELD_ID, FitContributor.DATA_TYPE_UINT16, {
                :count => 1, // The number of elements to add to the field if it is an array (Default 1)
                :mesgType => FitContributor.MESG_TYPE_SESSION, // The message type that this field should be added to. Defaults to MESG_TYPE_RECORD if not provided.
                :units => "mg/dL" // The display units as a String. This should use the current device language.
                });
    
            //var info = Act.getActivityInfo();
    
            //! If the activity timer is greater than 0, then we don't know the lap or timer state.
            //if( (info.timerTime != null) && (info.timerTime > 0) )
            //{
                //mLapCertainty = "?";
            //}
        }

    }

    hidden function doRecordFitData()
    {
        if (RECORDING == mRecordFitState)
        {
            var glucose_value = 50 + (Math.rand() % 120);
           
            Sys.print("RecordFitData Glucose="); Sys.println(glucose_value);
            mGlucoseFitField.setData(glucose_value);
        }
        else
        {
            Sys.println("RecordFitData not RECORDING!");
        }
    }
    
    //! This is called each time a lap is created, so increment the lap number.
    function onTimerLap()
    {
        //mLapNumber++;
    }

    //! The timer was started, so set the state to running.
    function onTimerStart()
    {
       mRecordFitState = RECORDING;
    }

    //! The timer was stopped, so set the state to stopped.
    function onTimerStop()
    {
        mRecordFitState = STOPPED;
    }

    //! The timer was started, so set the state to running.
    function onTimerPause()
    {
        mRecordFitState = PAUSED;
    }

    //! The timer was stopped, so set the state to stopped.
    function onTimerResume()
    {
        mRecordFitState = RECORDING;
    }

    //! The timer was reeset, so reset all our tracking variables
    function onTimerReset()
    {
        //mLapNumber = 0;
        mRecordFitState = STOPPED;
        //mLapCertainty = "";
    }
    

    //! Set your layout here. Anytime the size of obscurity of
    //! the draw context is changed this will be called.
    function onLayout(dc) {
        // Layout laden, in Abhängigkeit der Verdeckung z.B. bei runden Displays
        var obscurityFlags = DataField.getObscurityFlags();

        // Top left quadrant so we'll use the top left layout
        if (obscurityFlags == (OBSCURE_TOP | OBSCURE_LEFT)) {
            View.setLayout(Rez.Layouts.TopLeftLayout(dc));

        // Top right quadrant so we'll use the top right layout
        } else if (obscurityFlags == (OBSCURE_TOP | OBSCURE_RIGHT)) {
            View.setLayout(Rez.Layouts.TopRightLayout(dc));

        // Bottom left quadrant so we'll use the bottom left layout
        } else if (obscurityFlags == (OBSCURE_BOTTOM | OBSCURE_LEFT)) {
            View.setLayout(Rez.Layouts.BottomLeftLayout(dc));

        // Bottom right quadrant so we'll use the bottom right layout
        } else if (obscurityFlags == (OBSCURE_BOTTOM | OBSCURE_RIGHT)) {
            View.setLayout(Rez.Layouts.BottomRightLayout(dc));

        // Use the generic, centered layout
        } else {
            View.setLayout(Rez.Layouts.MainLayout(dc));
            var labelView = View.findDrawableById("label");
            labelView.locY = labelView.locY - 20; // 16
            var valueView = View.findDrawableById("value");
            valueView.locY = valueView.locY + 7;
        }

        View.findDrawableById("label").setText(Rez.Strings.label);
        
        // Prediction
        mArrows[GLUCOSE_PREDICTION_FALLING] = Ui.loadResource( Rez.Drawables.id_arrow_falling );
        mArrows[GLUCOSE_PREDICTION_FALLING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_falling_slow );
        mArrows[GLUCOSE_PREDICTION_CONSTANT] = Ui.loadResource( Rez.Drawables.id_arrow_constant );
        mArrows[GLUCOSE_PREDICTION_RISING_SLOW] = Ui.loadResource( Rez.Drawables.id_arrow_rising_slow );
        mArrows[GLUCOSE_PREDICTION_RISING] = Ui.loadResource( Rez.Drawables.id_arrow_rising );
        
        return true;
    }

    //! The given info object contains all the current workout
    //! information. Calculate a value and save it locally in this method.
    function compute(info) {
        // See Activity.Info in the documentation for available information.
        // mValue = 0.0;
        
        // DEBUG: Pfeil Animation
        if (mArrowIndex < (GLUCOSE_PREDICTION_SIZE-1))
        {
            mArrowIndex = mArrowIndex + 1;
        }
        else
        {
            // Einmal durchgewechselt...
            mArrowIndex = GLUCOSE_PREDICTION_FALLING;
            
            // TODO: Aufruf korrigeren
            doRecordFitData();
        }
        
        // Die Sensorwerte werden bei Empfang durch Ui.requestUpdate() aktualisiert, daher brauchen wir hier nichts machen!
    }

    //! Display the value you computed here. This will be called
    //! once a second when the data field is visible.
    function onUpdate(dc) {
        // Set the background color
        View.findDrawableById("Background").setColor(getBackgroundColor());

        // Set the foreground color and value
        var value = View.findDrawableById("value");
        if (getBackgroundColor() == Gfx.COLOR_BLACK) {
            value.setColor(Gfx.COLOR_WHITE);
        } else {
            value.setColor(Gfx.COLOR_BLACK);
        }
        
        if (mValue instanceof Toybox.Lang.Number)
        {
            value.setText(mValue.format("%.2f"));
        }
        else if (mValue instanceof Toybox.Lang.String)
        {
            value.setText(mValue);
        }
        else
        {
            value.setText("---");
            
            // DEBUG
            // Sys.println("mValue is not a number or string!");
        }

        // Call parent's onUpdate(dc) to redraw the layout
        View.onUpdate(dc);
        
        /**********************************************************************************/
        // Ab hier Überzeichnen wir das Layout
        try {
            // Update status
            var status_text_x =  10;
            var status_text_y =  dc.getHeight() - 25;
            dc.setColor(Gfx.COLOR_BLUE, Gfx.COLOR_TRANSPARENT);
                        
            if(mSensor == null)
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "No Channel!", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else if (mSensor.searching)
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Searching...", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else if (!mSensor.searching)
            {
                var glucose_value = mSensor.data.glucoseValue;
                mValue = glucose_value.format("%u"); // "%[flags][width][.precision]specifier" The supported specifiers are: d, i, u, o, x, X, f, e, E, g, G.
                
                mAlerting.doUpdate(glucose_value);
                
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Connected", Gfx.TEXT_JUSTIFY_LEFT);
            }
            else
            {
                dc.drawText(status_text_x, status_text_y, Gfx.FONT_MEDIUM, "Unknown!", Gfx.TEXT_JUSTIFY_LEFT);
            }
            
            // Arrow rechts mittig mit bisschen Platz zum Rand platzieren
            var arrow_x = dc.getWidth() - (32+5);
            var arrow_y = dc.getHeight()/2 - (32/2);
            dc.drawBitmap(arrow_x, arrow_y, mArrows[mArrowIndex]);
        }
        catch( ex instanceof UnexpectedTypeException ) {
            // Code to handle the throw of UnexpectedTypeException
            Sys.println("UnexpectedTypeException in onUpdate");
        }
        catch( ex ) {
            // Code to catch all execeptions
            Sys.println("UnexpectedTypeException in onUpdate");
            Sys.printStackTrace();
        }
        finally {
            // Code to execute when
        }
    }
}
