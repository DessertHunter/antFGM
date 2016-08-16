# antFGM
Bringing Flash Glucose Monitoring  and ANT wireless technology together

:loudspeaker: *WARNING:* Do-It-Yourself (DIY) means you are solely responsibly for everything :exclamation:

## TODO-List

- [ ] Writing this README
- [x] not yet completed
- [ ] Switching from USB-Power to LiPo with battery measurement
- [ ] Adding BLE DFU/OTA Support
- [ ] Adding CIQ Settings for eg. Alertinglevels
- [ ] Saving and Supporting
- [ ] Confirmation when Sensor-ID changes
- [ ] Remaining sensor time and mapping to RTC time
- [ ] Chart, with saving measurements in CIQ Object Store
- [ ] Deploying CIQ Widget & Datafield in Garmin Appstore

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

Demo of first working scanning: https://youtu.be/ph2V1yIZpBA

Screenshot of CIQ Widget on Garmin vivosmart HR: ![antFGM Screenshot on vivoactive HR](/Documentation/images/CIQ_screenshot_Widget_vivoactive_hr.PNG)


### Prerequisities

What things you need to install the software and how to install them

```
HW-Components used:
    * [MikroElektronika RFIDclick](http://www.mikroe.com/click/rfid/) ST CH95HF in SPI-Mode
    * [Nordic nRF51 Dongle](https://www.nordicsemi.com/eng/Products/nRF51-Dongle) nRF51422 SoC
    * Garmin vivoactive HR and Garmin Edge 1000
    * Anker PowerBank as 5V Supply

Software:
    * Soft device S310 (BLE & ANT), Keil uVision with Nordic nRF51 SDK 10.0
    * BLE HRM Profile (not showed in Video), will switch to BLE GLS (Glucose Profile)
    * Propriety own ANT FGM channel base on ANT+ HRM profile (*because ANT+ Continuous Glucose Monitor Profile is since over 3 years still not official!!!*)
    * Garmin Connect IQ based on MoxySensor-Example as Widget for vivosmart HR+ and Datafield for Edge 1000
```

### Installing

A step by step series of examples that tell you have to get a development env running

Stay what the step will be

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo


## Deployment

You will need a programmer like *Segger J-Link*.


## Built With

* Eclipse Mars with Garmin ConnectIQ SDK
* KEIL uVision 5


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.


## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/DessertHunter/antFGM/tags).


## Authors

* **DessertHunter** - *Initial work* - [antFGM](https://github.com/DessertHunter/antFGM)

See also the list of [contributors](https://github.com/DessertHunter/antFGM/contributors) who participated in this project.


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.


## Acknowledgments

* **JoernL** [LimiTTer](https://github.com/JoernL/LimiTTer)
* **SandraK82**  [LibreRead-iOS](https://github.com/SandraK82/LibreRead-iOS)
* **vicktor** [FreeStyleLibre-NFC-Reader](https://github.com/vicktor/FreeStyleLibre-NFC-Reader) (Victor Bautista of Social Diabetes)
* **Pierre Vandevenne** http://type1tennis.blogspot.de
