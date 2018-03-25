#!/bin/python3.6
import multiprocessing
import os
from os import listdir
from sys import platform
import logging
import sys
import multiprocessing
import time
from serial import Serial
import serial.tools.list_ports
import numpy as np
import scipy
from scipy import interpolate
from tkinter import *
from math import floor

PEDAL_RATIO_TO_SPEED = 0.145475372
PEDAL_CONNECTED_MIN_VALUE = 150

'''
TODO:
When expression pedal connected ignore buttons 


'''



''' Profile Impementation Suggession
Profile speed change changes the incrementation of the profile not calculating a bunch of profiles 

word: phase accumenulator
'''


LEFT = "LEFT"
RIGHT = "RIGHT"
encoding = "ascii"

def discover():
    ''' Returns list of Communication devices '''
    return [p.device for p in list(serial.tools.list_ports.comports())]

class SerialContainer(object):
    def __init__(self, serial, role):
        self._serial = serial
        self._role = role

    def get_serial(self):
        return self._serial

    def get_role(self):
        return self._role

class SerialManager(object):
    ''' Handles Arduino Operations '''
    _baud = 0

    #300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, or 115200
    def __init__(self, tasks, queue, baud):
        self._tasks = tasks
        self._queue = queue
        self._baud = baud
        self._lookup = {
            "LCD":SerialTask,
            "SPEAKER":SerialWriteOnlyTask
        }
        logger.log(20,"discovering devices")
        serial_devices = discover()
        logger.log(20,serial_devices)
        self._serial_devices = {}
        for serial_device in serial_devices:
            logger.log(20,f"serial:{serial_device}")
            ser = serial.Serial(port=serial_device,baudrate=baud)
            role = ser.readline().decode("ASCII").strip()
            logger.log(20,f"role:{role} serial:{serial_device}")
            if not role in self._serial_devices.keys():
                self._serial_devices[role] = []
            self._serial_devices[role].append(SerialContainer(serial_device,role))

    def communication(self,role,data):
        num_jobs = 0
        if(role in self._serial_devices.keys()):
            for serial_device in self._serial_devices[role]:
                num_jobs +=1
                logger.log(20,f"Task started for {self._baud} baud on {serial_device.get_serial()} as {serial_device.get_role()}")
                self._tasks.put(self._lookup[role](serial_device.get_serial(),self._baud,data))
        while num_jobs:
            result = self._queue.get()
            num_jobs -= 1
            yield result

class SerialTask(object):
    def __init__(self,  serial, baud,  data=None):
        self._serial = serial
        self._data = data
        self._baud = baud

    def __call__(self):
        if(self._data is not None and len(self._data) > 0):
            s = self._serial
            ser = Serial(port=self._serial,baudrate=self._baud)
            ser.write(self._data.encode("ascii"))
            indata = ser.readline()
            return indata
        else:
            return None

class SerialWriteOnlyTask(object):
    def __init__(self,  serial, baud, data):
        self._serial = serial
        self._data = data
        self._baud = baud

    def __call__(self):
        s = self._serial
        ser = Serial(port=self._serial,baudrate=self._baud)
        ser.write(self._data.encode("ascii"))

class TaskConsumer(multiprocessing.Process):
    
    def __init__(self, task_queue, result_queue):
        multiprocessing.Process.__init__(self)
        self.task_queue = task_queue
        self.result_queue = result_queue

    def run(self):
        proc_name = self.name
        while True:
            next_task = self.task_queue.get()
            if next_task is None:
                # Poison pill means shutdown
                self.task_queue.task_done()
                break
            answer = next_task()
            self.task_queue.task_done()
            self.result_queue.put(answer)
        return

class SpeedManager(object):
    def __init__(self):
        ''' Goal max speed should be 3 times faster to spin than min speed'''
        self._speed = 1
        self.max_speed = 3

    def get_speed(self):
        ''' Returns current speed '''
        return self._speed
    def set_speed(self, value):
        ''' Updates current speed '''
        self._speed = value    
    def increase(self):
        ''' Increases speed '''
        if(self._speed < self.max_speed):
            self._speed +=1

    def decrease(self):
        ''' Decreases speed '''
        if(self._speed > 1):
            self._speed -=1


def get_profile_value(profile, phase_accum):
    profile_length = len(profile)
    index = phase_accum.lower()
    while profile_length-1 < index:
        phase_accum.dec(profile_length)
        index = phase_accum.lower()

    if profile_length -1 < index:
        x =1
    elif len(profile[index])!=2:
        x =1

    should_strobe = profile[index][1]
    if index > profile_length -1:
        phase_accum.dec(profile_length-1)

    index = phase_accum.lower()-1
    y1 = profile[index][0]\

    index += 1

    if index > profile_length -1:
        phase_accum.dec(profile_length-1)
    y2 = profile[index-1][0]
    slope = y2 - y1

    return (y1 + slope * phase_accum.fract(), should_strobe)

