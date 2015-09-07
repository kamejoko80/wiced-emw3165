/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <inttypes.h>

#include "wiced.h"
#include "platform_audio.h"
#include "command_console.h"
#include "console_wl.h"

#include "wifi/command_console_wifi.h"

#include "audio_render.h"

#include "rtp_player.h"
#include "apollo_config.h"

#ifdef AUDIOPCM_ENABLE
#include "audiopcm.h"
#endif

#ifdef WWD_TEST_NVRAM_OVERRIDE
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define Mod32_GT( A, B )        ( (int32_t)( ( (uint32_t)( A ) ) - ( (uint32_t)( B ) ) ) >   0 )

#define RTP_CONSOLE_COMMANDS \
    { (char*) "exit",           rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Exit application" }, \
    { (char*) "play",           rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Start playing" }, \
    { (char*) "stop",           rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Stop playing" }, \
    { (char*) "port",           rtp_console_command,    1, NULL, NULL, (char *)"", (char *)"Set the rtp port" }, \
    { (char*) "volume",         rtp_console_command,    1, NULL, NULL, (char *)"", (char *)"Set the audio volume (0-100)" }, \
    { (char*) "config",         rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Display / change config values" }, \
    { (char*) "ascu_enable",    rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Enable ASCU interrupts" }, \
    { (char*) "ascu_disable",   rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Disable ASCU interrupts" }, \
    { (char*) "ascu_time",      rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"Display current time" }, \
    { (char*) "clock_test",     rtp_console_command,    0, NULL, NULL, (char *)"", (char *)"802.1AS clock test" }, \


/******************************************************
 *                    Constants
 ******************************************************/

#define RTP_MULTICAST_IPV4_ADDRESS          MAKE_IPV4_ADDRESS(224, 0, 0, 55)

#define MY_DEVICE_NAME                      "rtp_player"
#define MY_DEVICE_MODEL                     "1.0"
#define MAX_RTP_COMMAND_LENGTH              (85)
#define RTP_CONSOLE_COMMAND_HISTORY_LENGTH  (10)

#define RTP_THREAD_PRIORITY                 4

#define WICED_RTP_BUFFER_NODE_COUNT         (256)

#define FIRMWARE_VERSION                    "wiced-1.0"

#define RTP_THREAD_STACK_SIZE               (2 * 4096)

#define RTP_PACKET_TIMEOUT_MS               (500)

#define RTP_UDP_QUEUE_BACKLOG               (24)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    RTP_CONSOLE_CMD_EXIT = 0,
    RTP_CONSOLE_CMD_PLAY,
    RTP_CONSOLE_CMD_STOP,
    RTP_CONSOLE_CMD_PORT,
    RTP_CONSOLE_CMD_VOLUME,
    RTP_CONSOLE_CMD_CONFIG,
    RTP_CONSOLE_CMD_ENABLE,
    RTP_CONSOLE_CMD_DISABLE,
    RTP_CONSOLE_CMD_TIME,
    RTP_CONSOLE_CMD_CLOCK_TEST,

    RTP_CONSOLE_CMD_MAX,
} RTP_CONSOLE_CMDS_T;

#define NUM_NSECONDS_IN_SECOND                      (1000000000LL)
#define NUM_USECONDS_IN_SECOND                      (1000000)
#define NUM_NSECONDS_IN_MSECOND                     (1000000)
#define NUM_NSECONDS_IN_USECOND                     (1000)
#define NUM_USECONDS_IN_MSECOND                     (1000)
#define NUM_MSECONDS_IN_SECOND                      (1000)

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct cmd_lookup_s {
        char *cmd;
        uint32_t event;
} cmd_lookup_t;

typedef struct time_snapshot_s {
        int64_t ns_clock;
        int64_t master_clock;
        int64_t local_clock;
} time_snapshot_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

int rtp_console_command(int argc, char *argv[]);

/******************************************************
 *               Variables Definitions
 ******************************************************/

#ifdef PLATFORM_L1_CACHE_BYTES
#define NUM_BUFFERS_POOL_SIZE(x)       ((WICED_LINK_MTU_ALIGNED + sizeof(wiced_packet_t) + 1) * (x))
#define APP_RX_BUFFER_POOL_SIZE        NUM_BUFFERS_POOL_SIZE(WICED_RTP_BUFFER_NODE_COUNT)
#endif

#ifdef PLATFORM_L1_CACHE_BYTES
uint8_t                          rtp_rx_packets[APP_RX_BUFFER_POOL_SIZE + PLATFORM_L1_CACHE_BYTES]        __attribute__ ((section (".external_ram")));
#else
uint8_t                          rtp_rx_packets[WICED_NETWORK_MTU_SIZE * WICED_RTP_BUFFER_NODE_COUNT]     __attribute__ ((section (".external_ram")));
#endif
static wiced_bool_t rtp_stream_packet_pool_created;

static char rtp_command_buffer[MAX_RTP_COMMAND_LENGTH];
static char rtp_command_history_buffer[MAX_RTP_COMMAND_LENGTH * RTP_CONSOLE_COMMAND_HISTORY_LENGTH];

