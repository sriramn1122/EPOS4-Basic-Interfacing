# EPOS4-General-Config

This is an adaptation of original example provide by Maxon Motors with their Library. This program will help getting to speed quickly with EPOS4 controllers and allow rapid prototyping and testing

## Instructions to build and execute command:
1. Download and unzip file
2. Navigate into folder
3. Run make to compile the C++ file
4. Call the executable as per the following usage guidelines

## Usage:

**./drive_motor mode mode_value mode_parameters**

Available Modes :

| Velocity Mode                                                                                                                                          | Position Mode                                                                                                                                                                                                                  |
|--------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|  mode_value : is the RPM value input to the controller Allowed Range : (-104 to 104) RPM                                                               |  mode_value : is the position value ( in terms of shaft rotation) input to the controller Allowed Range : No limitations (ex: 2.5 rotations or 0.2 rotations)                                                                  |
|  mode_parameter - optional, can pass one to control data-logging It sets the log interval for the data file Default value for log interval is 1 second |  mode_parameter - optional, can pass one/two to control RPM and data-logging It sets the RPM during position seek It sets the log interval for data file Default value for RPM is 36Default value for log interval is 1 second |
| Termination: Controller will stay in this mode until kill signal (Ctrl+C                                                                               | Termination: Controller will exit after target position is reached, cannot force quit                                                                                                                                          |

**Note:** Killing power to the controller will stop all activity and reset the module

## Example Commands:

| Description                                                                        | Command                  |
|------------------------------------------------------------------------------------|--------------------------|
| To command the module to rotate at 45 RPM                                          | ./drive_motor vel 45     |
| To command the module to rotate out at 45 RPM and log data at 0.5 second intervals | ./drive_motor vel 45 0.5 |
| To command the module to rotate to 0.5 turns                                       | ./drive_motor pos 0.5    |
| To command the module to rotate to 10 turns at 50 RPM                              | ./drive_motor pos 10 50  |
| To command the module to rotate to 10 turns at 50 RPM and log data at 2 second intervals                             | ./drive_motor pos 5 40 2 |

## Data Output:

Data is written into a timestamped .csv file under the directory motor_data, with the following fields:
1. Velocity (RPM)
2. Position (QuadCounts)
3. Current (mA)
4. Time (time since epoch)