class PhaseAccumulator(object):

    def __init__(self, value):
        self._value = value

    def dec(self, speed):
        self._value -= speed
    def acc(self, speed):
        if(speed < 0):
            raise("Speed lower than range.")
        if(speed > 3):
            raise("Speed greater than range.")
        self._value += speed

    def lower(self):
        return floor(self._value)

    def fract(self):
        return self._value - self.lower()

class Profile(object):

    def __init__(self, speed_manager, left, right):
        self._name = ""
        self._left = left
        self._right = right
        self._acc = PhaseAccumulator(0)
        self._speed_manager = speed_manager

    def get_speed(self):
        return self._speed_manager.get_speed()
    def set_name(self, name):
        self._name = name

    def get_name(self):
        return self._name

    def get_left_value(self):
        return get_profile_value(self._left, self._acc)

    def get_right_value(self):
        return get_profile_value(self._right, self._acc)

    def set_next(self):
        self._acc.acc(self._speed_manager.get_speed())

class ProfileManager(object):
    _current_profile = 0
    _profiles = []

    def __init__(self, profile_folder, speed_manager):
        self._speed_manager = speed_manager
        self._current_profile_data = None
        self._profile_folder = profile_folder
        self.load_profiles()

    def outputData(self, data):
        return f"{data},|".rstrip()

    def _read_profile(self):
        ''' Returns a list of left, right values of 0-359 '''

        profile_file_name = self.get_current_profile()

        term = "|".encode(encoding)
        delimiter = ",".encode(encoding)
        with open(profile_file_name) as profile_file:
            left = []
            right = []
            for line in profile_file:
                profile_left, profile_right, strobe_left, strobe_right = [_ for _ in line.encode(encoding).rstrip().rstrip(term).rsplit(delimiter)]
                logger.log(10, f"left:{profile_left} right:{profile_right}")
                left.append((int(float(profile_left.decode(encoding))), int(strobe_left.decode(encoding))))
                right.append((int(float(profile_right.decode(encoding))), int(strobe_right.decode(encoding))))
            profile = Profile(self._speed_manager, left, right)
            profile.set_name(profile_file_name)
            self._current_profile_data = profile
        
    def load_profiles(self):
        ''' Loads Profiles '''
        self._profiles = [f"{self._profile_folder}{f}" for f in listdir(self._profile_folder) if os.path.isfile(f"{self._profile_folder}{f}")]

    def get_current_profile(self):
        ''' Returns the current profile '''
        return self._profiles[self._current_profile]

    def get_profile_data(self):
        if self._current_profile_data == None:
            self._read_profile()
        return self._current_profile_data

    def set_next(self):
        ''' Sets Profile to Next '''
        if(self._current_profile < len(self._profiles)-1):
            self._current_profile +=1
            self._read_profile()

    def set_previous(self):
        ''' Sets Profile to Previous '''
        if(self._current_profile > 0):
            self._current_profile -=1
            self._read_profile()