uint8_t rtp_thread_stack_buffer[RTP_THREAD_STACK_SIZE]                               __attribute__ ((section (".bss.ccm")));

const command_t rtp_command_table[] = {
    RTP_CONSOLE_COMMANDS
    WL_COMMANDS
    WIFI_COMMANDS
    CMD_TABLE_END
};

static cmd_lookup_t command_lookup[RTP_CONSOLE_CMD_MAX] = {
        { "exit",           PLAYER_EVENT_SHUTDOWN   },
        { "play",           PLAYER_EVENT_PLAY       },
        { "stop",           PLAYER_EVENT_STOP       },
        { "port",           0                       },
        { "volume",         0                       },
        { "config",         0                       },
        { "ascu_enable",    0                       },
        { "ascu_disable",   0                       },
        { "ascu_time",      0                       },
        { "clock_test",     0                       },
};

const char* firmware_version = FIRMWARE_VERSION;

static rtp_player_t *g_player;

/******************************************************
 *               Function Definitions
 ******************************************************/

static void clock_test(rtp_player_t* player, int iterations)
{
    int64_t start_ns_time, start_net_mtime, start_net_ltime;
    wiced_result_t result;
    uint32_t ns_secs;
    uint32_t ns_nsecs;
    uint32_t master_secs;
    uint32_t master_nsecs;
    uint32_t local_secs;
    uint32_t local_nsecs;
    time_snapshot_t *tarray;
    int count;

    (void)player;

    tarray = calloc(iterations, sizeof(time_snapshot_t));
    if (tarray == NULL)
    {
        WPRINT_APP_INFO(("Unable to allocate time snapshot array\r\n"));
        return;
    }

    /*
     * Make sure that we can read the 802.1AS time. We'll only check the return value once
     * on the theory that if it works once, it'll always work.
     */

    result = wiced_time_read_8021as(&master_secs, &master_nsecs, &local_secs, &local_nsecs);

    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to read 802.1AS time\r\n"));
        return;
    }

    WPRINT_APP_INFO(("Starting %d clock iterations\r\n", iterations));

    wiced_init_nanosecond_clock();

    start_ns_time = wiced_get_nanosecond_clock_value();
    wiced_time_read_8021as(&master_secs, &master_nsecs, &local_secs, &local_nsecs);
    start_net_mtime = (int64_t)master_secs * NUM_NSECONDS_IN_SECOND + (int64_t)master_nsecs;
    start_net_ltime = (int64_t)local_secs  * NUM_NSECONDS_IN_SECOND + (int64_t)local_nsecs;

    for (count = 0; count < iterations; ++count)
    {
        wiced_rtos_delay_milliseconds(500);
        tarray[count].ns_clock = (int64_t)wiced_get_nanosecond_clock_value();
        wiced_time_read_8021as(&master_secs, &master_nsecs, &local_secs, &local_nsecs);
        tarray[count].master_clock = (int64_t)master_secs * NUM_NSECONDS_IN_SECOND + (int64_t)master_nsecs;
        tarray[count].local_clock  = (int64_t)local_secs * NUM_NSECONDS_IN_SECOND + (int64_t)local_nsecs;
    }

    WPRINT_APP_INFO(("Completed %d clock iterations\r\n", count));

    /*
     * Now print out the results.
     */

    for (count = 0; count < iterations; ++count)
    {
        ns_secs  = (uint32_t)((tarray[count].ns_clock - start_ns_time) / NUM_NSECONDS_IN_SECOND);
        ns_nsecs = (uint32_t)((tarray[count].ns_clock - start_ns_time) % NUM_NSECONDS_IN_SECOND);

        master_secs  = (uint32_t)((tarray[count].master_clock - start_net_mtime) / NUM_NSECONDS_IN_SECOND);
        master_nsecs = (uint32_t)((tarray[count].master_clock - start_net_mtime) % NUM_NSECONDS_IN_SECOND);

        local_secs  = (uint32_t)((tarray[count].local_clock - start_net_ltime) / NUM_NSECONDS_IN_SECOND);
        local_nsecs = (uint32_t)((tarray[count].local_clock - start_net_ltime) % NUM_NSECONDS_IN_SECOND);

        WPRINT_APP_INFO(("Sample %d   Nanosecond %lu.%09lu   Master %lu.%09lu   Local %lu.%09lu\r\n",
                         count + 1, ns_secs, ns_nsecs, master_secs, master_nsecs, local_secs, local_nsecs));
    }

    free(tarray);
}


