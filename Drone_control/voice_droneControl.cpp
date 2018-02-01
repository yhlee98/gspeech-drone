#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <dronecore/dronecore.h>
#include <string>
#include <math.h>
#include <boost/algorithm/string.hpp>
#include <vector>
#include <bits/stdc++.h>

using namespace dronecore;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

#define ERROR_CONSOLE_TEXT "\033[31m" //Turn text on console red
#define TELEMETRY_CONSOLE_TEXT "\033[34m" //Turn text on console blue
#define NORMAL_CONSOLE_TEXT "\033[0m"  //Restore normal console colour
#define PORT 8080


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

int set_socket(){

    int sock = 0;
    struct sockaddr_in serv_addr;
    char hello[] = "Connection complete";
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("223.171.33.71");
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "223.171.33.71", &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
  
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
    read( sock , buffer, 1024);
    if (buffer[0] == '\0'){
        printf("Connection fail!\n");
        exit(EXIT_FAILURE);
    }
    printf("%s\n",buffer );
    return sock;

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


int Drone_control(Device &device, std::vector<std::string> command){

    const std::string &offb_mode = "COMMAND";
    int duration;
    float x, y, z, vel_x, vel_y, vel_z, velocity, degree;
    bool state_land = true;
    std::string command_input;
    Action::Result arm_result, takeoff_result, land_result;
    Offboard::Result offboard_result;
    Telemetry::Position location;


    // Command input   
    
    command_input = command[0];


    if (command_input ==  "takeoff"){

        arm_result = device.action().arm();
        action_error_exit(arm_result, "Arming failed");
        std::cout << "Armed" << std::endl;

        takeoff_result = device.action().takeoff();
        action_error_exit(takeoff_result, "Takeoff failed");
        std::cout << "In Air..." << std::endl;
        state_land = false;
       
        sleep_for(seconds(5));
        device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});
        Offboard::Result offboard_result = device.offboard().start();
        offboard_error_exit(offboard_result, "Offboard start failed");
        offboard_log(offb_mode, "Offboard started");
    }

    else if(command_input == "velgoto"){

        vel_x = stof(command[1],0);
        vel_y = stof(command[2],0);
        vel_z = stof(command[3],0);
        duration = stoi(command[4],0);

        if (!altitude_check(device, duration * vel_z)){
                return 0;
        }

        offboard_log(offb_mode,  "Velocity control");
        device.offboard().set_velocity_body({vel_x, vel_y, vel_z, 0.0f});
        sleep_for(seconds(duration));
        device.offboard().set_velocity_body({0.0f, 0.0f, 0.0f, 0.0f});        
    }

    else if(command_input == "goto"){

        x = stof(command[1],0);
        y = stof(command[2],0);
        z = stof(command[3],0);
        velocity = stof(command[4],0);

        if (!altitude_check(device, z)){
            return 0;
        }

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


    else if(command_input == "turn"){

        degree = stof(command[1],0);

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

    else if (command_input == "navgoto"){


        x = stof(command[1],0);
        y = stof(command[2],0);
        z = stof(command[3],0);
        velocity = stof(command[4],0);        

        location = device.telemetry().position();

        x = (x - location.latitude_deg)*6400000*M_PI/180;
        y = (y - location.longitude_deg)*6400000*M_PI/180;
        z = -(z - location.absolute_altitude_m);


        if (!altitude_check(device, z)){
            return 0;
            }

        vel_x = x*velocity/sqrt((x*x)+(y*y)+(z*z));
        vel_y = y*velocity/sqrt((x*x)+(y*y)+(z*z));
        vel_z = z*velocity/sqrt((x*x)+(y*y)+(z*z));

        duration = int(sqrt((x*x)+(y*y)+(z*z))/velocity);

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

    return 1;
}


std::vector<std::string> message_handling(char message[1024]){
    
    std::vector<std::string> results;
    boost::split(results, message, boost::is_any_of(" "));

    return results;
}


  
int main()//int argc, char const *argv[])
{

    int sock;
    char buffer[1024] = {0};
    std::vector<std::string> command;
    DroneCore dc;

    char quit_command[] = "quit";

    // open tcp socket as a client
    sock = set_socket();

    // connect with Drone
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


    
    //listen for socket
    while (true){

        read(sock, buffer, 1024);
        

        if (buffer[0] != '\0'){
            
            printf("[Message listen] %s\n", buffer);

            command = message_handling(buffer);
            for (int i = 0; i < 100; i++){
                buffer[i] = '\0';
            }

            if (command[0] == "finish"){
                command = message_handling(quit_command);
                Drone_control(device, command);
                close(sock);

                sleep_for(seconds(5));
                return EXIT_SUCCESS;
            }

            
            Drone_control(device, command);
            printf("Complete command!\n");
            send(sock, "Complete command!", strlen("Complete command!"), 0);


        }
        else{
            printf("Waiting for server command ...\n");
        }

        sleep_for(seconds(1));
    }

    return 0;
}