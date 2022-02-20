
CXX=g++
CXXARM=/usr/bin/arm-linux-gnueabihf-g++
CXXARM_ZERO=/opt/cross-pi-gcc-10.2.0-0/bin/arm-linux-gnueabihf-g++
EXE=test_buzzer
EXE_ARM=test_buzzer
BIN=bin

INCLUDE= -I ~/TDisk/Boost/boost_1_76_0/ 
INCLUDE_ARM =  -I /home/pi/boost_1_76_0/ -I /usr/include -I /usr/include/arm-linux-gnueabihf  
INCLUDE_ARM_ZERO =  -I /home/pi/boost_1_76_0/  -I /usr/include/arm-linux-gnueabihf/  -I /usr/include 


LIBS=  -pthread   -lcurl  -lssl -lcrypto /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_thread.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_system.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_chrono.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_context.a
LIBS_ARM = -pthread   -lcurl  -lssl -lcrypto   /home/pi/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/pi/boost_1_76_0/stage/lib/libboost_thread.a /home/pi/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/pi/boost_1_76_0/stage/lib/libboost_system.a /home/pi/boost_1_76_0/stage/lib/libboost_chrono.a /home/pi/boost_1_76_0/stage/lib/libboost_context.a
LIBS_ARM_ZERO = -pthread   -lcurl  -lssl -lcrypto   /home/pi/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/pi/boost_1_76_0/stage/lib/libboost_thread.a /home/pi/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/pi/boost_1_76_0/stage/lib/libboost_system.a /home/pi/boost_1_76_0/stage/lib/libboost_chrono.a /home/pi/boost_1_76_0/stage/lib/libboost_context.a

CXXFLAGS =  -std=c++17
CXXFLAGS_RELEASE= $(CXXFLAGS) -DRELEASE -s   -Werror=unused-variable -Werror=unused-result -Werror=parentheses
CXXFLAGS_DEBUG= $(CXXFLAGS)  -DDEBUG -g   
BUILD = build
ROOT= ../../../

OBJS =  $(BUILD)/test_buzzer.o \
		$(BUILD)/configFile.o \
		$(BUILD)/helpers.o \
		$(BUILD)/util_rpi.o \
		$(BUILD)/gpio.o \
		$(BUILD)/rpi_gpio.o \
		$(BUILD)/notification.o \
		$(BUILD)/leds.o \
		$(BUILD)/buzzer.o
		
	   

SRCS = $(ROOT)configFile.cpp \
	   $(ROOT)helpers/helpers.cpp \
	   $(ROOT)helpers/util_rpi.cpp \
	   $(ROOT)hal/gpio.cpp \
	   $(ROOT)hal_linux/rpi_gpio.cpp \
	   $(ROOT)notification_module/notification.cpp \
	   $(ROOT)notification_module/leds.cpp \
	   $(ROOT)notification_module/buzzer.cpp \
	   ../test_buzzer.cpp 
	   
	   


all: release





release: buzzer.release
	$(CXX)   -O2 -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS)  ;
	@echo "building finished ..."; 
	@echo "DONE."

debug: buzzer.debug
	$(CXX) -Og -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release: buzzer.arm.release
	$(CXXARM)    -O2  -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_debug: buzzer.arm.debug
	$(CXXARM)    -Og -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release_zero: buzzer.arm.release.zero
	$(CXXARM_ZERO)  -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM_ZERO) ;
	@echo "building finished ..."; 
	@echo "DONE."

buzzer.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   $(CXXFLAGS_RELEASE)  -c   $(SRCS)  $(INCLUDE)  ; 
	cd .. ; 
	@echo "compliling finished ..."

buzzer.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   $(CXXFLAGS_DEBUG)  -c  $(SRCS)  $(INCLUDE);
	cd .. ; 
	@echo "compliling finished ..."


buzzer.arm.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_RELEASE)  -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."

buzzer.arm.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_DEBUG) -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."



buzzer.arm.release.zero: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM_ZERO)   $(CXXFLAGS_RELEASE) -c  $(SRCS)  $(INCLUDE_ARM_ZERO)  ; 
	cd .. ; 
	@echo "compliling finished ..."

git_submodule:
	git submodule update --init --recursive


copy: clean
	mkdir -p $(BIN); \
	cp ../../config.*.json $(BIN); 
	@echo "copying finished"

clean:
	rm -rf $(BIN); 
	rm -rf $(BUILD);
	@echo "cleaning finished"



	