int rtp_console_command(int argc, char *argv[])
{
    uint32_t event = 0;
    uint8_t volume;
    int port;
    int i;

    WPRINT_APP_INFO(("Received command: %s\n", argv[0]));

    if (g_player == NULL || g_player->tag != PLAYER_TAG_VALID)
    {
        WPRINT_APP_INFO(("Bad player structure\r\n"));
        return ERR_CMD_OK;
    }

    /*
     * Lookup the command in our table.
     */

    for (i = 0; i < RTP_CONSOLE_CMD_MAX; ++i)
    {
        if (strcmp(command_lookup[i].cmd, argv[0]) == 0)
            break;
    }

    if (i >= RTP_CONSOLE_CMD_MAX)
    {
        WPRINT_APP_INFO(("Unrecognized command: %s\n", argv[0]));
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case RTP_CONSOLE_CMD_EXIT:
        case RTP_CONSOLE_CMD_PLAY:
        case RTP_CONSOLE_CMD_STOP:
            event = command_lookup[i].event;
            break;

        case RTP_CONSOLE_CMD_PORT:
            port = atoi(argv[1]);
            if (port)
            {
                g_player->rtp_port  = port;
                g_player->rtcp_port = port + 1;
            }
            break;

        case RTP_CONSOLE_CMD_VOLUME:
            volume = (uint8_t)atoi(argv[1]);
            if (g_player->audio)
            {
                wiced_audio_render_set_volume(g_player->audio, volume);
            }
            break;

        case RTP_CONSOLE_CMD_CONFIG:
            apollo_set_config(g_player, argc, argv);
            break;

        case RTP_CONSOLE_CMD_ENABLE:
            wiced_time_enable_8021as();
            break;

        case RTP_CONSOLE_CMD_DISABLE:
            wiced_time_disable_8021as();
            break;

        case RTP_CONSOLE_CMD_TIME:
        {
            uint32_t master_secs;
            uint32_t master_nanosecs;
            uint32_t local_secs;
            uint32_t local_nanosecs;
            int result;

            result = wiced_time_read_8021as(&master_secs, &master_nanosecs, &local_secs, &local_nanosecs);

            if (result == WICED_SUCCESS)
            {
                wiced_time_t wtime;

                wiced_time_get_time(&wtime);
                printf("Master time = %u.%u secs\n", (unsigned int)master_secs, (unsigned int)master_nanosecs);
                printf("Current local time = %u.%u secs\n", (unsigned int)local_secs, (unsigned int)local_nanosecs);
                printf("Wtime is %d\n", (int)wtime);
            }
            else
            {
                printf("Error returned from wiced_time_read_8021as\n");
            }
        }
        break;

        case RTP_CONSOLE_CMD_CLOCK_TEST:
            clock_test(g_player, atoi(argv[1]));
            break;
    }

    if (event)
    {
        /*
         * Send off the event to the main audio loop.
         */

        wiced_rtos_set_event_flags(&g_player->events, event);
    }

    return ERR_CMD_OK;
}


void host_get_firmware_version(const char **firmware_string)
{
    *firmware_string = firmware_version;
}


static int get_num_channels(int chmap)
{
    int num_channels;

    switch (chmap)
    {
        case 0: num_channels = 1; break;
        case 1: num_channels = 2; break;
        case 2: num_channels = 3; break;
        case 3: num_channels = 6; break;
        case 4: num_channels = 8; break;
        case 5: num_channels = 12; break;
        case 14: num_channels = 24; break;
        default: num_channels = -1; break;
    }

    return num_channels;
}


wiced_result_t buffer_release(wiced_audio_render_buf_t *buf, void* session_ptr)
{
    rtp_player_t* player = (rtp_player_t *)session_ptr;

    if (player == NULL || player->tag != PLAYER_TAG_VALID)
    {
        WPRINT_APP_INFO(("buffer release with bad player handle %p\n", player));
    }

    if (buf != NULL && buf->opaque != NULL)
    {
        /* Delete the packet */
        wiced_packet_delete((wiced_packet_t *)buf->opaque);
        buf->opaque   = NULL;
        buf->data_buf = NULL;
    }

    return WICED_SUCCESS;
}


