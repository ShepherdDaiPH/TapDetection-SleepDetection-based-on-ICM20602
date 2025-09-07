# TapDetection+InactiveDetection based on ICM20602 

#### Description

This is a project featuring on tap detection and inactive detection based on ICM20602 sensor and STM32F103T8U6TR MCU.
It is designed to differentiate tapping on the MCU from unintented swinging.
It can also automatically manage battery by setting MCU and ICM20602 to low power mode when MCU is inactive for more than one minute.
Low power mode can be exited when receiving wake-on-motion interrupt.
It is developed on Keil and configured by CubeMX.

---

#### Software Framework

This project contains following section of codes:
1. main.c and main.h (main function that initialize devices and freertos)
2. icm20602.c and icm20602.h (essential drivers that operate ICM20602)
3. tap_detection.c and tap_detection.h (functions used for tap detection)
4. Other essential codes required in development

---

#### Facing Problems

We tried to implement FreeRTOS feature into this project but failed due to unknown reasons (We suppose it was because  that the __WFI() could not be triggered).
So we aborted this feature in this edition.
We plan to figure out why it could not work and re-implement it in the future.
The feasibility of this project was tested on September 6th. It tuned out that everything worked well without FreeRTOS.

---

### Plan for future Presentation

We can prepare for future presentation in the following ways:
1. Measure the power consumption of the MCU during active mode and inactie mode for comparison.
2. Film the MCU when it is operating its tap detection feature.
3. Draw various data tables and graphs.

---

### Update

We added INA226 drivers to this project for future measurement.