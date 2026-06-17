#include <stdio.h>
#include "loopback.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "stdlib.h"
// #include <string.h> //< if you get an error on memset() you need string.h

#if LOOPBACK_MODE == LOOPBACK_MAIN_NOBLCOK

//static uint16_t j=0;
static uint16_t any_port = 	50000;
static uint8_t curr_state[8] = {0,};
static uint8_t sock_state[8] = {0,};
uint8_t* msg_v4 = "IPv4 mode";
uint8_t* msg_v6 = "IPv6 mode";
uint8_t* msg_dual = "Dual IP mode";

/**
 * @file loopback.c
 * @brief TCP/UDP Loopback Implementation File
 * @details Contains implementations for TCP server/client and UDP server loopback functionality
 *
 * This file provides network loopback implementations for:
 * - TCP server (IPv4/IPv6/Dual mode)
 * - TCP client (IPv4/IPv6/Dual mode) 
 * - UDP server (IPv4/IPv6/Dual mode)
 * - Custom TCP server implementation
 *
 * The loopback functions handle socket operations including:
 * - Socket creation and initialization
 * - Connection establishment 
 * - Data sending/receiving
 * - Connection termination
 *
 * @author WIZnet -- Modified by Nathan Cauwet
 * @date 2023 (WizNet) 2024 (Nathan Cauwet)
 * @note Requires wizchip_conf.h and socket.h
 * 
 * @callgraph 
 * @callergraph
 */
/// 
// #include "../../../../include/globals.h" // included by custom_cmd.h
// #include "globals.h"

/* global glob = {
    .dats[0]={.vals.v=5.0f, .vals.i=0.5f},
    .dats[1]={.vals.v=2.5f, .vals.i=0.25f}
    
}; */ /// \brief initialization of glob