static wiced_result_t process_packet_header(rtp_player_t *player, uint32_t *rtp_data, uint32_t *pt)
{
    wiced_result_t result = WICED_SUCCESS;
    wiced_audio_render_params_t params;
    uint16_t bits_per_sample;
    uint32_t header;
    uint32_t sample_rate;
    uint32_t format;
    uint32_t seq;
    int num_channels;

    /*
     * Check for a sequence number error.
     */

    header = (uint32_t)(ntohl(rtp_data[RTP_AUDIO_OFFSET_BASE]));
    seq    = header & RTP_MAX_SEQ_NUM;
    *pt    = (header >> 16) & 0x7F;
    if (player->seq_valid != WICED_FALSE && seq != player->next_seq)
    {
        WPRINT_APP_INFO(("SEQ error - cur %ld, last %ld (lost %"PRIi32")\r\n", seq, player->last_seq, (seq-1-player->last_seq)));
    }

    /*
     * Update the sequence number information.
     */

    player->last_seq  = seq;
    player->next_seq  = (player->last_seq + 1) & RTP_MAX_SEQ_NUM;
    player->seq_valid = 1;

    /*
     * If this is an audio packet, parse the audio format from the RTP extension.
     */

    if (*pt == RTP_PAYLOAD_AUDIO)
    {
        format = ntohl(rtp_data[RTP_AUDIO_OFFSET_FORMAT]);

        switch (format & RTP_AUDIO_SAMPLE_RATE_MASK)
        {
            case RTP_AUDIO_SAMPLE_RATE_441: sample_rate = 44100; break;
            case RTP_AUDIO_SAMPLE_RATE_48:  sample_rate = 48000; break;
            case RTP_AUDIO_SAMPLE_RATE_96:  sample_rate = 96000; break;
            case RTP_AUDIO_SAMPLE_RATE_192: sample_rate = 192000; break;
            default:
                WPRINT_APP_INFO(("Unknown sample rate\r\n"));
                return WICED_ERROR;
        }

        num_channels = get_num_channels((format >> RTP_AUDIO_CHANNEL_SHIFT) & 0xF);

        /*override: libaudiopcm always return mono signal                        */
        /*          but 43909 does not support mono out, so we double the signal */
        if (player->audiopcm != NULL)
            num_channels = 2;

        if (num_channels < 0)
        {
            WPRINT_APP_INFO(("Unsupported channel map 0x%x\r\n",
                             (unsigned int)(format >> RTP_AUDIO_CHANNEL_SHIFT) & 0xF));
            return WICED_ERROR;
        }

        if (format & RTP_AUDIO_BPS_24)
        {
            bits_per_sample = 24;
        }
        else
        {
            bits_per_sample = 16;
        }

        /*
         * If this is the first audio packet then we need to set the format and
         * fire off the audio render component.
         */

        if (player->rtp_data_received == WICED_FALSE)
        {
            WPRINT_APP_INFO(("First RTP packet (%lu) - parsing audio format: 0x%08lx\r\n", seq, format));

            player->rtp_data_received     = WICED_TRUE;
            player->audio_sample_rate     = sample_rate;
            player->audio_channels        = num_channels;
            player->audio_bits_per_sample = bits_per_sample;

            /*
             * Fire off the audio render component.
             */

            params.buffer_nodes = 100;
            params.buffer_ms    = player->dct_app->buffering_ms;
            params.threshold_ms = player->dct_app->threshold_ms;
            params.clock_enable = player->dct_app->clock_enable;
            params.audio_config.sample_rate     = player->audio_sample_rate;
            params.audio_config.bits_per_sample = player->audio_bits_per_sample;
            params.audio_config.channels        = player->audio_channels;
            params.audio_config.volume          = player->audio_volume;
            player->audio = wiced_audio_render_init(&params, buffer_release, player);

            if (player->audio == NULL)
            {
                WPRINT_APP_INFO(("Unable to initialize audio render component\r\n"));
                player->rtp_done = 1;
                result = WICED_ERROR;
            }
        }
        else
        {
            /*
             * We need to make sure that the configuration hasn't changed on us.
             */

            if (player->audio_sample_rate != sample_rate || player->audio_channels != num_channels ||
                player->audio_bits_per_sample != bits_per_sample)
            {
                WPRINT_APP_INFO(("Audio configuration change\r\n"));
                player->rtp_done = 1;
                result = WICED_ERROR;
            }
        }
    }

    return result;
}


#ifdef AUDIOPCM_ENABLE
static int8_t push_audio_output(int8_t* err, void* audiopcm, void* player_app,
                                unsigned char* buf, uint32_t* buf_len, uint32_t* buf_id,
                                audiopcm_audio_info_t* audio_info)
{
    /* this is the callback called from libaudiopcm */
    /* each buffer has audio_info associated.       */
    /* you can pass the buffer to the player        */
    /* or dump the audio samples in a .wav file     */
    int8_t tmperr=0;
    rtp_player_t *player = NULL;
    wiced_result_t result;

    /* WICED audio render */
    wiced_audio_render_buf_t ar_buf;

    /* unused vars: avoid compiler warnings */
    (void)audiopcm;
    (void)buf_id;

    /* chunk the payload as per mediaplayer bufpool */
    if (NULL != player_app)
    {
        player = (rtp_player_t*)player_app;

        ar_buf.data_buf    = buf;
        ar_buf.data_offset = 0;
        ar_buf.data_length = *buf_len;
        ar_buf.pts         = audio_info->sys_clk;
        ar_buf.opaque      = NULL;

        /* play the sample */
        result = wiced_audio_render_push(player->audio, &ar_buf);

        if (result != WICED_SUCCESS)
        {
            WPRINT_APP_INFO(("No free buffers in an audio queue\r\n"));
            return WICED_ERROR;
        }
    }

    if (NULL != err)
    {
        *err = tmperr;
    }

    return 0;
}
#endif


