#include "elev.h"
#include "fsm.h"
#include "timer.h"
#include "queue.h"
#include <time.h>
#include <stdio.h>


int main() {
    // initialize hardware

    if (!elev_init()) {
        printf("Unable to initialize elevator hardware!\n");
        return 1;
    }
    // initializes the elevator
    FSM_init();
    init_arrays();
    State state = IDLE;

    int current_floor;
    int last_floor;
    int last_direction;

    while (1) {

        //updates current_floor, queue and last_floor continuously
        current_floor = elev_get_floor_sensor_signal();
        check_queue();
        if (elev_get_floor_sensor_signal() != -1){ last_floor = elev_get_floor_sensor_signal();}

        if (elev_get_stop_signal()){
            elev_set_motor_direction(DIRN_STOP);
            state = EMERGENCY_STOP;
        }

        switch (state){
            case IDLE:
                //checks if the elevator has stopped inbetween floors
                if ((current_floor == -1) && (direction == DIRN_STOP)){
                    elev_set_motor_direction(DIRN_STOP);
                    //checks if there are any orders and if the orders are above the previous floor
                    if ( check_queue() && order_above(last_floor-(last_direction==DIRN_DOWN))) {
                        direction = DIRN_UP;
                        elev_set_motor_direction(direction);  
                        state = DRIVE;
                    } 
                    else if(check_queue()){
                        direction = DIRN_DOWN;
                        elev_set_motor_direction(direction); 
                        state = DRIVE;
                    }
                    break;
                }
                //goes here if the elevator is in a floor
                if (check_orders()){
                    if(check_queue_floor(current_floor)){
                        state = DOORS_OPEN;
                    }else{
                        if(current_floor == -1){
                            direction = get_direction(last_floor);
                            elev_set_motor_direction(direction);
                            state = DRIVE; 
                        }else{
                            direction = get_direction(current_floor);
                            elev_set_motor_direction(direction);
                            state = DRIVE;
                        }
                    }
                }
                break;

            case DRIVE:
                if(elev_get_floor_sensor_signal() !=-1){
                    elev_set_floor_indicator(current_floor);
                }
                if (check_queue_floor(current_floor)){
                    //checks if the elevator is going to stop if a floor has an order
                    if((direction == DIRN_DOWN) && (order_floor_direction_down(current_floor) || (current_floor == 0) || (!order_below(current_floor)))){
                        elev_set_motor_direction(DIRN_STOP);
                        state = DOORS_OPEN;
                    }
                    else if((direction == DIRN_UP) && (order_floor_direction_up(current_floor) || (current_floor == N_FLOORS-1) || (!order_above(current_floor)))){
                        elev_set_motor_direction(DIRN_STOP);
                        state = DOORS_OPEN;
                    } 
                }
                break;

            case DOORS_OPEN:
                last_floor = current_floor;
                if(current_floor == -1){ //spør studass om denne
                    FSM_init();
                }
                if(!is_timer_on()){
                    timer_start();
                }
                delete_floor_order(current_floor);
                elev_set_door_open_lamp(1);
                if(timer_three_seconds()){
                    elev_set_door_open_lamp(0);
                    turn_off_timer();
                    if(check_orders()){
                        direction = get_direction(current_floor);
                        elev_set_motor_direction(direction);
                        state = DRIVE;
                    }else{
                        state = IDLE;
                    }
                }
                break;

            case EMERGENCY_STOP:
                last_direction = direction;
                elev_set_motor_direction(DIRN_STOP);
                direction = DIRN_STOP;
                delete_all_orders();
                while(elev_get_stop_signal()){
                    elev_set_stop_lamp(1);
                    if (current_floor != -1){
                        elev_set_door_open_lamp(1);
                    }
                }
                elev_set_stop_lamp(0);
                //checks if the stop button was pushed when the elevator was in a floor
                if(current_floor != -1){
                    if(!is_timer_on()){
                        timer_start();
                    }
                    elev_set_door_open_lamp(1);
                    if(timer_three_seconds()){
                        elev_set_door_open_lamp(0);
                        turn_off_timer();
                        state = IDLE;
                    }
                }else{
                    if(elev_get_stop_signal()){
                        state = EMERGENCY_STOP;
                    }else{

                        state = IDLE;
                    }
                }
                break;
        }

    }
    return 0;
}
