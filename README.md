<h1 align="center">
    <p>SmartGuard</p>
</h1>

<p align="center">
  <a href="#about-the-project">About the Project</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
  <a href="#branches">Branches</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
  <a href="#dependencies">Dependencies</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
  <a href="#how-to-run">How to Run</a></br>
  <a href="#next-features">Next features</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
  <a href="#references">References</a>
</p>

<br />

## About the Project

This repository contains the code related to the Group 2 of Innovation Challenge Project SmartGuard.

The SmartGuard is about Smart Wearables for Impact and Fall Detection. The challenge description says:

"Accidents in construction, cycling, industrial work, and emergency response environments pose significant risks to human safety Head injuries, falls, and impact-related accidents are among the leading causes of severe injuries and fatalities in these sectors. In many cases, the lack of immediate medical attention following an accident exacerbates the severity of injuries. Moreover, workers often operate in isolated or hazardous conditions where timely communication is difficult, increasing the risk of delayed response. Similarly, elderly individuals are highly vulnerable to falls, which can lead to long-term health complications or death. Despite advancements in personal protective equipment, most helmets and safety gear lack integrated systems for real-time accident detection and alerting. This gap in functionality limits the effectiveness of safety measures, especially in dynamic or high-risk environments. There is a growing need for smart, sensor-enabled solutions that can monitor impacts, detect falls, and provide timely alerts to prevent severe outcomes."

In practice, we just used the readings of the IMU MPU-6050, to get the readings of accelerometer and gyroscope and processed it and applied on a threshold-based algorithm. The results are also sent via serial communication to a python-based HMI. The results gathered were simple but promissing.

## Branches

The current repository contains 3 branches:

- **master**: contains the embeeded Raspberry Pi Pico Code, written in C++. More information on how to build can be found [here](https://github.com/marcos-moura97/smart-guard/?tab=readme-ov-file#dependencies).

- **docs**: contains the reports from INS4000 class as well as the introdutory information about the challenge. All the files are in pdf format.

- **hmi**: contains the HMI code for real-time visualization of the data. More information on how to run can be found [here](https://github.com/marcos-moura97/smart-guard/?tab=readme-ov-file#how-to-run).

To switch between branched you can use the command:

```
$ git switch <branch_name>
```

And to list all the branches, just run:

```
$ git branch
```

## Dependencies

### Raspberry Pi Code

To build and flash the embedded software, I used the Visual Studio extension **Raspberry Pi Pico**, that contains all the necessary tools and packages for debugg, compile and flash the code.

Once you have it installed, it's very straight forward to compile and run, you can use the chapter 3 of [this tutorial](https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf?disposition=inline) as example.

### HMI

For the HMI, you need to have python3 installed on your machine, you need to install the python libraries with the command:

```
$ pip install -r requirements.txt
```

## How to Run

### Raspberry Pi Pico code

You can follow the steps of Chapter 5 in [this tutorial](https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf?disposition=inline) as example, but basically you just need to click **Run Project (USB)** once the raspberry is in flash mode.

### udev rules (Linux)

Copy the contents of [`60-picotool.rules`](https://github.com/raspberrypi/picotool/blob/master/udev/60-picotool.rules) to `/etc/udev/rules.d/60-picotool.rules`. This may be needed for setting the right permissions for the device (typically `/dev/ttyACM0`).

### HMI

First, change to the hmi branch:

```
$ git switch hmi
```

Then, install the requirements:

```
$ pip install -r requirements.txt
```

Then connect the prototype and check its COM port and update it on line 4 of main.py. After that, just run with:

```
$ python3 main.py
```

## Next Features

(we will decide on this Friday's meeting)

## References

[Raspberry Pi Pico Documentation](https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf?disposition=inline)