static wiced_result_t process_packet(rtp_player_t *player, wiced_packet_t *rtp_packet)
{
    uint8_t* rtp_data;
    uint16_t data_length;
    uint16_t available_data_length;
    wiced_audio_render_buf_t buf;
    wiced_result_t result;
    uint32_t* header;
    uint32_t ext_words;
    uint32_t pts_high;
    uint32_t pts_low;
    uint32_t pt;

    if (rtp_packet == NULL)
        return WICED_ERROR;

    /*
     * Get the packet data and make sure it's large enough to be a proper RTP packet.
     */

    rtp_data    = NULL;
    data_length = 0;
    result = wiced_packet_get_data(rtp_packet, 0, &rtp_data, &data_length, &available_data_length);

    if (result != WICED_SUCCESS || rtp_data == NULL || data_length == 0)
    {
        WPRINT_APP_INFO(("Invalid RTP packet at 0x%08lX, with size = %d \r\n", (uint32_t)rtp_data, data_length));
        return WICED_ERROR;
    }

    if (data_length < RTP_HEADER_SIZE)
    {
        WPRINT_APP_INFO(("RTP packet too small: %d\r\n", data_length));
        return WICED_ERROR;
    }

    /*
     * We have what appears to be a valid packet.
     */

    player->rtp_packets_received++;
    player->total_bytes_received += data_length;

    /*
     * Process the packet header.
     */

    result = process_packet_header(player, (uint32_t *)rtp_data, &pt);
    if (result != WICED_SUCCESS)
    {
        return result;
    }

    /*
     * AUDIOPCM path (if enabled)
     */

#ifdef AUDIOPCM_ENABLE
    if (player->audiopcm != NULL)
    {
        /*
         * pass the buffer to the libaudiopcm
         */
        int8_t tmperr = 0;

        audiopcm_input_push_pkt(&tmperr, player->audiopcm,
                                ntohs(*((uint16_t*)(rtp_data+2))),
                                (char*)rtp_data, (uint32_t)data_length, NULL);

        /* release this buffer */
        wiced_packet_delete(rtp_packet);
    }
    else
#endif
    {
        /*
         * non-audiopcm: We only process audio payload packets.
         */

        if (pt == RTP_PAYLOAD_AUDIO)
        {
            header = (uint32_t *)rtp_data;

            pts_high = ntohl(header[RTP_AUDIO_OFFSET_CLOCK_HIGH]);
            pts_low  = ntohl(header[RTP_AUDIO_OFFSET_CLOCK_LOW]);

            /*
             * Get the RTP extension header size.
             */

            ext_words = (ntohl(header[RTP_AUDIO_OFFSET_ID]) & 0x0000FFFF) + 1;

            /*
             * Pass this packet on to the next component.
             */

            buf.data_buf    = rtp_data;
            buf.data_offset = RTP_BASE_HEADER_SIZE + (ext_words * 4);
            buf.data_length = data_length - buf.data_offset;
            buf.pts         = (((int64_t)pts_high) << 32) | pts_low;
            buf.opaque      = rtp_packet;

            result = wiced_audio_render_push(player->audio, &buf);
            if (result != WICED_SUCCESS)
            {
                WPRINT_APP_INFO(("No free buffers in an audio queue\r\n"));
                wiced_packet_delete(rtp_packet);
                return WICED_ERROR;
            }
        }
        else
        {
            wiced_packet_delete(rtp_packet);
        }
    }

    return WICED_SUCCESS;
}


static void rtp_thread(uint32_t context)
{
    rtp_player_t *player = (rtp_player_t *)context;
    wiced_packet_t *rtp_packet;
    wiced_result_t result;
    uint32_t timeout;

    WPRINT_APP_INFO(("rtp_thread begin\r\n"));

    if (player == NULL)
        return;

    while (player->rtp_done == WICED_FALSE && player->tag == PLAYER_TAG_VALID)
    {
        /*
         * If we haven't received any data yet then we wait forever. Otherwise
         * timeout if we don't receive any more packets.
         */

        if (player->rtp_data_received)
        {
            timeout = RTP_PACKET_TIMEOUT_MS;
        }
        else
        {
            timeout = WICED_NEVER_TIMEOUT;
        }

        /*
         * Wait for a packet.
         */

        rtp_packet = NULL;
        result = wiced_udp_receive(&player->rtp_socket, &rtp_packet, timeout);
        if (result != WICED_SUCCESS)
        {
            if (result == WICED_TCPIP_TIMEOUT)
            {
                WPRINT_APP_INFO(("RTP timeout\r\n"));
                player->rtp_done = WICED_TRUE;
            }
            else if (result != WICED_TCPIP_WAIT_ABORTED)
            {
                WPRINT_APP_INFO(("Error receiving RTP packet\r\n"));
            }
            continue;
        }

        /*
         * Process the received packet.
         */

        if (process_packet(player, rtp_packet) != WICED_SUCCESS)
        {
            wiced_packet_delete(rtp_packet);
        }
    }

    WPRINT_APP_INFO(("rtp_thread end\r\n"));

    /*
     * Print out some connection info.
     */

    if (player->rtp_data_received != 0)
    {
        uint32_t bytes   = (uint32_t)player->total_bytes_received;
        uint32_t packets = (uint32_t)player->rtp_packets_received;

        WPRINT_APP_INFO(("%lu bytes received in %lu packets\r\n", bytes, packets));
    }

    /*
     * Make sure we let the main app know we're stopping.
     */

    wiced_rtos_set_event_flags(&player->events, PLAYER_EVENT_AUTOSTOP);

    WICED_END_OF_CURRENT_THREAD();
}


