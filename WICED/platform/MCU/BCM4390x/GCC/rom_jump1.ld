/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */



ROM_JUMPS__sbrk_jump                                    (rwx) : ORIGIN = 0x0069f000, LENGTH = 16
ROM_JUMPS__read_jump                                    (rwx) : ORIGIN = 0x0069f010, LENGTH = 16
ROM_JUMPS__write_jump                                   (rwx) : ORIGIN = 0x0069f020, LENGTH = 16
ROM_JUMPS__close_jump                                   (rwx) : ORIGIN = 0x0069f030, LENGTH = 16
ROM_JUMPS__fstat_jump                                   (rwx) : ORIGIN = 0x0069f040, LENGTH = 16
ROM_JUMPS__isatty_jump                                  (rwx) : ORIGIN = 0x0069f050, LENGTH = 16
ROM_JUMPS__lseek_jump                                   (rwx) : ORIGIN = 0x0069f060, LENGTH = 16
ROM_JUMPS_tx_application_define_jump                    (rwx) : ORIGIN = 0x0069f070, LENGTH = 16
ROM_JUMPS_platform_power_down_hook_jump                 (rwx) : ORIGIN = 0x0069f080, LENGTH = 16
ROM_JUMPS__tx_initialize_low_level_jump                 (rwx) : ORIGIN = 0x0069f090, LENGTH = 16
ROM_JUMPS_packet_release_notify_jump                    (rwx) : ORIGIN = 0x0069f0a0, LENGTH = 16
ROM_JUMPS_host_rtos_get_tickrate_jump                   (rwx) : ORIGIN = 0x0069f0b0, LENGTH = 16
ROM_JUMPS_wps_host_get_ap_list_size_jump                (rwx) : ORIGIN = 0x0069f0c0, LENGTH = 16
ROM_JUMPS_wps_host_scan_jump                            (rwx) : ORIGIN = 0x0069f0d0, LENGTH = 16
ROM_JUMPS_wps_host_join_jump                            (rwx) : ORIGIN = 0x0069f0e0, LENGTH = 16
ROM_JUMPS_wps_host_leave_jump                           (rwx) : ORIGIN = 0x0069f0f0, LENGTH = 16
ROM_JUMPS_wps_host_retrieve_ap_jump                     (rwx) : ORIGIN = 0x0069f100, LENGTH = 16
ROM_JUMPS_wps_host_create_eapol_packet_jump             (rwx) : ORIGIN = 0x0069f110, LENGTH = 16
ROM_JUMPS_wps_host_get_eapol_packet_size_jump           (rwx) : ORIGIN = 0x0069f120, LENGTH = 16
ROM_JUMPS_wps_host_get_eapol_data_jump                  (rwx) : ORIGIN = 0x0069f130, LENGTH = 16
ROM_JUMPS_wps_host_free_eapol_packet_jump               (rwx) : ORIGIN = 0x0069f140, LENGTH = 16
ROM_JUMPS_wps_host_start_timer_jump                     (rwx) : ORIGIN = 0x0069f150, LENGTH = 16
ROM_JUMPS_wps_host_stop_timer_jump                      (rwx) : ORIGIN = 0x0069f160, LENGTH = 16
ROM_JUMPS_wps_host_store_ap_jump                        (rwx) : ORIGIN = 0x0069f170, LENGTH = 16
ROM_JUMPS_wps_host_add_vendor_ie_jump                   (rwx) : ORIGIN = 0x0069f180, LENGTH = 16
ROM_JUMPS_wps_host_remove_vendor_ie_jump                (rwx) : ORIGIN = 0x0069f190, LENGTH = 16
ROM_JUMPS_wps_host_get_authorized_macs_jump             (rwx) : ORIGIN = 0x0069f1a0, LENGTH = 16
ROM_JUMPS_wps_host_retrieve_credential_jump             (rwx) : ORIGIN = 0x0069f1b0, LENGTH = 16
ROM_JUMPS_wps_host_store_credential_jump                (rwx) : ORIGIN = 0x0069f1c0, LENGTH = 16
ROM_JUMPS_wps_clear_pbc_overlap_array_jump              (rwx) : ORIGIN = 0x0069f1d0, LENGTH = 16
ROM_JUMPS_wps_record_last_pbc_enrollee_jump             (rwx) : ORIGIN = 0x0069f1e0, LENGTH = 16
ROM_JUMPS_wps_pbc_overlap_check_jump                    (rwx) : ORIGIN = 0x0069f1f0, LENGTH = 16
ROM_JUMPS_wps_host_send_packet_jump                     (rwx) : ORIGIN = 0x0069f200, LENGTH = 16
ROM_JUMPS_wps_process_event_jump                        (rwx) : ORIGIN = 0x0069f210, LENGTH = 16
ROM_JUMPS_besl_host_get_mac_address_jump                (rwx) : ORIGIN = 0x0069f220, LENGTH = 16
ROM_JUMPS_besl_host_malloc_jump                         (rwx) : ORIGIN = 0x0069f230, LENGTH = 16
ROM_JUMPS_besl_host_free_jump                           (rwx) : ORIGIN = 0x0069f240, LENGTH = 16
ROM_JUMPS_besl_host_hton16_jump                         (rwx) : ORIGIN = 0x0069f250, LENGTH = 16
ROM_JUMPS_besl_host_hton32_jump                         (rwx) : ORIGIN = 0x0069f260, LENGTH = 16
ROM_JUMPS_besl_host_ltoh16_jump                         (rwx) : ORIGIN = 0x0069f270, LENGTH = 16
ROM_JUMPS_besl_host_hton32_ptr_jump                     (rwx) : ORIGIN = 0x0069f280, LENGTH = 16
ROM_JUMPS_besl_host_hton16_ptr_jump                     (rwx) : ORIGIN = 0x0069f290, LENGTH = 16
ROM_JUMPS_besl_host_random_bytes_jump                   (rwx) : ORIGIN = 0x0069f2a0, LENGTH = 16
ROM_JUMPS_p2p_send_action_frame_jump                    (rwx) : ORIGIN = 0x0069f2b0, LENGTH = 16
ROM_JUMPS_p2p_host_negotiation_complete_jump            (rwx) : ORIGIN = 0x0069f2c0, LENGTH = 16
ROM_JUMPS_p2p_host_find_device_jump                     (rwx) : ORIGIN = 0x0069f2d0, LENGTH = 16
ROM_JUMPS_p2p_host_add_vendor_ie_jump                   (rwx) : ORIGIN = 0x0069f2e0, LENGTH = 16
ROM_JUMPS_p2p_host_remove_vendor_ie_jump                (rwx) : ORIGIN = 0x0069f2f0, LENGTH = 16
ROM_JUMPS_tls_host_get_packet_data_jump                 (rwx) : ORIGIN = 0x0069f310, LENGTH = 16
ROM_JUMPS_tls_host_set_packet_start_jump                (rwx) : ORIGIN = 0x0069f320, LENGTH = 16
ROM_JUMPS_tls_host_free_packet_jump                     (rwx) : ORIGIN = 0x0069f330, LENGTH = 16
ROM_JUMPS_tls_host_receive_packet_jump                  (rwx) : ORIGIN = 0x0069f340, LENGTH = 16
ROM_JUMPS_ssl_flush_output_jump                         (rwx) : ORIGIN = 0x0069f350, LENGTH = 16
ROM_JUMPS_tls_host_get_defragmentation_buffer_jump      (rwx) : ORIGIN = 0x0069f360, LENGTH = 16
ROM_JUMPS_tls_host_free_defragmentation_buffer_jump     (rwx) : ORIGIN = 0x0069f370, LENGTH = 16
ROM_JUMPS_tls_host_create_buffer_jump                   (rwx) : ORIGIN = 0x0069f380, LENGTH = 16
ROM_JUMPS_tls_host_malloc_jump                          (rwx) : ORIGIN = 0x0069f390, LENGTH = 16
ROM_JUMPS_tls_host_free_jump                            (rwx) : ORIGIN = 0x0069f3a0, LENGTH = 16
ROM_JUMPS_tls_host_get_time_jump                        (rwx) : ORIGIN = 0x0069f3b0, LENGTH = 16