//   EDITED  //
int32_t custom_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
{
    #ifdef BSERIES_EN
    if(eeprom_write_enabled || eeprom_active) return 1; //< skips if performing eeprom action. Prevents EEPROM corruption. return 1 = no error
    int32_t ret;
    datasize_t sentsize=0;
    int8_t status,inter;
    uint8_t tmp = 0;
    datasize_t received_size;
    uint8_t arg_tmp8;
    uint8_t* mode_msg;

    if(loopback_mode == AS_IPV4)
    {
       mode_msg = msg_v4;
    }else if(loopback_mode == AS_IPV6)
    {
       mode_msg = msg_v6;
    }else
    {
       mode_msg = msg_dual;
    }
    #ifdef _LOOPBACK_DEBUG_
        uint8_t dst_ip[16], ext_status;
        uint16_t dst_port;
    #endif
        getsockopt(sn, SO_STATUS, &status); //! get socket status
        switch(status)
        {
        case SOCK_ESTABLISHED :
        //! check for interrupt flag
            ctlsocket(sn,CS_GET_INTERRUPT,&inter); //! get interrupt value
            if(inter & Sn_IR_CON)
            {
                //! removed debug option
                arg_tmp8 = Sn_IR_CON;
                ctlsocket(sn,CS_CLR_INTERRUPT,&arg_tmp8); //! clear interrupt flag
            }
            getsockopt(sn,SO_RECVBUF,&received_size); //! check value & size of RECV buffer

            if(received_size > 0){
                /// Mirror the USB read_to_process limit: reject any command line longer than the
                /// shared MAX_BUFFER_SIZE instead of silently truncating it to DATA_BUF_SIZE.
                bool cmd_too_long = (received_size > MAX_BUFFER_SIZE - 1);
                if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE; //! if data is too big, truncate the recv
                ret = recv(sn, buf, received_size); //! recv data from peer (also drains the socket RX buffer)
                if(cmd_too_long) {
                    // Discard the over-long line and notify the client, same as the USB path.
                    format_and_send("\r\nERR; command too long (max %d chars), line discarded\r\n",
                                    MAX_BUFFER_SIZE - 1);
                    if(ret > 0) memset(buf, 0, (size_t)ret);
                } else {
                    char* buf_copy = strdup(buf);
                    if(user_input_buffer == NULL && sizeof(buf_copy) > 0) {
                        // copy_to_store(buf); //! copy the command to the user_input_buffer
                        // copy_to_store(buf_copy); //! copy the command to the user_input_buffer
                        process_commands(buf_copy); //! process the command
                    }
                    memset(buf, 0, strlen(buf)); //< should clear the buffer after it is copied. Unsure about size parameter
                }

                if(ret <= 0) return ret;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                received_size = (uint16_t) ret;
                sentsize = 0;

                /*  Below here handles echoing the message back to where it came from (hence "loopback")*/
                /* while(received_size != sentsize)
                {
                    ret = send(sn, buf+sentsize, received_size-sentsize);
                    if(ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
                } */
            }
            break;
        case SOCK_CLOSE_WAIT : // similar to a "do while" (kinda). Performs the action one more time then disconnects from socket
            #define _LOOPBACK_DEBUG_
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:CloseWait\r\n",sn);
            #endif
            getsockopt(sn, SO_RECVBUF, &received_size);
            if(received_size > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
            {
                if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE;
                ret = recv(sn, buf, received_size);

                if(ret <= 0) return ret;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                received_size = (uint16_t) ret;
                sentsize = 0;

                while(received_size != sentsize)
                {
                    ret = send(sn, buf+sentsize, received_size-sentsize);
                    if(ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
                }
            }

            if((ret = disconnect(sn)) != SOCK_OK) return ret;
                #ifdef _LOOPBACK_DEBUG_
                    printf("%d:Socket Closed\r\n", sn);
                #endif
            break;
        case SOCK_INIT : //! initialize socket
            if( (ret = listen(sn)) != SOCK_OK) return ret;
                    printf("%d:Listen, TCP server loopback, port [%d] as %s\r\n", sn, port, mode_msg);
            break;
        case SOCK_CLOSED: //! socket closed
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:TCP server loopback start\r\n",sn);
            #endif
                switch(loopback_mode)
                {
                case AS_IPV4:
                    tmp = socket(sn, Sn_MR_TCP4, port, SOCK_IO_NONBLOCK);
                    break;
                case AS_IPV6:
                    tmp = socket(sn, Sn_MR_TCP6, port, SOCK_IO_NONBLOCK);
                    break;
                case AS_IPDUAL:
                    tmp = socket(sn, Sn_MR_TCPD, port, SOCK_IO_NONBLOCK);
                    break;
                default:
                    break;
                }
                if(tmp != sn)    /* reinitialize the socket */
                {
                    #ifdef _LOOPBACK_DEBUG_
                        printf("%d : Fail to create socket.\r\n",sn);
                    #endif
                    return SOCKERR_SOCKNUM;
                }
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:Socket opened[%d]\r\n",sn, getSn_SR(sn));
                sock_state[sn] = 1;
            #endif
            break;
        default:
            break;
        }
    return 1;
    #endif
}

int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
{
    int32_t ret;
    datasize_t sentsize=0;
    int8_t status,inter;
    uint8_t tmp = 0;
    datasize_t received_size;
    uint8_t arg_tmp8;
    uint8_t* mode_msg;

    if(loopback_mode == AS_IPV4)
    {
       mode_msg = msg_v4;
    }else if(loopback_mode == AS_IPV6)
    {
       mode_msg = msg_v6;
    }else
    {
       mode_msg = msg_dual;
    }
    #ifdef _LOOPBACK_DEBUG_
        uint8_t dst_ip[16], ext_status;
        uint16_t dst_port;
    #endif
        getsockopt(sn, SO_STATUS, &status);
        switch(status)
        {
        case SOCK_ESTABLISHED :
            ctlsocket(sn,CS_GET_INTERRUPT,&inter);
            if(inter & Sn_IR_CON)
            {
                //! removed debug option
                arg_tmp8 = Sn_IR_CON;
                ctlsocket(sn,CS_CLR_INTERRUPT,&arg_tmp8);
            }
            getsockopt(sn,SO_RECVBUF,&received_size);

            if(received_size > 0){
                if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE;
                ret = recv(sn, buf, received_size);

                if(ret <= 0) return ret;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                received_size = (uint16_t) ret;
                sentsize = 0;

                while(received_size != sentsize)
                {
                    ret = send(sn, buf+sentsize, received_size-sentsize);
                    if(ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
                }
            }
            break;
        case SOCK_CLOSE_WAIT :
            #define _LOOPBACK_DEBUG_
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:CloseWait\r\n",sn);
            #endif
            getsockopt(sn, SO_RECVBUF, &received_size);
            if(received_size > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
            {
                if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE;
                ret = recv(sn, buf, received_size);

                if(ret <= 0) return ret;      // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                received_size = (uint16_t) ret;
                sentsize = 0;

                while(received_size != sentsize)
                {
                    ret = send(sn, buf+sentsize, received_size-sentsize);
                    if(ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
                }
            }

            if((ret = disconnect(sn)) != SOCK_OK) return ret;
                #ifdef _LOOPBACK_DEBUG_
                    printf("%d:Socket Closed\r\n", sn);
                #endif
            break;
        case SOCK_INIT :
            if( (ret = listen(sn)) != SOCK_OK) return ret;
                    printf("%d:Listen, TCP server loopback, port [%d] as %s\r\n", sn, port, mode_msg);
            break;
        case SOCK_CLOSED:
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:TCP server loopback start\r\n",sn);
            #endif
                switch(loopback_mode)
                {
                case AS_IPV4:
                    tmp = socket(sn, Sn_MR_TCP4, port, SOCK_IO_NONBLOCK);
                    break;
                case AS_IPV6:
                    tmp = socket(sn, Sn_MR_TCP6, port, SOCK_IO_NONBLOCK);
                    break;
                case AS_IPDUAL:
                    tmp = socket(sn, Sn_MR_TCPD, port, SOCK_IO_NONBLOCK);
                    break;
                default:
                    break;
                }
                if(tmp != sn)    /* reinitialize the socket */
                {
                    #ifdef _LOOPBACK_DEBUG_
                        printf("%d : Fail to create socket.\r\n",sn);
                    #endif
                    return SOCKERR_SOCKNUM;
                }
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:Socket opened[%d]\r\n",sn, getSn_SR(sn));
                sock_state[sn] = 1;
            #endif
            break;
        default:
            break;
        }
    return 1;
}


int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint8_t loopback_mode)
{

    int32_t ret; // return value for SOCK_ERRORs
    datasize_t sentsize=0;
    uint8_t status,inter,addr_len;
    datasize_t received_size;
    uint8_t tmp = 0;
    uint8_t arg_tmp8;
    wiz_IPAddress destinfo;


    // Socket Status Transitions
    // Check the W6100 Socket n status register (Sn_SR, The 'Sn_SR' controlled by Sn_CR command or Packet send/recv status)
    getsockopt(sn,SO_STATUS,&status);
    switch(status)
    {
    case SOCK_ESTABLISHED :
        ctlsocket(sn,CS_GET_INTERRUPT,&inter);
        if(inter & Sn_IR_CON)	// Socket n interrupt register mask; TCP CON interrupt = connection with peer is successful
        {
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:Connected to - %d.%d.%d.%d : %d\r\n",sn, destip[0], destip[1], destip[2], destip[3], destport);
            #endif
            arg_tmp8 = Sn_IR_CON;
            ctlsocket(sn,CS_CLR_INTERRUPT,&arg_tmp8);// this interrupt should be write the bit cleared to '1'
        }

        //////////////////////////////////////////////////////////////////////////////////////////////
        // Data Transaction Parts; Handle the [data receive and send] process
        //////////////////////////////////////////////////////////////////////////////////////////////
        getsockopt(sn, SO_RECVBUF, &received_size);

        if(received_size > 0) // Sn_RX_RSR: Socket n Received Size Register, Receiving data length
        {
            if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE; // DATA_BUF_SIZE means user defined buffer size (array)
            ret = recv(sn, buf, received_size); // Data Receive process (H/W Rx socket buffer -> User's buffer)

            if(ret <= 0) return ret; // If the received data length <= 0, receive failed and process end
            received_size = (uint16_t) ret;
            sentsize = 0;

            // Data sentsize control
            while(received_size != sentsize)
            {
                ret = send(sn, buf+sentsize, received_size-sentsize); // Data send process (User's buffer -> Destination through H/W Tx socket buffer)
                if(ret < 0) // Send Error occurred (sent data length < 0)
                {
                    close(sn); // socket close
                    return ret;
                }
                sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////
        break;

    case SOCK_CLOSE_WAIT :
        #ifdef _LOOPBACK_DEBUG_
            printf("%d:CloseWait\r\n",sn);
        #endif
        getsockopt(sn, SO_RECVBUF, &received_size);

        if((received_size = getSn_RX_RSR(sn)) > 0) // Sn_RX_RSR: Socket n Received Size Register, Receiving data length
        {
            if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE; // DATA_BUF_SIZE means user defined buffer size (array)
            ret = recv(sn, buf, received_size); // Data Receive process (H/W Rx socket buffer -> User's buffer)

            if(ret <= 0) return ret; // If the received data length <= 0, receive failed and process end
            received_size = (uint16_t) ret;
            sentsize = 0;

            // Data sentsize control
            while(received_size != sentsize)
            {
                ret = send(sn, buf+sentsize, received_size-sentsize); // Data send process (User's buffer -> Destination through H/W Tx socket buffer)
                if(ret < 0) // Send Error occurred (sent data length < 0)
                {
                    close(sn); // socket close
                    return ret;
                }
                sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
            }
        }
        if((ret=disconnect(sn)) != SOCK_OK) return ret;
            #ifdef _LOOPBACK_DEBUG_
                printf("%d:Socket Closed\r\n", sn);
            #endif
        break;

    case SOCK_INIT :
        #ifdef _LOOPBACK_DEBUG_
            if(loopback_mode == AS_IPV4)
                printf("%d:Try to connect to the %d.%d.%d.%d, %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
            else if(loopback_mode == AS_IPV6)
            {
                printf("%d:Try to connect to the %04X:%04X", sn, ((uint16_t)destip[0] << 8) | ((uint16_t)destip[1]),
                    ((uint16_t)destip[2] << 8) | ((uint16_t)destip[3]));
                printf(":%04X:%04X", ((uint16_t)destip[4] << 8) | ((uint16_t)destip[5]),
                    ((uint16_t)destip[6] << 8) | ((uint16_t)destip[7]));
                printf(":%04X:%04X", ((uint16_t)destip[8] << 8) | ((uint16_t)destip[9]),
                    ((uint16_t)destip[10] << 8) | ((uint16_t)destip[11]));
                printf(":%04X:%04X,", ((uint16_t)destip[12] << 8) | ((uint16_t)destip[13]),
                    ((uint16_t)destip[14] << 8) | ((uint16_t)destip[15]));
                printf("%d\r\n", destport);
            }
        #endif

        if(loopback_mode == AS_IPV4)
          ret = connect(sn, destip, destport, 4); /* Try to connect to TCP server(Socket, DestIP, DestPort) */
        else if(loopback_mode == AS_IPV6)
          ret = connect(sn, destip, destport, 16); /* Try to connect to TCP server(Socket, DestIP, DestPort) */

        // printf("SOCK Status: %d\r\n", ret); //! removed because it was spamming console

        if( ret != SOCK_OK) return ret;	//	Try to TCP connect to the TCP server (destination)
        break;

    case SOCK_CLOSED:
        switch(loopback_mode)
        {
        case AS_IPV4:
            tmp = socket(sn, Sn_MR_TCP4, any_port++, SOCK_IO_NONBLOCK);
            break;
        case AS_IPV6:
            tmp = socket(sn, Sn_MR_TCP6, any_port++, SOCK_IO_NONBLOCK);
            break;
        case AS_IPDUAL:
            tmp = socket(sn, Sn_MR_TCPD, any_port++, SOCK_IO_NONBLOCK);
            break;
        default:
            break;
        }

        if(tmp != sn){    /* reinitialize the socket */
            #ifdef _LOOPBACK_DEBUG_
                printf("%d : Fail to create socket.\r\n",sn);
            #endif
            return SOCKERR_SOCKNUM;
        }
        //! Commented out because it spammed the console. only useful for debugging
        // printf("%d:Socket opened[%d]\r\n",sn, getSn_SR(sn));
        sock_state[sn] = 1;

        break;
    default:
        break;
    }
    return 1;
}

//customized
static uint8_t _destip[16] = {0,};
static uint16_t _destport;
static uint8_t _addrlen;
int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
{
    uint8_t status;
    static uint8_t destip[16] = {0,};
    static uint16_t destport;
    uint8_t pack_info;
    uint8_t addr_len;
    datasize_t ret;
    datasize_t received_size;
    uint16_t size, sentsize;
    uint8_t* mode_msg;

    if(loopback_mode == AS_IPV4)
    {
            mode_msg = msg_v4;
    }else if(loopback_mode == AS_IPV6)
    {
            mode_msg = msg_v6;
    }else
    {
            mode_msg = msg_dual;
    }

    getsockopt(sn, SO_STATUS,&status);
    switch(status)
    {
    case SOCK_UDP:
        getsockopt(sn, SO_RECVBUF, &received_size);
        if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE;
        if(received_size>0)
        {
            ret = recvfrom(sn, buf, received_size, (uint8_t*)&destip, (uint16_t*)&destport, &addr_len);
            /* printf("%s\r\n", buf); //! verify buffer has what you expect it to have
            //! Process command received
            // cmdETH(&glob, buf); 
            cmdETH2(&glob, buf); /// \attention ONCE FINISHED, MOVE TO custom_tcps() */
            if(ret <= 0) //! if ret <= 0, an error has occured
             return ret;
            received_size = (uint16_t) ret;
            //! Warning: This will PROBABLY empty the buffer before it is passed to its pointer (in the function call) 
            for(int i=0; i<received_size+1; i++) {
                buf[i]=0;
            }
            if(destip != NULL) {
                printf("destip: %d.%d.%d.%d\n", destip[0], destip[1], destip[2], destip[3]);
                for(int i=0; i<count_of(destip); i++) {
                    _destip[i] = destip[i];
                }
                _destport = destport;
                _addrlen = addr_len;
            }
        }
        #ifdef BSERIES_EN
        if(datalog_flag == 1) {
            ret = recvfrom(sn, buf, received_size, (uint8_t*)&destip, (uint16_t*)&destport, &addr_len);
            // Datalog functionality disabled - measured_data removed from B960 module
            // float converted_volts[NUM_CHANNELS], converted_amps[NUM_CHANNELS];
            // for(int i=0; i<NUM_CHANNELS; i++) {
            //     converted_volts[i] = Fix2Float(measured_data[i].v);
            //     converted_amps[i] = Fix2Float(measured_data[i].i);
            // }
            // send_datalog(_destip, _destport, _addrlen, 
            //     "SN=%s\\r\\n"
            //     "A: %0.3fV\\t%0.4fA\\r\\n"
            //     "B: %0.3fV\\t%0.4fA\\r\\n" 
            //     "C: %0.3fV\\t%0.4fA\\r\\n"
            //     "D: %0.3fV\\t%0.4fA\\r\\n",
            //     serial_number,
            //     converted_volts[0], converted_amps[0],
            //     converted_volts[1], converted_amps[1], 
            //     converted_volts[2], converted_amps[2],
            //     converted_volts[3], converted_amps[3]
            // );
            datalog_flag = 0; //< reset flag for next timer interrupt
            memset(buf, 0, sizeof(buf)); //< empties dummy message from buffer
        }
        #endif
        break;
    case SOCK_CLOSED:

        switch(loopback_mode)
        {
        case AS_IPV4:
           socket(sn,Sn_MR_UDP4, port, SOCK_IO_NONBLOCK);
           break;
        case AS_IPV6:
           socket(sn,Sn_MR_UDP6, port, SOCK_IO_NONBLOCK);
           break;
        case AS_IPDUAL:
            socket(sn,Sn_MR_UDPD, port, SOCK_IO_NONBLOCK);
            break;
        }
        printf("%d:Opened, UDP loopback, port [%d] as %s\r\n", sn, port, mode_msg);

    }

    return 0;
}

/** -------------------------
 *      Backup of default
 *  -------------------------/
/* int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port, uint8_t loopback_mode)
{
    uint8_t status;
    static uint8_t destip[16] = {0,};
    static uint16_t destport;
    uint8_t pack_info;
    uint8_t addr_len;
    datasize_t ret;
    datasize_t received_size;
    uint16_t size, sentsize;
    uint8_t* mode_msg;

    if(loopback_mode == AS_IPV4)
    {
            mode_msg = msg_v4;
    }else if(loopback_mode == AS_IPV6)
    {
            mode_msg = msg_v6;
    }else
    {
            mode_msg = msg_dual;
    }

    getsockopt(sn, SO_STATUS,&status);
    switch(status)
    {
    case SOCK_UDP:
        getsockopt(sn, SO_RECVBUF, &received_size);
        if(received_size > DATA_BUF_SIZE) received_size = DATA_BUF_SIZE;
        if(received_size>0)
        {
            ret = recvfrom(sn, buf, received_size, (uint8_t*)&destip, (uint16_t*)&destport, &addr_len);

            if(ret <= 0)
             return ret;
            received_size = (uint16_t) ret;
            sentsize = 0;
            while(sentsize != received_size){
                ret = sendto(sn, buf+sentsize, received_size-sentsize, destip, destport, addr_len);

                if(ret < 0) return ret;

                sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
             }
        }
        break;
    case SOCK_CLOSED:

        switch(loopback_mode)
        {
        case AS_IPV4:
           socket(sn,Sn_MR_UDP4, port, SOCK_IO_NONBLOCK);
           break;
        case AS_IPV6:
           socket(sn,Sn_MR_UDP6, port, SOCK_IO_NONBLOCK);
           break;
        case AS_IPDUAL:
            socket(sn,Sn_MR_UDPD, port, SOCK_IO_NONBLOCK);
            break;
        }
        printf("%d:Opened, UDP loopback, port [%d] as %s\r\n", sn, port, mode_msg);

    }

    return 0;
} */

#endif