static wiced_result_t rtp_setup_stream_buffers(rtp_player_t *player)
{
    wiced_result_t result = WICED_SUCCESS;

    /* Guard against multiple creations of the packet pool */
    if (rtp_stream_packet_pool_created == WICED_FALSE)
    {
        /* Create extra rx packet pool on the first megabyte of the external memory, this packet pool will be used for an audio queue */
        result = wiced_network_create_packet_pool(rtp_rx_packets, (uint32_t)sizeof(rtp_rx_packets), WICED_NETWORK_PACKET_RX);
        if (result != WICED_SUCCESS)
        {
            return result;
        }
        rtp_stream_packet_pool_created = WICED_TRUE;
    }

    return WICED_SUCCESS;
}


static wiced_result_t rtp_stream_start(rtp_player_t *player)
{
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Create the UDP socket for receiving RTP packets.
     */

    WPRINT_APP_INFO(("Creating RTP socket on port %d\r\n", player->rtp_port));
    result = wiced_udp_create_socket(&player->rtp_socket, player->rtp_port, player->dct_network->interface);
    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to create RTP socket\r\n"));
        goto exit;
    }
    wiced_udp_update_socket_backlog(&player->rtp_socket, RTP_UDP_QUEUE_BACKLOG);

    /*
     * Setup our streaming buffers.
     */

    result = rtp_setup_stream_buffers(player);
    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Unable to setup RTP buffers\r\n"));
        goto exit;
    }

    /*
     * Now create the RTP thread for receiving packets.
     */

    player->rtp_done             = WICED_FALSE;
    player->seq_valid            = WICED_FALSE;
    player->rtp_data_received    = WICED_FALSE;
    player->total_bytes_received = 0;
    player->rtp_packets_received = 0;

#ifdef AUDIOPCM_ENABLE
    /*
     * Create audiopcm component
     */

    {
        int8_t rv = 0;
        audiopcm_new(&rv, &player->audiopcm, 0 /*VERBOSE*/, 0 /*sysclk*/);
        if (rv == 0)
        {
            /* setting the log verbosity to zero, only errors */
            audiopcm_set_log_verbosity(NULL, player->audiopcm, 0);

            /* config AUDIOPCM */
            audiopcm_set_label(NULL, player->audiopcm, "pcm01");
            audiopcm_set_output_push_CBACK(NULL, player->audiopcm, player, push_audio_output);
            audiopcm_set_clock(NULL, player->audiopcm, 0); /* free run clock */
        }
        else
        {
            WPRINT_APP_INFO(("AUDIOPCM component: FAILED, unable to create\r\n"));
            result = WICED_ERROR;
            goto exit;
        }
    }

    /*
     * Start the audiopcm library.
     */

    if (player->audiopcm != NULL)
    {
        int8_t rv = 0;
        audiopcm_start(&rv, player->audiopcm,
                       APCM_FORMAT_WAV|APCM_FORMAT_PCM_16,    /*expected audio format*/
                       player->dct_app->channel,               /*channel to play*/
                       APCM_DEFAULT_AUDIO_TARGET_LATENCY_MS,  /*audio target latency (ms)*/
                       APCM_DEFAULT_AUDIO_JITTER_BUFFER_MS,   /*jitter buffer (ms)*/
                       APCM_DEFAULT_AUDIO_RTT_MS);            /*average RTT (ms)*/
        if (rv != 0)
        {
            WPRINT_APP_INFO(("Unable to start audiopcm component rv=%d\r\n",rv));
            audiopcm_destroy(&rv, &player->audiopcm);
            player->audiopcm = NULL;
            result = WICED_ERROR;
            goto exit;
        }
    }
#endif

    result = wiced_rtos_create_thread_with_stack(&player->rtp_thread, RTP_THREAD_PRIORITY, "RTP thread", rtp_thread, rtp_thread_stack_buffer, RTP_THREAD_STACK_SIZE, player);
    wiced_assert( "RTP thread can not be created", result == WICED_SUCCESS );
    player->rtp_thread_ptr = &player->rtp_thread;

exit:
    return result;
}


static void rtp_stream_stop(rtp_player_t *player)
{
    if (player == NULL || player->tag != PLAYER_TAG_VALID)
    {
        return;
    }

    /*
     * Stop the rtp thread if it is running.
     */

    player->rtp_done = WICED_TRUE;
    if (player->rtp_thread_ptr != NULL)
    {
        wiced_rtos_thread_force_awake(&player->rtp_thread);
        wiced_rtos_thread_join(&player->rtp_thread);
        wiced_rtos_delete_thread(&player->rtp_thread);
        wiced_udp_delete_socket(&player->rtp_socket);
        player->rtp_thread_ptr = NULL;
    }

#ifdef AUDIOPCM_ENABLE
    if (player->audiopcm != NULL)
    {
        audiopcm_stop(NULL, player->audiopcm);
        audiopcm_destroy(NULL, &player->audiopcm);
        player->audiopcm = NULL;
    }
#endif

    if (player->audio != NULL)
    {
        wiced_audio_render_deinit(player->audio);
        player->audio = NULL;
    }
}


