a demo to run mindvision camera in linux

# How to run

```Bash
# install SDL
sudo apt-get update
sudo apt-get install libsdl2-2.0

# download & build
git clone https://github.com/FasterISP/mindvision_test.git
mkdir -p mindvision_test/build
cd mindvision_test/build
cmake ..
make -j8

# run
sudo ./example_camera_test
```