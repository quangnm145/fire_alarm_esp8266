ls
mkdir -p ~/esp
cd esp/
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
ls
cd ESP8266_RTOS_SDK/
pwd
open .
export IDF_PATH=/home/QuangNM/esp/ESP8266_RTOS_SDK
python -m pip install --user -r $IDF_PATH/requirements.txt
python2.7 -m pip install --user -r $IDF_PATH/requirements.txt
cp -r $IDF_PATH/examples/get-started/hello_world .
ls
rm -rf hello_world/
cd ..
mkdir project
cd project/
cp -r $IDF_PATH/examples/get-started/hello_world .
ls
cd hello_world/
make menuconfig
cd ..
cd ..
git clone --recursive https://github.com/pfalcon/esp-open-sdk.git
git clone --recursive https://github.com/Superhouse/esp-open-rtos.git
ls
