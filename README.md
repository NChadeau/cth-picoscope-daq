### Prerequisites

* PyBind11 is used for python interfacing \
  Install in Ubuntu:
  ```sh
  apt install pybind11-dev
  ```
  Confirm it works by running:
  ```sh
  pybind11-config
  ```
* gcc-11 for PyBind11
* Python3
* PicoScope libraries (only made for ps6000 and ps6000a for the moment)\
  Installation instructions for Ubuntu and OpenSUSE can be found [here](https://www.picotech.com/downloads/linux)\
  Ubuntu:
  ```sh
  bash -c 'wget -O- https://labs.picotech.com/Release.gpg.key | gpg --dearmor > /usr/share/keyrings/picotech-archive-keyring.gpg' \
  bash -c 'echo "deb [signed-by=/usr/share/keyrings/picotech-archive-keyring.gpg] https://labs.picotech.com/picoscope7/debian/ picoscope main" >/etc/apt/sources.list.d/picoscope7.list' \
  apt-get update \
  apt install libps6000 libps6000a
  ```
  RedHat distributions:
  The picoscope libraries are not easily accessible in redhat and need enabling of SHA1 signature verification. All picoscope sdk packages can be found [here](https://labs.picotech.com/rc/picoscope7/rpm/). We also need to install dependencies manually. Note that this method is not secure and should not be considered a typical way of installing packages.
  To install the packages, we first enable SHA1 signature verification
  ```sh
  update-crypto-policies --set DEFAULT:SHA1
  ```
  Install our packages
  ```sh
  dnf install https://labs.picotech.com/rc/picoscope7/rpm/x86_64/libpicoipp-1.4.0-4r161.x86_64.rpm \
              https://labs.picotech.com/rc/picoscope7/rpm/x86_64/libps6000-2.1.139-6r6031.x86_64.rpm \
              https://labs.picotech.com/rc/picoscope7/rpm/x86_64/libps6000a-1.0.139-0r6031.x86_64.rpm
  ```
  And finally disable SHA1 support
  ```sh
  update-crypto-policies --set DEFAULT
  ```

### Installation

1. Run make
  ```
  make
  ```
  Hope that worked

### Documentation for fullDaq.py

  Usage documentation is available [here](https://docs.google.com/document/d/1bO7mmGigRIAl0k5rsDep0J5nz4fyK3EAK_bv9pZOlq8/edit?usp=drive_link) (WIP) and is visible to anyone
