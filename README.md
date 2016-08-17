# antFGM
Bringing Flash Glucose Monitoring  and ANT wireless technology together

## Introduction

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


#### HW-Components

##### Transmitter

* ST CH95HF in SPI-Mode
  * [MikroElektronika RFIDclick](http://www.mikroe.com/click/rfid/)
  * or [Solutions Cubed BM019](http://www.solutions-cubed.com/bm019/)
* Nordic nRF51422 SoC module
  * **for evalation using** [Nordic nRF51 Dongle](https://www.nordicsemi.com/eng/Products/nRF51-Dongle)
  * Anker PowerBank as 5V Supply

##### Receiver

* Garmin Connect IQ 2 (Biker Monkey) compatible device like...
  * [Garmin vivoactive HR](https://buy.garmin.com/de-DE/DE/sport-training/fitness/vivoactive-hr/prod538374.html)
  * [Garmin Edge 1000](https://buy.garmin.com/de-DE/DE/sport-training/fahrrad/edge-1000/prod134491.html)


#### Software

* Keil uVision 5 Project:
  * based on Nordic nRF51 SDK v10.0.0 *multiprotocol/ble_ant_app_hrm* example
  * using Softdevice S310 (BLE & ANT)
  * providing BLE HRM Profile (not showed in Video), will switch to BLE GLS (Glucose Profile)
  * providing propriety own ANT FGM channel implementation based on ANT+ HRM profile (*because ANT+ Continuous Glucose Monitor Profile is since over 3 years still not official!!!*)
* Garmin Connect IQ Widget and Datafield:
  * based on MoxySensor-Example
  * Support devices: *vivosmart_hr* and *edge_1000*


### Installing


A step by step series of examples that tell you have to get a development env running

1. Installing Eclipse and Garmin Connect IQ SDK v2.1.2. See [Garmin Developer](http://developer.garmin.com/connect-iq/programmers-guide/getting-started/).
2. Next you will need Keil and Nordic nRF51 SDK v10.0.0. See [Nordic Infocenter](http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk51.v10.0.0/getting_started_installing.html?cp=6_0_1_1_0).


```
Find the Secret!
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo


## Deployment

You will need a programmer like *Segger J-Link*.


## Built With

Used IDEs:
* Eclipse Mars with Garmin ConnectIQ SDK
* KEIL uVision 5


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.


## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/DessertHunter/antFGM/tags).


## Authors

See the list of [contributors](https://github.com/DessertHunter/antFGM/contributors) who participated in this project.


## License

This project is licensed under the _MIT License_ - see the [LICENSE](LICENSE) file for details.
Be aware of additional _Nordic_ and _Dynastream_ - see [license.txt](/Keil µVision5/nRF51422/Documentation/license.txt).


## Acknowledgments

* **JoernL** [LimiTTer](https://github.com/JoernL/LimiTTer)
* **SandraK82**  [LibreRead-iOS](https://github.com/SandraK82/LibreRead-iOS)
* **vicktor** [FreeStyleLibre-NFC-Reader](https://github.com/vicktor/FreeStyleLibre-NFC-Reader) (Victor Bautista of Social Diabetes)
* **Pierre Vandevenne** http://type1tennis.blogspot.de
