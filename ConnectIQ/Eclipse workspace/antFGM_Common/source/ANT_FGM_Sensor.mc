//!
//! Copyright 2016 DessertHunter
//!
using Toybox.Ant as Ant;
using Toybox.System as Sys;
using Toybox.Test as Test;
using Toybox.Time as Time;


//! Anglehnt an: Bluetooth CGM Profil @see: https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=294793
//!
//! Lifecyle: ANT-Channel: Closed -> Open -> Searching -> Connected -> Disconnected/Researching -> Closed
//!
//! Communicating with ANT Sensors:
//! Connect IQ provides a low level interface for communication with ANT and ANT+ sensors.
//! With this interface, an ANT channel can be created to send and receive ANT packets.
//! The ANT Generic interface is not available to watch faces.
//! Low and High priority search timeout for sensors differs from the basic ANT radio specification
//! to allow for interoperation with native ANT behavior on devices. These are limited to a maximum timeout of 30 seconds and 5 seconds respectively.
class ANT_FGM_Sensor extends Ant.GenericChannel
{
    // ANT-Channel Settings:
    const DEVICE_NUMBER = 123;  //! 123 for *Provisory* ANT FGM (0=Wildcarded Device Number)
    const DEVICE_TYPE = 99;     //! 99 for *Provisory* ANT FGM (0=Wildcard)
    const TRANS_TYPE = 1;       //! 0 for Pairing
    const PERIOD = 8070;        //! 4,06 Hz Channel Period (0x1F86u)
    const RF_FREQ = 60;         //! *Provisory* ANT FGM RF Freq 2460 MHz
    const LOW_PRI_TIMEOUT = 12; //! 30s Low Priority Search Timeout, (2.5s increments) that a receiving channel will wait for in order to start tracking a master. Limited to a maximum of 30 seconds (Range of 0 to 12).
    // High priority search not supported in data-fields. const HI_PRI_TIMEOUT = 2;   //! 5s High Priority Search Timeout, (2.5s increments) that a receiving channel will wait for in order to start tracking a master. Limited to a maximum of 5 seconds (Range of 0 to 2).
    const PROXIMITY_BIN = 0;    //! Disable proximity pairing

    hidden var chanAssign;

    var data; // class FGM_Data
    var searching; // boolean
    var deviceCfg; // class Ant.DeviceConfig

    // Contains data of all Pages
    class FGM_Data
    {
        enum {
            GLUCOSE_PREDICTION_UNKNOWN,
            GLUCOSE_PREDICTION_FALLING,
            GLUCOSE_PREDICTION_FALLING_SLOW,
            GLUCOSE_PREDICTION_CONSTANT,
            GLUCOSE_PREDICTION_RISING_SLOW,
            GLUCOSE_PREDICTION_RISING,
            GLUCOSE_PREDICTION_SIZE
        }

        // Page 0
        hidden var pastSequenceNumber; // to detect new values
        var sequenceNumber;        // class Number / uint8_t
        var glucoseValue;          // class Number / uint16_t
        var glucosePrediction;     // class Number / uint8_t
        var glucoseClimbSinkRate;  // class Number / int8_t
        var timeOffset;            // class Number / uint16_t
        var momentLastMeasurement; // class Moment

        // Page 1
        var nfcState; // uint8_t
        var batteryLevel; // uint8_t

        // Page 2:
        var manufacturerID; // class Number / uint8_t      Manufacturer ID
        var serialNumber;   // class Number / uint16_t     Serial Number

        // Page 3:
        var hardwareVersion; // class Number / uint8_t      Hardware version
        var softwareVersion; // class Number / uint8_t      Software version
        var modelNumber;     // class Number / uint8_t      Model number

        function initialize()
        {
            sequenceNumber = 0;
            glucoseValue = 0;
            timeOffset = 0;
            pastSequenceNumber = sequenceNumber; // keine neuen Daten
            glucosePrediction = GLUCOSE_PREDICTION_UNKNOWN;
            nfcState = 0; // TODO: ENUM
            batteryLevel = 0;
            manufacturerID = 0;
            serialNumber = 0;
            hardwareVersion = 0;
            softwareVersion = 0;
            modelNumber = 0;
            momentLastMeasurement = new Time.Moment(0);
        }

        function queryIsNewData()
        {
          if (pastSequenceNumber != sequenceNumber) // Neue Daten?
          {
            pastSequenceNumber = sequenceNumber;
            return true;
          }
          return false;
        }
    }

    // FGM page 0 Parser
    class MeasurementDataPage
    {
        static const PAGE_NUMBER = 0;
        static const INVALID_GLUCOSE_VALUE = 0xFFFF;

        static function parse(payload, data)
        {
            // payload
            // [1] glucose_climb_sink_rate
            // [2] glucose_prediction
            // [3] time_offset_LSB
            // [4] time_offset_MSB
            // [5] sequence_number
            // [6] glucose_LSB
            // [7] glucose_MSB
            data.sequenceNumber = payload[5];
            data.glucoseValue = parseGlucose(payload);
            data.glucosePrediction = parseGlucosePrediction(payload);
            data.glucoseClimbSinkRate = payload[1];
            data.timeOffset = parseTimeOffset(payload);

            // Check if the data has changed
            if (data.queryIsNewData())
            {
                data.momentLastMeasurement = Time.now(); // Neue Werte
            }
        }

        static hidden function parseGlucosePrediction(payload)
        {
           return payload[2]; //
        }

        static hidden function parseGlucose(payload)
        {
           var glucose_value = (payload[6] | ((payload[7] & 0x0F) << 8));
           //Test.assertNotEqualMessage(glucose_value, INVALID_GLUCOSE_VALUE, "INVALID_GLUCOSE_VALUE parsed");
           return glucose_value;
        }