static void audio_mainloop(rtp_player_t *player)
{
    wiced_result_t result;
    uint32_t events;

    WPRINT_APP_INFO(("Begin audio mainloop\r\n"));

    /*
     * If auto play is set then start off by sending ourselves a play event.
     */

    if (player->dct_app->auto_play != WICED_FALSE)
    {
        wiced_rtos_set_event_flags(&player->events, PLAYER_EVENT_PLAY);
    }

    while (player->tag == PLAYER_TAG_VALID)
    {
        events = 0;
        result = wiced_rtos_wait_for_event_flags(&player->events, PLAYER_ALL_EVENTS, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_WAIT_FOREVER);
        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & PLAYER_EVENT_SHUTDOWN)
            break;

        if (events & PLAYER_EVENT_PLAY)
        {
            player->stop_received = 0;
            if (player->rtp_thread_ptr == NULL)
            {
                result = rtp_stream_start(player);
                if (result != WICED_SUCCESS)
                {
                    WPRINT_APP_INFO(("Error creating RTP thread\r\n"));
                }
            }
            else
            {
                WPRINT_APP_INFO(("RTP thread already running\r\n"));
            }
        }

        if (events & (PLAYER_EVENT_STOP | PLAYER_EVENT_AUTOSTOP))
        {
            if (player->rtp_thread_ptr != NULL)
            {
                rtp_stream_stop(player);
            }

            if (events & PLAYER_EVENT_STOP)
            {
                player->stop_received = 1;
            }
            else if (!player->stop_received)
            {
                /*
                 * Start playing again automatically.
                 */

                wiced_rtos_set_event_flags(&player->events, PLAYER_EVENT_PLAY);
            }
        }
    };

    /*
     * Make sure that playback has been shut down.
     */

    if (player->rtp_thread_ptr != NULL)
    {
        rtp_stream_stop(player);
    }

    WPRINT_APP_INFO(("End audio mainloop\r\n"));
}


static void shutdown_player(rtp_player_t *player)
{
    /*
     * Stop the rtp thread if it is running.
     */

    rtp_stream_stop(player);

    /*
     * Shutdown the console.
     */

    command_console_deinit();

    wiced_rtos_deinit_event_flags(&player->events);

    player->tag = PLAYER_TAG_INVALID;
    free(player);
}


