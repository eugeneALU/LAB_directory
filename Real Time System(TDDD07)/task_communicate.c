/**
 * @file	task_communicate.c
 * @author  Eriks Zaharans
 * @date    31 Oct 2013
 *
 * @section DESCRIPTION
 *
 * Communicate Task.
 */

/* -- Includes -- */
/* system libraries */

/* project libraries */
#include "task.h"
#include "timelib.h"

#define MAX_MESSAGE 10
#define MAX_SEND 5

 /**
 * Communication (receive and send data)
 */
void task_communicate(void)
{
	// Check if task is enabled
	if(g_task_communicate.enabled == s_TRUE)
	{
		// Loacal variables
		void *data; // Void pointer for data
		int data_type; // Data type

		// UDP Packet
		char udp_packet[g_config.udp_packet_size];
		int udp_packet_len;

		// Protocol
		protocol_t packet;

		//Start the new sequence
		int seq = 0; // Massi thing
		//In principle I want to send all the data in the buffer
		int last_id = g_list_send_victim->count + g_list_send_location->count + g_list_send_pheromone->count + g_list_send_stream->count; // Massi thing


		// --------------------------------------------------
		//	LAB 2 starts here
		// --------------------------------------------------
        send_location =0;
		/* --- Send Data --- */
		//sequence = victim -> location -> pheromone -> stream
		int package_sent = 0;
		while(g_list_send_victim->count != 0)
		{
            if (package_sent >= MAX_MESSAGE){
                discard_send[0] += g_list_send_victim->count;
                doublylinkedlist_empty(g_list_send_victim);
                break;
            }
            package_sent++;
            seq++;
            data = (void *)malloc(sizeof(victim_t));

            // Get data from the list
			doublylinkedlist_remove(g_list_send_victim, g_list_send_victim->first ,data, &data_type);

			// Encode data into UDP packet
			protocol_encode(udp_packet,
					&udp_packet_len,
					s_PROTOCOL_ADDR_BROADCAST,
					g_config.robot_id,
					g_config.robot_team,
					s_PROTOCOL_TYPE_DATA,
					seq,
					g_message_sequence_id,
					last_id,
					data_type,
					data);
			// Broadcast packet
			udp_broadcast(g_udps, udp_packet, udp_packet_len);
			send_times[0]++;
			total_send++;
			// Free memory
			free(data);
		}
		while(g_list_send_location->count != 0)
		{
            send_location++;
            if (package_sent >= MAX_MESSAGE || send_location > 1){
                discard_send[1] += g_list_send_location->count;
                doublylinkedlist_empty(g_list_send_location);
                break;
            }
            package_sent++;
            seq++;
            data = (void *)malloc(sizeof(robot_t));
			doublylinkedlist_remove(g_list_send_location, g_list_send_location->first ,data, &data_type);
			protocol_encode(udp_packet,
					&udp_packet_len,
					s_PROTOCOL_ADDR_BROADCAST,
					g_config.robot_id,
					g_config.robot_team,
					s_PROTOCOL_TYPE_DATA,
					seq,
					g_message_sequence_id,
					last_id,
					data_type,
					data);
			udp_broadcast(g_udps, udp_packet, udp_packet_len);
			send_times[1]++;
			total_send++;
			free(data);
		}
        if (!flip){
            flip = 1;
            while(g_list_send_pheromone->count != 0)
            {
                if (package_sent >= MAX_MESSAGE){
                    discard_send[2] += g_list_send_pheromone->count;
                    doublylinkedlist_empty(g_list_send_pheromone);
                    break;
                }
                package_sent++;
                seq++;
                data = (void *)malloc(sizeof(pheromone_map_sector_t));
                doublylinkedlist_remove(g_list_send_pheromone, g_list_send_pheromone->first ,data, &data_type);
                protocol_encode(udp_packet,
                        &udp_packet_len,
                        s_PROTOCOL_ADDR_BROADCAST,
                        g_config.robot_id,
                        g_config.robot_team,
                        s_PROTOCOL_TYPE_DATA,
                        seq,
                        g_message_sequence_id,
                        last_id,
                        data_type,
                        data);
                udp_broadcast(g_udps, udp_packet, udp_packet_len);
                send_times[2]++;
                total_send++;
                free(data);
            }
            while(g_list_send_stream->count != 0)
            {
                if (package_sent >= MAX_MESSAGE){
                    discard_send[3] += g_list_send_stream->count;
                    doublylinkedlist_empty(g_list_send_stream);
                    break;
                }
                package_sent++;
                seq++;
                data = (void *)malloc(sizeof(stream_t));
                doublylinkedlist_remove(g_list_send_stream, g_list_send_stream->first ,data, &data_type);
                protocol_encode(udp_packet,
                        &udp_packet_len,
                        s_PROTOCOL_ADDR_BROADCAST,
                        g_config.robot_id,
                        g_config.robot_team,
                        s_PROTOCOL_TYPE_DATA,
                        seq,
                        g_message_sequence_id,
                        last_id,
                        data_type,
                        data);
                udp_broadcast(g_udps, udp_packet, udp_packet_len);
                send_times[3]++;
                total_send++;
                free(data);
            }
        }
        else {
            flip = 0;
            while(g_list_send_stream->count != 0)
            {
                if (package_sent >= MAX_MESSAGE){
                    discard_send[3] += g_list_send_stream->count;
                    doublylinkedlist_empty(g_list_send_stream);
                    break;
                }
                package_sent++;
                seq++;
                data = (void *)malloc(sizeof(stream_t));
                doublylinkedlist_remove(g_list_send_stream, g_list_send_stream->first ,data, &data_type);
                protocol_encode(udp_packet,
                        &udp_packet_len,
                        s_PROTOCOL_ADDR_BROADCAST,
                        g_config.robot_id,
                        g_config.robot_team,
                        s_PROTOCOL_TYPE_DATA,
                        seq,
                        g_message_sequence_id,
                        last_id,
                        data_type,
                        data);
                udp_broadcast(g_udps, udp_packet, udp_packet_len);
                send_times[3]++;
                total_send++;
                free(data);
            }
            while(g_list_send_pheromone->count != 0)
            {
                if (package_sent >= MAX_MESSAGE+8){
                    discard_send[2] += g_list_send_pheromone->count;
                    doublylinkedlist_empty(g_list_send_pheromone);
                    break;
                }
                package_sent++;
                seq++;
                data = (void *)malloc(sizeof(pheromone_map_sector_t));
                doublylinkedlist_remove(g_list_send_pheromone, g_list_send_pheromone->first ,data, &data_type);
                protocol_encode(udp_packet,
                        &udp_packet_len,
                        s_PROTOCOL_ADDR_BROADCAST,
                        g_config.robot_id,
                        g_config.robot_team,
                        s_PROTOCOL_TYPE_DATA,
                        seq,
                        g_message_sequence_id,
                        last_id,
                        data_type,
                        data);
                udp_broadcast(g_udps, udp_packet, udp_packet_len);
                send_times[2]++;
                total_send++;
                free(data);
            }
        }
		/*
		while(g_list_send->count != 0)
		{
			seq++;

			// Allocate memory for data structure
			switch(g_list_send->first->data_type)
			{
			// Robot pose
			case s_DATA_STRUCT_TYPE_ROBOT :
				data = (void *)malloc(sizeof(robot_t));
				break;
			// Victim information
			case s_DATA_STRUCT_TYPE_VICTIM :
				data = (void *)malloc(sizeof(victim_t));
				break;
			// Pheromone map
			case s_DATA_STRUCT_TYPE_PHEROMONE :
				data = (void *)malloc(sizeof(pheromone_map_sector_t));
				break;
			// Command (for future use)
			case s_DATA_STRUCT_TYPE_CMD :
				data = (void *)malloc(sizeof(command_t));
				break;
			case s_DATA_STRUCT_TYPE_STREAM :
				data = (void *)malloc(sizeof(stream_t));
				break;
			// Other
			default :
				// Do nothing
				continue;
				break;
			}

			// Get data from the list
			doublylinkedlist_remove(g_list_send, g_list_send->first ,data, &data_type);

			// Encode data into UDP packet
			protocol_encode(udp_packet,
					&udp_packet_len,
					s_PROTOCOL_ADDR_BROADCAST,
					g_config.robot_id,
					g_config.robot_team,
					s_PROTOCOL_TYPE_DATA,
					seq,
					g_message_sequence_id,
					last_id,
					data_type,
					data);

            //record timing that victim is found(cannot >1300)
            if (data_type == s_DATA_STRUCT_TYPE_VICTIM) {
                double time_refine_to_send = timelib_timer_get(refine_to_send);
                if (time_refine_to_send > 1300) {over_R2S++;}
                printf("Send a victim in %f ms\n",time_refine_to_send);
                total_victim_found++;
                victim_found_time += time_refine_to_send;
            }
			// Broadcast packet
			udp_broadcast(g_udps, udp_packet, udp_packet_len);

			// Free memory
			free(data);
		}*/



		/* --- Receive Data --- */
		// Receive packets, decode and forward to proper process

		// Receive UDP packet
		while(udp_receive(g_udps, udp_packet, &udp_packet_len) == s_OK)
		{
			// Decode packet
			//printf("%s\n",udp_packet);
			if(protocol_decode(&packet, udp_packet, udp_packet_len, g_config.robot_id, g_config.robot_team) == s_OK)
			{
				// Now decoding depends on the type of the packet
				switch(packet.type)
				{
				// ACK
				case s_PROTOCOL_TYPE_ACK :
					// Do nothing
					break;

				//Massi: go_ahead packet
				case s_PROTOCOL_TYPE_GO_AHEAD :
				{
					// Declare go ahead command
					command_t go_ahead;
					go_ahead.cmd = s_CMD_GO_AHEAD;
					// Redirect to mission by adding it to the queue
					queue_enqueue(g_queue_mission, &go_ahead, s_DATA_STRUCT_TYPE_CMD);

					// Debuging stuff
					debug_printf("GO_AHEAD RECEIVED for robot %d team %d\n",packet.recv_id,packet.send_team);
					// Calculate time from packet (ms and s)
					int send_time_s = floor(packet.send_time / 1000);
					int send_time_ms = packet.send_time % 1000;
					int now = floor(((long long)timelib_unix_timestamp() % 60000) / 1000);
					debug_printf("GO_AHEAD_TIME: %d (%d)\n",send_time_s,now);

					break;
				}
				// Data
				case s_PROTOCOL_TYPE_DATA :
					// Continue depending on the data type
					switch(packet.data_type)
					{
					// Robot pose
					case s_DATA_STRUCT_TYPE_ROBOT :
						debug_printf("received robot\n");
						// Do nothing
						break;
					// Victim information
					case s_DATA_STRUCT_TYPE_VICTIM :
						debug_printf("received victim\n");
						// Redirect to mission by adding it to the queue
						queue_enqueue(g_queue_mission, packet.data, s_DATA_STRUCT_TYPE_VICTIM);
						break;
					// Pheromone map
					case s_DATA_STRUCT_TYPE_PHEROMONE :
						debug_printf("received pheromone\n");
						// Redirect to navigate by adding it to the queue
						queue_enqueue(g_queue_navigate, packet.data, s_DATA_STRUCT_TYPE_PHEROMONE);

						break;
					// Command
					case s_DATA_STRUCT_TYPE_CMD :
						debug_printf("received CMD\n");
                        // start timing that receive the command
                        timelib_timer_set(&receive_to_control);  //start timing
						// Redirect to mission by adding it to the queue
						queue_enqueue(g_queue_mission, packet.data, s_DATA_STRUCT_TYPE_CMD);
						break;
					case s_DATA_STRUCT_TYPE_STREAM :
						debug_printf("received data stream item\n");
						break;
					// Other
					default :
						// Do nothing
						break;
					}
				// Other ?
				default:
					// Do nothing
					break;
				}

				// Free memory (only if data packet was received!)
				if(packet.type == s_PROTOCOL_TYPE_DATA)
					free(packet.data);
			}
		}

		// Increase msg sequance id
		g_message_sequence_id++;
	}
}
