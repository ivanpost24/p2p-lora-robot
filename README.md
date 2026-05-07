# WIoT final project

Authored by: Farjan Ahmed, Carlos Giron, Ivan Post

This is our final project for CS 4501: Wireless for the Internet of Things. It is a prototype system for controlling a mobile robot over long distances (from our observation, up to 0.32 km) using the LoRa physical layer protocol. It is designed to operate on the 915 MHz unlicensed band in the United States, which means you do not need an amateur radio license to use it. [This is a video of an operator controlling the robot from about 0.3 km away](https://drive.google.com/file/d/1XLk9izT9hsWHlGPpsFzeZ8Zzyy3jfpdS/view?usp=sharing); the control latency is primarily due to delays in the video feed and is under 500 ms in practice. The exact feasible range will depend on environmental factors.

We have provided instructions below to reproduce our results.

## Building the robot

### Components list:
* [WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) Heltec Board
* 2 [Sparkfun Dual tb6612fng Motor Drivers](https://www.sparkfun.com/sparkfun-motor-driver-dual-tb6612fng-1a.html) for motor control
* 4 [12V DC 100RPM Greartisan](amazon.com/Greartisan-Electric-Reduction-Centric-Diameter/dp/B072R57C56/ref=sr_1_6?crid=129Y1UB2Q6AGI&dib=eyJ2IjoiMSJ9.DwmBOBIYZeCSiQA5saHHIxj7DpqChe3S76xeKR8zBhYkiYd1JqMtlKpyXpX8ml9P-PodyTQpF1U53Aiup88Gdk4xNOoXEbOCuZ-eF9qleWwFV8hyrN2Rm18CDetzJ0f9fudnr4t8DZx4H_xdhjciGS_N9iVl0n4egDoUJzDH8lzn89O2e6Xu7hb-V0s1TQMCYfGA-40lgYDVYd1WclO5MOMkNcvgmsqaYL5KdbTtvnZBJRt6uXWlYDs0ATjglZvQkKjh9Chfi3PU3360LFzHv60yV7CS9oWNygZAhhukyWI.Be_9xpxrY9lM7WvLv97bnTWI3TeSQBAD8dNGXVNmA04&dib_tag=se&keywords=greartisan&qid=1778122489&sprefix=greara%2Caps%2C104&sr=8-6) Motors
* [INIU Portable Power Bank](https://www.amazon.com/INIU-Portable-Slimmest-Powerbank-Compatible/dp/B0DC9PV394/ref=sr_1_4?crid=18OPPD3YMVHBW&dib=eyJ2IjoiMSJ9.0WQzVjTrLphPpYXHXLB68u4mBA5Rbn288wfA5sWyp35DpGtrewRhO8osgHukWIII1DwwZKSDCB8bNecpqMsOLGRi4OvMCrledarzWXZo2mm3vVUFbNUYvgteSJSiUC4W4GYlY3ICUgLDd70C-CZUId6LxWdVb2gp8cnrTH14hRnbeg_DZL2gJpJ-KxWzOnmqB7snAoO4SP9RmN1tobYXKDJH2LOSkcSvpj6yAEEGyvA.t-OCLLBvhl4AjC0RjoiDnJkgwEFwfezcISsImUpkACE&dib_tag=se&keywords=inui%2Bpower%2Bbank&qid=1778122614&sprefix=inui%2Bpower%2Bbank%2B%2Caps%2C218&sr=8-4&th=1)
* [ACEIRMC USB Power Delivery Module](https://www.amazon.com/ACEIRMC-Trigger-Polling-Detector-Notebook/dp/B0BTHXKM91/ref=sxin_17_pa_sp_search_thematic_sspa?content-id=amzn1.sym.354b3233-22c2-4681-93ae-c50603e79747%3Aamzn1.sym.354b3233-22c2-4681-93ae-c50603e79747&crid=441D3EZMIEG8&cv_ct_cx=pd+module&keywords=pd+module&pd_rd_i=B0BTHXKM91&pd_rd_r=e3bb3bd6-a17f-436b-9dd3-0978a650ec78&pd_rd_w=KqERS&pd_rd_wg=o0LLY&pf_rd_p=354b3233-22c2-4681-93ae-c50603e79747&pf_rd_r=KJ86FRZV0K8ABR1RP7X3&qid=1778122403&s=electronics&sbo=RZvfv%2F%2FHxDF%2BO5021pAnSA%3D%3D&sprefix=pd+modul%2Celectronics%2C102&sr=1-1-9630ce6b-eb04-480e-8729-567d2ef7dacb-spons&aref=fOHUb9EmTP&sp_csd=d2lkZ2V0TmFtZT1zcF9zZWFyY2hfdGhlbWF0aWM&psc=1) for requesting 12V from a power bank
* [HiLetgo game joystick](amazon.com/HiLetgo-Controller-JoyStick-Breakout-Arduino/dp/B00P7QBGD2)

Additional pcb relevant components such as resistors, switches, and connectors are shown in this schematic:
[robot schematic](https://drive.google.com/file/d/1SUa91KxaiSQ853Xc8eG_WPLGtsJD8Y2j/view?usp=sharing)

Gerber files for our pcb using this schematic are included in the "/gerbers" directory.

### Pin assignments:
Assignments for the peripheral robot device are as follows:
* 19 - PWMA/B
* 20 - A/BIN1
* 26 - A/BIN2
* 47 - PWMA/B
* 48 - A/BIN1
* 33 - A/BIN2

These pins are used to set the direction of motors and speed of motors as they are connected to tb6612fng motor drivers. You **MUST NOT** power the motor drivers using the 3v3 pins of the Heltec board. Due to a hardware issue it seems that these boards are unable to power the motor drivers and remain on for operation. You must use a seperate 3v3 source to power the drivers.

In our case we used an *Arduino Uno R3* to deliver 3v3, but a simple battery should suffice. It is important to remember to ground both Arduino and Heltec on the same rail otherwise reference issues for digital logic occur.

Assignments for the central controller are as follows:
* 7 - Vrx
* 6 - Vry
* 5 - Sw

Although the datasheet recommends using 5V as input, our code was designed around using 3v3.