static void set_nvram_mac(void)
{
#ifdef WWD_TEST_NVRAM_OVERRIDE
    platform_dct_wifi_config_t dct_wifi;
    wiced_result_t result;
    uint32_t size;
    uint32_t i;
    char *nvram;

    result = wiced_dct_read_with_copy(&dct_wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
    if (result != WICED_SUCCESS)
    {
        return;
    }

    if (wwd_bus_get_wifi_nvram_image(&nvram, &size) != WWD_SUCCESS)
    {
        return;
    }

    /*
     * We have the mac address from the DCT so now we just need to update the nvram image.
     * Search for the 'macaddr=' token.
     */

    for (i = 0; i < size; )
    {
        if (nvram[i] == '\0')
        {
            break;
        }

        if (nvram[i] != 'm' || nvram[i+1] != 'a' || nvram[i+2] != 'c' || nvram[i+3] != 'a' ||
            nvram[i+4] != 'd' || nvram[i+5] != 'd' || nvram[i+6] != 'r' || nvram[7] != '=')
        {
            while(i < size && nvram[i] != '\0')
            {
                i++;
            }
            i++;
            continue;
        }

        /*
         * Found the macaddr token. Now we just need to update it.
         */

        sprintf(&nvram[i+8], "%02x:%02x:%02x:%02x:%02x:%02x", dct_wifi.mac_address.octet[0],
                dct_wifi.mac_address.octet[1], dct_wifi.mac_address.octet[2], dct_wifi.mac_address.octet[3],
                dct_wifi.mac_address.octet[4], dct_wifi.mac_address.octet[5]);
        break;
    }
#endif
}


static rtp_player_t *init_player(void)
{
    rtp_player_t *player;
    wiced_result_t result;
    uint32_t tag = PLAYER_TAG_VALID;
    wiced_hostname_t hostname;

    /*
     * Temporary to work around a WiFi bug with RMC.
     */

    set_nvram_mac();

    /* Initialize the device */
    result = wiced_init();
    if (result != WICED_SUCCESS)
    {
        return NULL;
    }

    /*
     * Allocate the main player structure.
     */

    player = calloc_named("rtp_player", 1, sizeof(rtp_player_t));
    if (player == NULL)
    {
        WPRINT_APP_INFO(("Unable to allocate player structure\r\n"));
        return NULL;
    }

    /*
     * Create the command console.
     */

    WPRINT_APP_INFO(("Start the command console\r\n"));
    result = command_console_init(STDIO_UART, sizeof(rtp_command_buffer), rtp_command_buffer, RTP_CONSOLE_COMMAND_HISTORY_LENGTH, rtp_command_history_buffer, " ");
    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Error starting the command console\r\n"));
        free(player);
        return NULL;
    }
    console_add_cmd_table(rtp_command_table);

    /*
     * Create our event flags.
     */

    result = wiced_rtos_init_event_flags(&player->events);
    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Error initializing event flags\r\n"));
        tag = PLAYER_TAG_INVALID;
    }

    /* Get network configuration */
    result = wiced_dct_read_lock((void **)&player->dct_network, WICED_FALSE, DCT_NETWORK_CONFIG_SECTION, 0, sizeof(platform_dct_network_config_t));
    if (result != WICED_SUCCESS || (player->dct_network->interface != WICED_STA_INTERFACE && player->dct_network->interface != WICED_AP_INTERFACE))
    {
        WPRINT_APP_INFO(("Can't get network configuration!\r\n"));
        tag = PLAYER_TAG_INVALID;
    }

    /* Get WiFi configuration */
    result = wiced_dct_read_lock( (void **)&player->dct_wifi, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
    if (result != WICED_SUCCESS || player->dct_wifi->device_configured != WICED_TRUE)
    {
        WPRINT_APP_INFO(("Can't get WiFi configuration!\r\n"));
        tag = PLAYER_TAG_INVALID;
    }
    else
    {
        wiced_mac_t wiced_mac = {{0}};
        /* Until the MAC is OTP'ed into the board, let's check to see if the Wifi DCT mac
         *       is non-zero and different from the NVRAM value
         */
        /* check if the DCT mac is non-zero */
        if (memcmp(&wiced_mac, &player->dct_wifi->mac_address, sizeof(wiced_mac_t)) != 0)
        {
            /* check to see if the MAC address in the driver is different from what is in the Wifi DCT */
            wwd_wifi_get_mac_address( &wiced_mac, WICED_STA_INTERFACE );
            if (memcmp(&wiced_mac, &player->dct_wifi->mac_address, sizeof(wiced_mac_t)) != 0)
            {
                /* It's different, set it from wifi DCT */
                WPRINT_APP_INFO(("Apollo Setting MAC addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
                        player->dct_wifi->mac_address.octet[0], player->dct_wifi->mac_address.octet[1],
                        player->dct_wifi->mac_address.octet[2], player->dct_wifi->mac_address.octet[3],
                        player->dct_wifi->mac_address.octet[4], player->dct_wifi->mac_address.octet[5]));

                if (wwd_wifi_set_mac_address( player->dct_wifi->mac_address ) != WWD_SUCCESS)
                {
                    WPRINT_APP_INFO(("Set MAC addr failed!\r\n"));
                }
            }
        }
    }

    /* Get app specific data from Non Volatile DCT */
    result = wiced_dct_read_lock((void **)&player->dct_app, WICED_FALSE, DCT_APP_SECTION, 0, sizeof(apollo_audio_dct_t));
    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Can't get app configuration!\r\n"));
        tag = PLAYER_TAG_INVALID;
    }

    /* Set our hostname  to the speaker name - this way DHCP servers will see which unique speaker acquired a lease
     * once the network init runs ...
     */
    result = wiced_network_set_hostname( player->dct_app->speaker_name );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_NETWORK_ERROR( ("Can't set hostname for dhcp_client_init!\r\n") );
    }
    else
    {
        result = wiced_network_get_hostname( &hostname );
        if ( result != WICED_SUCCESS )
        {
            WPRINT_NETWORK_ERROR( ( "Can't get hostname for dhcp_client_init!\r\n") );
        }
    }

    /*
     * Set some default values.
     */

    player->rtp_port     = RTP_DEFAULT_PORT;
    player->rtcp_port    = player->rtp_port + 1;
    player->audio_volume = player->dct_app->volume;

    /*
     * Make sure that 802.1AS time is enabled.
     */

    wiced_time_enable_8021as();

    /* Initialize platform audio */
    platform_init_audio();

    player->tag = tag;

    return player;
}


void application_start(void)
{
    wiced_result_t result;
    rtp_player_t *player;

    /*
     * Main initialization.
     */

    if ((player = init_player()) == NULL)
    {
        return;
    }
    g_player = player;

    /* print out our current configuration */
    apollo_config_print_info(g_player);

    /* Bring up the network interface */
    result = apollo_network_up_default();

    if (result == WICED_SUCCESS)
    {
        SET_IPV4_ADDRESS(player->mrtp_ip_address, RTP_MULTICAST_IPV4_ADDRESS);
        WPRINT_APP_INFO(("Joining multicast group %d.%d.%d.%d\n",
                (int)((player->mrtp_ip_address.ip.v4 >> 24) & 0xFF), (int)((player->mrtp_ip_address.ip.v4 >> 16) & 0xFF),
                (int)((player->mrtp_ip_address.ip.v4 >> 8) & 0xFF),  (int)(player->mrtp_ip_address.ip.v4 & 0xFF)));
        result = wiced_multicast_join(player->dct_network->interface, &player->mrtp_ip_address);
    }

    if (result != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("Bringing up network interface failed!\r\n"));
        return;
    }

    /*
     * Drop into our main loop.
     */

    audio_mainloop(player);

    /*
     * Cleanup and exit.
     */

    g_player = NULL;
    shutdown_player(player);
    player = NULL;
}