class SpeakerAppliance(object):
    ''' Connects the deviceManager with the ProfileManager '''

    def __init__(self, profile_location='profiles/', baud=115200, update_rate=.0001):
        # Establish communication queues
        self._tasks = multiprocessing.JoinableQueue()
        self._queue = multiprocessing.Queue()
        self._dm = SerialManager(self._tasks, self._queue, baud)
        self._sm = SpeedManager()
        self._pm = ProfileManager(os.path.join(os.path.dirname(__file__), profile_location), self._sm)
        self._default_count = 25
        self._update_rate = update_rate
        self._response_map = {
            "5":self.on_none,
            "0":self.on_right,
            "3":self.on_left,
            "1":self.on_up,
            "2":self.on_down
        }
        self._count = 0
        self._lastTime = int(round(time.time() * 10))


    def on_none(self):
        pass

    def okay_to_process(self):
        return self._count < 1

    def on_up(self):
        ''' When up button pressed '''
        logger.log(20,"on_up was called")
        self._count = self._default_count
        self._sm.increase()
        
    def on_down(self):
        ''' When down button pressed '''
        logger.log(20,"on_down was called")
        self._count = self._default_count
        self._sm.decrease()

    def on_left(self):
        ''' When left button pressed '''
        logger.log(20,"on_left was called")
        self._count = self._default_count
        self._pm.set_previous()

    def on_right(self):
        ''' When right button pressed '''
        logger.log(20,"on_right was called")
        self._count = self._default_count
        self._pm.set_next()

    def get_speed_from_raw_value(self, value):
        return floor((float(value) - PEDAL_CONNECTED_MIN_VALUE) * PEDAL_RATIO_TO_SPEED)
 
    def _process_response(self, raw_message):
        ''' Message from LCD '''
        message = raw_message.decode("ascii").strip()
        logger.log(30, message)
        pieces = message.split(",")
        if not len(pieces) is 2:
            logger.error("Invalid message from lcd {message}")
        else:
            button_command = pieces[0]
            raw_speed_command = pieces[1]
            if(button_command in self._response_map):
                self._response_map[button_command]()
            else:
                logger.error(f"Unable to process button response:{raw_message}")
            if not int(raw_speed_command)<PEDAL_CONNECTED_MIN_VALUE:
                processed_speed_value = self.get_speed_from_raw_value(raw_speed_command)
                logger.log(10, f"Processed Speed Value from Expression Pedal:{processed_speed_value}")
                #self._sm.set_speed(processed_speed_value)
            


    def profile_mapper(self, profile, role):
        left = profile.get_left_value()
        right = profile.get_right_value()
        profile_name = profile.get_name()
        profile_name =profile_name.split("/")[-1].ljust(16)
        speed = str(profile.get_speed()).ljust(16)
        if(role=="LCD"):
            result = f"{profile_name},{speed},|"
        elif(role=="SPEAKER"):
            result = f"{left[0]},{left[1]},{right[0]},{right[1]}|"
        logger.log(10, f"Role: {role}, Result: {result}")
        return result

    def _loop(self):
        #input("AnyKey")
        current_time = get_time()
        logger.log(10, "Loop")
        self._count -=1
        if(self._count < 0):
            self._count = 0


        #if current_time - self._waitTime > self._lastTime:
        logger.log(10,f"lastTime:{self._lastTime} current: {current_time}")
        profile =self._pm.get_profile_data()

        for _ in self._dm.communication("SPEAKER",self.profile_mapper(profile,"SPEAKER")):
            pass
        logger.log(10, profile.get_name())
        logger.log(10, profile.get_left_value())
        logger.log(10, profile.get_right_value())

        profile.set_next()
        self._lastTime = current_time
        for result in self._dm.communication("LCD",self.profile_mapper(profile,"LCD")):
            if(self.okay_to_process()):
                if result is not None:
                    self._process_response(result)

    # def stop(self):
    #     # Add a poison pill for each consumer
    #     for i in range(1,num_consumers):
    #         tasks.put(None)

    def start(self):
        ''' Update Applicance '''
        # Start consumers
        num_consumers = multiprocessing.cpu_count() * 3
        #print('Creating %d consumers' % num_consumers)
        consumers = [ TaskConsumer(self._tasks, self._queue)
                    for i in range(1,num_consumers) ]
        for w in consumers:
            w.start()

        while True:
            self._loop()

def get_time():
    return int(round(time.time() * 1000))


# if __name__ == '__main__':

#     # current_time = get_time()
#     # next_time = get_time()
#     # print(current_time)
#     # print(next_time)

#     # exit()
#     logger = logging.getLogger()
#     logger.setLevel(logging.DEBUG)
#     directory = "profiles/"
#     if(len(sys.argv)>1):
#         directory = sys.argv[1]
#     baud = 9600
#     if(len(sys.argv)>2):
#         baud = sys.argv[2]
#     ch = logging.StreamHandler(sys.stdout)
#     ch.setLevel(logging.DEBUG)
#     formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
#     ch.setFormatter(formatter)
#     logger.addHandler(ch)
#     sa = SpeakerAppliance(logging.getLogger(__name__),directory)
#     sa.start()




#profiles = [360,350,203,23,312]





#if __name__ == '__main__':
#    profiles = range(1, 200)
#    pa = PhaseAccumulator(1)
#    speed = 3


if __name__ == '__main__':
    #logger = logging.getLogger()
    #profile_location = "profiles/"
    #_sm = SpeedManager()
    #_pm = ProfileManager(logger, os.path.join(os.path.dirname(__file__), profile_location), _sm)
    #print(_pm.get_profile_data().get_left_value())
    #print(_pm.get_profile_data().get_right_value())
    #profile = _pm.get_profile_data()
    #for _ in range(100):
    #    print(profile.get_name())
    #    print(profile.get_left_value())
    #    print(profile.get_right_value())
    #     profile.set_next()

    current_time = get_time()
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    directory = "profiles/"
    if(len(sys.argv)>1):
     directory = sys.argv[1]
    baud = 115200
    if(len(sys.argv)>2):
     baud = sys.argv[2]
    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(50)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    ch.setFormatter(formatter)
    logger.addHandler(ch)
    sa = SpeakerAppliance(directory, baud)
    sa.start()
