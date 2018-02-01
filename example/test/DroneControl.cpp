#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <dronecore/dronecore.h>
#include <string>
#include <math.h>

using namespace dronecore;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

#define ERROR_CONSOLE_TEXT "\033[31m" //Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" //Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m"  //Restore normal console colour

// Handles Action's result
inline void action_error_exit(Action::Result result, const std::string &message)
{
    if (result != Action::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Action::result_str(
                      result) << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Handles Offboard's result
inline void offboard_error_exit(Offboard::Result result, const std::string &message)
{
    if (result != Offboard::Result::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message << Offboard::result_str(
                      result) << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Handles connection result
inline void connection_error_exit(DroneCore::ConnectionResult result, const std::string &message)
{
    if (result != DroneCore::ConnectionResult::SUCCESS) {
        std::cerr << ERROR_CONSOLE_TEXT << message
                  << DroneCore::connection_result_str(result)
                  << NORMAL_CONSOLE_TEXT << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Logs during Offboard control
inline void offboard_log(const std::string &offb_mode, const std::string msg)
{
    std::cout << "[" << offb_mode << "] " << msg << std::endl;
}


int altitude_check(Device &device, float altitude){
    Telemetry::Position location;
    float dalt;

    location = device.telemetry().position();
    dalt = location.relative_altitude_m - altitude;
    if (dalt <= 2.5){
        printf("Too low altitude: %f\n", dalt);
        return 0;
    }
    else{
        return 1;
    }
}

int main(int, char **)
{
    DroneCore dc;
    const std::string &offb_mode = "COMMAND";

    DroneCore::ConnectionResult conn_result = dc.add_udp_connection();
    connection_error_exit(conn_result, "Connection failed");

    // Wait for the device to connect via heartbeat
    while (!dc.is_connected()) {
        std::cout << "Wait for device to connect via heartbeat" << std::endl;
        sleep_for(seconds(1));
    }

    // Device got discovered.
    Device &device = dc.device();

    while (!device.telemetry().health_all_ok()) {
        std::cout << "Waiting for device to be ready" << std::endl;
        sleep_for(seconds(1));
    }
    std::cout << "Device is ready" << std::endl;

    // define variables
    int duration;
    float x, y, z, vel_x, vel_y, vel_z, velocity;
    float degree;
    bool state_land = true;
    Telemetry::Position location;
    Telemetry::Battery battery;
    std::string command_input;
    Action::Result arm_result, takeoff_result, land_result;
    Offboard::Result offboard_result;

    // Command input    
    while (true){

        std::cout << "Please input command: ";
        std::cin >> command_input;


        if (command_input ==  "takeoff"){

            arm_result = device.action().arm();
            action_error_exit(arm_result, "Arming failed");
            std::cout << "Armed" << std::endl;

            takeoff_result = device.action().takeoff();
            action_error_exit(takeoff_result, "Takeoff failed");
            std::cout << "In Air..." << std::endl;
            state_land = false;
            sleep_for(seconds(5));
        }

        else if(command_input == "velgoto"){

            std::cout << "X velocity: " << std::endl;
            std::cin >> vel_x;
            std::cout << "Y velocity: " << std::endl;
            std::cin >> vel_y;
            std::cout << "Z velocity: " << std::endl;
            std::cin >> vel_z;
            std::cout << "duration: " << std::endl;
            std::cin >> duration;

            if (!altitude_check(device, duration * vel_z)){
                continue;
            }

            offboard_log(offb_mode,  "Velocity control");
            device.offboard().set_velocity_body({vel_x, vel_y, vel_z, 0.0f});
            sleep_for(seconds(duration));
            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});
        }

        else if(command_input == "goto"){

            std::cout << "X distance: " << std::endl;
            std::cin >> x;
            std::cout << "Y distance: " << std::endl;
            std::cin >> y;
            std::cout << "Z distance: " << std::endl;
            std::cin >> z;
            std::cout << "Velocity: " << std::endl;
            std::cin >> velocity;

            vel_x = x*velocity/sqrt((x*x)+(y*y)+(z*z));
            vel_y = y*velocity/sqrt((x*x)+(y*y)+(z*z));
            vel_z = z*velocity/sqrt((x*x)+(y*y)+(z*z));

            duration = int(sqrt((x*x)+(y*y)+(z*z))/velocity);
            std::cout << duration << std::endl;

            if (!altitude_check(device, z)){
                continue;
            }

            offboard_log(offb_mode,  "Position Control");
            device.offboard().set_velocity_body({vel_x, vel_y, vel_z, 0.0f});
            sleep_for(seconds(duration));
            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});
        }


        else if(command_input == "yaw"){

            std::cout << "Heading degree: " << std::endl;
            std::cin >> degree;

            offboard_log(offb_mode,  "Degree Control");
            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 45});
            duration = int(1000*degree/45);
            sleep_for(milliseconds(duration));
            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});

        }

        else if(command_input == "land"){

            land_result = device.action().land();
            action_error_exit(land_result, "Landing failed");

            // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
            sleep_for(seconds(10));
            std::cout << "Landed" << std::endl;
            state_land = true;    
        }

        else if(command_input == "offboard_stop"){

            offboard_result = device.offboard().stop();
            offboard_error_exit(offboard_result, "Offboard stop failed: ");
            offboard_log(offb_mode, "Offboard stopped");     
        }
        else if(command_input == "offboard_start"){

            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});
            Offboard::Result offboard_result = device.offboard().start();
            offboard_error_exit(offboard_result, "Offboard start failed");
            offboard_log(offb_mode, "Offboard started");
        }

        else if (command_input == "b"){
            battery = device.telemetry().battery();
            printf("Voltage: %f\nRemaining percent: %f\n", battery.voltage_v, battery.remaining_percent);
        }

        else if(command_input == "gps"){

            location = device.telemetry().position();
            printf("GPS\nlat: %f\nlon: %f\nalt: %f\n", location.latitude_deg, location.longitude_deg, location.relative_altitude_m);
        }

        else if (command_input == "navgoto"){

            std::cout << "Latitude: " << std::endl;
            std::cin >> x;
            std::cout << "Longitude: " << std::endl;
            std::cin >> y;
            std::cout << "Altitude: " << std::endl;
            std::cin >> z;
            std::cout << "Velocity: " << std::endl;
            std::cin >> velocity;

            location = device.telemetry().position();

            x = (x - location.latitude_deg)*6400000*M_PI/180;
            y = (y - location.longitude_deg)*6400000*M_PI/180;
            z = -(z - location.absolute_altitude_m);


            if (!altitude_check(device, z)){
                continue;
            }

            printf( "x %f y %f z %f", x, y, z);



            vel_x = x*velocity/sqrt((x*x)+(y*y)+(z*z));
            vel_y = y*velocity/sqrt((x*x)+(y*y)+(z*z));
            vel_z = z*velocity/sqrt((x*x)+(y*y)+(z*z));

            duration = int(sqrt((x*x)+(y*y)+(z*z))/velocity);
            std::cout << duration << std::endl;

            offboard_log(offb_mode,  "Position Control");
            device.offboard().set_velocity_body({vel_x, vel_y, vel_z, 0.0f});
            sleep_for(seconds(duration));
            device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});

        }

        else if(command_input == "quit"){
            if(state_land != true){

                land_result = device.action().return_to_launch();
                action_error_exit(land_result, "Landing failed");

                // We are relying on auto-disarming but let's keep watching the telemetry for a bit longer.
                sleep_for(seconds(10));
                std::cout << "Landed" << std::endl;
                state_land = true;
            }

            return EXIT_SUCCESS;
        }

        else{
            
            std::cout << "Wrong Command!" << std::endl;
        }

    }
}