        static hidden function parseTimeOffset(payload)
        {
           return (payload[3] | ((payload[4] & 0x0F) << 8));
        }
    } // end class


    class SensorDataPage
    {
        static const PAGE_NUMBER = 1;

        // TODO: andere data-Member beschreiben
        static function parse(payload, data)
        {
            // payload
            // [1-3] cumulative_operating_time[3]
            // [4-5] reserved[2]
            //   [6] battery_level
            //   [7] nfc_state
            data.batteryLevel = payload[6];
            data.nfcState = payload[7];
        }

    } // end class


    class InfoDataPage2
    {
        static const PAGE_NUMBER = 2;

        static function parse(payload, data)
        {
            // payload:
            //   [1] manuf_id
            //   [2] serial_num_LSB
            //   [3] serial_num_MSB
            // [4-7] reserved[4]
            data.manufacturerID = payload[1];
            data.serialNumber = (payload[2] | ((payload[3] & 0x0F) << 8));
        }

    } // end class InfoDataPage2


    class InfoDataPage3
    {
        static const PAGE_NUMBER = 3;

        static function parse(payload, data)
        {
            // payload[1] = hw_version;
            // payload[2] = sw_version;
            // payload[3] = model_num;
            // payload[4-7] = reserved[4];
            data.hardwareVersion = payload[1];
            data.softwareVersion = payload[2];
            data.modelNumber = payload[3];
        }

    } // end class InfoDataPage3


    function initialize()
    {
        // Get the channel
        chanAssign = new Ant.ChannelAssignment(
            Ant.CHANNEL_TYPE_RX_NOT_TX, //! Open ANT (bidirectional) Slave Channel
            Ant.NETWORK_PUBLIC); //! Use ANT Public Network Key
        GenericChannel.initialize(method(:onMessage), chanAssign);

        // Set the configuration
        deviceCfg = new Ant.DeviceConfig( {
            :deviceNumber => DEVICE_NUMBER, // 0 = Wildcard our search
            :deviceType => DEVICE_TYPE,
            :transmissionType => TRANS_TYPE,
            :messagePeriod => PERIOD,
            :radioFrequency => RF_FREQ,                   // 57 = ANT+ Frequency
            :searchTimeoutLowPriority => LOW_PRI_TIMEOUT, // 10 = Timeout in 25s
            // High priority search not supported in data-fields. :searchTimeoutHighPriority => HI_PRI_TIMEOUT, // 0 = Pair to all transmitting sensors
            :searchThreshold => PROXIMITY_BIN
            // :networkKey64Bit => ?     // 64 bit network key TODO: Brauch ich glaube ich nicht wegen ChannelAssignment NETWORK_PLUS NETWORK_PUBLIC
            } );
        GenericChannel.setDeviceConfig(deviceCfg);

        data = new FGM_Data();
        searching = true;
    }

    function open()
    {
        //! Open the channel
        GenericChannel.open();

        data = new FGM_Data();
        searching = true;
    }

    function closeSensor()
    {
        GenericChannel.close();
    }

    function onMessage(msg)
    {
        try {
            //! Parse the payload
            var payload = msg.getPayload();

            if( Ant.MSG_ID_BROADCAST_DATA == msg.messageId )
            {
                var msg_page_number = (payload[0].toNumber() & 0x7F); // Anmerkung: Ohne Toggle Bit 7
                if( MeasurementDataPage.PAGE_NUMBER == msg_page_number)
                {
                    // Were we searching?
                    if(searching)
                    {
                        searching = false;

                        // Update our device configuration primarily to see the device number of the sensor we paired to
                        deviceCfg = GenericChannel.getDeviceConfig();
                    }

                    MeasurementDataPage.parse(payload, data);
                }
                else if (SensorDataPage.PAGE_NUMBER == msg_page_number)
                {
                    SensorDataPage.parse(payload, data);
                }
                else if (InfoDataPage2.PAGE_NUMBER == msg_page_number)
                {
                    InfoDataPage2.parse(payload, data);
                }
                else if (InfoDataPage3.PAGE_NUMBER == msg_page_number)
                {
                    InfoDataPage3.parse(payload, data);
                }
                else
                {
                    Sys.print("Warning: Unknown Page in ANT_FGM_Sensor::onMessage() ");
                    Sys.println(msg_page_number);
                }
            }
            else if( Ant.MSG_ID_CHANNEL_RESPONSE_EVENT == msg.messageId )
            {
                if( Ant.MSG_ID_RF_EVENT == (payload[0] & 0xFF) )
                {
                    if( Ant.MSG_CODE_EVENT_CHANNEL_CLOSED == (payload[1] & 0xFF) )
                    {
                        // Channel closed, re-open
                        open();
                        // Sys.println("UMSG_CODE_EVENT_CHANNEL_CLOSED in ANT_FGM_Sensor::onMessage()");
                    }
                    else if( Ant.MSG_CODE_EVENT_RX_FAIL_GO_TO_SEARCH == (payload[1] & 0xFF) )
                    {
                        searching = true;
                        // Sys.println("MSG_CODE_EVENT_RX_FAIL_GO_TO_SEARCH in ANT_FGM_Sensor::onMessage()");
                    }
                }
                else
                {
                    //It is a channel response.
                }
            }

        }
        catch( ex instanceof UnexpectedTypeException ) {
            // Code to handle the throw of UnexpectedTypeException
            Sys.println("UnexpectedTypeException in ANT_FGM_Sensor::onMessage()");
            ex.printStackTrace();
        }
        catch( ex ) {
            // Code to catch all execeptions
            Sys.println("Exception in ANT_FGM_Sensor::onMessage()");
            ex.printStackTrace();
        }
        finally {
            // Code to execute when
        }
    }
} // end class
