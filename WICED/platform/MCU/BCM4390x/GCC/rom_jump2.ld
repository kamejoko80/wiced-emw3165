/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */


.rom_jump_table__sbrk_jump :
{
    *(*._sbrk_jump )
} > ROM_JUMPS__sbrk_jump AT > ROM_JUMPS__sbrk_jump

.rom_jump_table__read_jump :
{
    *(*._read_jump )
} > ROM_JUMPS__read_jump AT > ROM_JUMPS__read_jump

.rom_jump_table__write_jump :
{
    *(*._write_jump )
} > ROM_JUMPS__write_jump AT > ROM_JUMPS__write_jump

.rom_jump_table__close_jump :
{
    *(*._close_jump )
} > ROM_JUMPS__close_jump AT > ROM_JUMPS__close_jump

.rom_jump_table__fstat_jump :
{
    *(*._fstat_jump )
} > ROM_JUMPS__fstat_jump AT > ROM_JUMPS__fstat_jump

.rom_jump_table__isatty_jump :
{
    *(*._isatty_jump )
} > ROM_JUMPS__isatty_jump AT > ROM_JUMPS__isatty_jump

.rom_jump_table__lseek_jump :
{
    *(*._lseek_jump )
} > ROM_JUMPS__lseek_jump AT > ROM_JUMPS__lseek_jump

.rom_jump_table_tx_application_define_jump :
{
    *(.text.tx_application_define_jump )
} > ROM_JUMPS_tx_application_define_jump AT > ROM_JUMPS_tx_application_define_jump

.rom_jump_table_platform_power_down_hook_jump :
{
    *(*.platform_power_down_hook_jump )
} > ROM_JUMPS_platform_power_down_hook_jump AT > ROM_JUMPS_platform_power_down_hook_jump

.rom_jump_table__tx_initialize_low_level_jump :
{
    *(*._tx_initialize_low_level_jump )
} > ROM_JUMPS__tx_initialize_low_level_jump AT > ROM_JUMPS__tx_initialize_low_level_jump

.rom_jump_table_packet_release_notify_jump :
{
    *(*.packet_release_notify_jump )
} > ROM_JUMPS_packet_release_notify_jump AT > ROM_JUMPS_packet_release_notify_jump

.rom_jump_table_host_rtos_get_tickrate_jump :
{
    *(*.host_rtos_get_tickrate_jump )
} > ROM_JUMPS_host_rtos_get_tickrate_jump AT > ROM_JUMPS_host_rtos_get_tickrate_jump

.rom_jump_table_wps_host_get_ap_list_size_jump :
{
    *(*.wps_host_get_ap_list_size_jump )
} > ROM_JUMPS_wps_host_get_ap_list_size_jump AT > ROM_JUMPS_wps_host_get_ap_list_size_jump

.rom_jump_table_wps_host_scan_jump :
{
    *(*.wps_host_scan_jump )
} > ROM_JUMPS_wps_host_scan_jump AT > ROM_JUMPS_wps_host_scan_jump

.rom_jump_table_wps_host_join_jump :
{
    *(*.wps_host_join_jump )
} > ROM_JUMPS_wps_host_join_jump AT > ROM_JUMPS_wps_host_join_jump

.rom_jump_table_wps_host_leave_jump :
{
    *(*.wps_host_leave_jump )
} > ROM_JUMPS_wps_host_leave_jump AT > ROM_JUMPS_wps_host_leave_jump

.rom_jump_table_wps_host_retrieve_ap_jump :
{
    *(*.wps_host_retrieve_ap_jump )
} > ROM_JUMPS_wps_host_retrieve_ap_jump AT > ROM_JUMPS_wps_host_retrieve_ap_jump

.rom_jump_table_wps_host_create_eapol_packet_jump :
{
    *(*.wps_host_create_eapol_packet_jump )
} > ROM_JUMPS_wps_host_create_eapol_packet_jump AT > ROM_JUMPS_wps_host_create_eapol_packet_jump

.rom_jump_table_wps_host_get_eapol_packet_size_jump :
{
    *(*.wps_host_get_eapol_packet_size_jump )
} > ROM_JUMPS_wps_host_get_eapol_packet_size_jump AT > ROM_JUMPS_wps_host_get_eapol_packet_size_jump

.rom_jump_table_wps_host_get_eapol_data_jump :
{
    *(*.wps_host_get_eapol_data_jump )
} > ROM_JUMPS_wps_host_get_eapol_data_jump AT > ROM_JUMPS_wps_host_get_eapol_data_jump

.rom_jump_table_wps_host_free_eapol_packet_jump :
{
    *(*.wps_host_free_eapol_packet_jump )
} > ROM_JUMPS_wps_host_free_eapol_packet_jump AT > ROM_JUMPS_wps_host_free_eapol_packet_jump

.rom_jump_table_wps_host_start_timer_jump :
{
    *(*.wps_host_start_timer_jump )
} > ROM_JUMPS_wps_host_start_timer_jump AT > ROM_JUMPS_wps_host_start_timer_jump

.rom_jump_table_wps_host_stop_timer_jump :
{
    *(*.wps_host_stop_timer_jump )
} > ROM_JUMPS_wps_host_stop_timer_jump AT > ROM_JUMPS_wps_host_stop_timer_jump

.rom_jump_table_wps_host_store_ap_jump :
{
    *(*.wps_host_store_ap_jump )
} > ROM_JUMPS_wps_host_store_ap_jump AT > ROM_JUMPS_wps_host_store_ap_jump

.rom_jump_table_wps_host_add_vendor_ie_jump :
{
    *(*.wps_host_add_vendor_ie_jump )
} > ROM_JUMPS_wps_host_add_vendor_ie_jump AT > ROM_JUMPS_wps_host_add_vendor_ie_jump

.rom_jump_table_wps_host_remove_vendor_ie_jump :
{
    *(*.wps_host_remove_vendor_ie_jump )
} > ROM_JUMPS_wps_host_remove_vendor_ie_jump AT > ROM_JUMPS_wps_host_remove_vendor_ie_jump

.rom_jump_table_wps_host_get_authorized_macs_jump :
{
    *(*.wps_host_get_authorized_macs_jump )
} > ROM_JUMPS_wps_host_get_authorized_macs_jump AT > ROM_JUMPS_wps_host_get_authorized_macs_jump

.rom_jump_table_wps_host_retrieve_credential_jump :
{
    *(*.wps_host_retrieve_credential_jump )
} > ROM_JUMPS_wps_host_retrieve_credential_jump AT > ROM_JUMPS_wps_host_retrieve_credential_jump

.rom_jump_table_wps_host_store_credential_jump :
{
    *(*.wps_host_store_credential_jump )
} > ROM_JUMPS_wps_host_store_credential_jump AT > ROM_JUMPS_wps_host_store_credential_jump

.rom_jump_table_wps_clear_pbc_overlap_array_jump :
{
    *(*.wps_clear_pbc_overlap_array_jump )
} > ROM_JUMPS_wps_clear_pbc_overlap_array_jump AT > ROM_JUMPS_wps_clear_pbc_overlap_array_jump

.rom_jump_table_wps_record_last_pbc_enrollee_jump :
{
    *(*.wps_record_last_pbc_enrollee_jump )
} > ROM_JUMPS_wps_record_last_pbc_enrollee_jump AT > ROM_JUMPS_wps_record_last_pbc_enrollee_jump

.rom_jump_table_wps_pbc_overlap_check_jump :
{
    *(*.wps_pbc_overlap_check_jump )
} > ROM_JUMPS_wps_pbc_overlap_check_jump AT > ROM_JUMPS_wps_pbc_overlap_check_jump

.rom_jump_table_wps_host_send_packet_jump :
{
    *(*.wps_host_send_packet_jump )
} > ROM_JUMPS_wps_host_send_packet_jump AT > ROM_JUMPS_wps_host_send_packet_jump

.rom_jump_table_wps_process_event_jump :
{
    *(*.wps_process_event_jump )
} > ROM_JUMPS_wps_process_event_jump AT > ROM_JUMPS_wps_process_event_jump

.rom_jump_table_besl_host_get_mac_address_jump :
{
    *(*.besl_host_get_mac_address_jump )
} > ROM_JUMPS_besl_host_get_mac_address_jump AT > ROM_JUMPS_besl_host_get_mac_address_jump

.rom_jump_table_besl_host_malloc_jump :
{
    *(*.besl_host_malloc_jump )
} > ROM_JUMPS_besl_host_malloc_jump AT > ROM_JUMPS_besl_host_malloc_jump

.rom_jump_table_besl_host_free_jump :
{
    *(*.besl_host_free_jump )
} > ROM_JUMPS_besl_host_free_jump AT > ROM_JUMPS_besl_host_free_jump

.rom_jump_table_besl_host_hton16_jump :
{
    *(*.besl_host_hton16_jump )
} > ROM_JUMPS_besl_host_hton16_jump AT > ROM_JUMPS_besl_host_hton16_jump

.rom_jump_table_besl_host_hton32_jump :
{
    *(*.besl_host_hton32_jump )
} > ROM_JUMPS_besl_host_hton32_jump AT > ROM_JUMPS_besl_host_hton32_jump

.rom_jump_table_besl_host_ltoh16_jump :
{
    *(*.besl_host_ltoh16_jump )
} > ROM_JUMPS_besl_host_ltoh16_jump AT > ROM_JUMPS_besl_host_ltoh16_jump

.rom_jump_table_besl_host_hton32_ptr_jump :
{
    *(*.besl_host_hton32_ptr_jump )
} > ROM_JUMPS_besl_host_hton32_ptr_jump AT > ROM_JUMPS_besl_host_hton32_ptr_jump

.rom_jump_table_besl_host_hton16_ptr_jump :
{
    *(*.besl_host_hton16_ptr_jump )
} > ROM_JUMPS_besl_host_hton16_ptr_jump AT > ROM_JUMPS_besl_host_hton16_ptr_jump

.rom_jump_table_besl_host_random_bytes_jump :
{
    *(*.besl_host_random_bytes_jump )
} > ROM_JUMPS_besl_host_random_bytes_jump AT > ROM_JUMPS_besl_host_random_bytes_jump

.rom_jump_table_p2p_send_action_frame_jump :
{
    *(*.p2p_send_action_frame_jump )
} > ROM_JUMPS_p2p_send_action_frame_jump AT > ROM_JUMPS_p2p_send_action_frame_jump

.rom_jump_table_p2p_host_negotiation_complete_jump :
{
    *(*.p2p_host_negotiation_complete_jump )
} > ROM_JUMPS_p2p_host_negotiation_complete_jump AT > ROM_JUMPS_p2p_host_negotiation_complete_jump

.rom_jump_table_p2p_host_find_device_jump :
{
    *(*.p2p_host_find_device_jump )
} > ROM_JUMPS_p2p_host_find_device_jump AT > ROM_JUMPS_p2p_host_find_device_jump

.rom_jump_table_p2p_host_add_vendor_ie_jump :
{
    *(*.p2p_host_add_vendor_ie_jump )
} > ROM_JUMPS_p2p_host_add_vendor_ie_jump AT > ROM_JUMPS_p2p_host_add_vendor_ie_jump

.rom_jump_table_p2p_host_remove_vendor_ie_jump :
{
    *(*.p2p_host_remove_vendor_ie_jump )
} > ROM_JUMPS_p2p_host_remove_vendor_ie_jump AT > ROM_JUMPS_p2p_host_remove_vendor_ie_jump

.rom_jump_table_tls_host_get_packet_data_jump :
{
    *(*.tls_host_get_packet_data_jump )
} > ROM_JUMPS_tls_host_get_packet_data_jump AT > ROM_JUMPS_tls_host_get_packet_data_jump

.rom_jump_table_tls_host_set_packet_start_jump :
{
    *(*.tls_host_set_packet_start_jump )
} > ROM_JUMPS_tls_host_set_packet_start_jump AT > ROM_JUMPS_tls_host_set_packet_start_jump

.rom_jump_table_tls_host_free_packet_jump :
{
    *(*.tls_host_free_packet_jump )
} > ROM_JUMPS_tls_host_free_packet_jump AT > ROM_JUMPS_tls_host_free_packet_jump

.rom_jump_table_tls_host_receive_packet_jump :
{
    *(*.tls_host_receive_packet_jump )
} > ROM_JUMPS_tls_host_receive_packet_jump AT > ROM_JUMPS_tls_host_receive_packet_jump

.rom_jump_table_ssl_flush_output_jump :
{
    *(*.ssl_flush_output_jump )
} > ROM_JUMPS_ssl_flush_output_jump AT > ROM_JUMPS_ssl_flush_output_jump

.rom_jump_table_tls_host_get_defragmentation_buffer_jump :
{
    *(*.tls_host_get_defragmentation_buffer_jump )
} > ROM_JUMPS_tls_host_get_defragmentation_buffer_jump AT > ROM_JUMPS_tls_host_get_defragmentation_buffer_jump

.rom_jump_table_tls_host_free_defragmentation_buffer_jump :
{
    *(*.tls_host_free_defragmentation_buffer_jump )
} > ROM_JUMPS_tls_host_free_defragmentation_buffer_jump AT > ROM_JUMPS_tls_host_free_defragmentation_buffer_jump

.rom_jump_table_tls_host_create_buffer_jump :
{
    *(*.tls_host_create_buffer_jump )
} > ROM_JUMPS_tls_host_create_buffer_jump AT > ROM_JUMPS_tls_host_create_buffer_jump

.rom_jump_table_tls_host_malloc_jump :
{
    *(*.tls_host_malloc_jump )
} > ROM_JUMPS_tls_host_malloc_jump AT > ROM_JUMPS_tls_host_malloc_jump

.rom_jump_table_tls_host_free_jump :
{
    *(*.tls_host_free_jump )
} > ROM_JUMPS_tls_host_free_jump AT > ROM_JUMPS_tls_host_free_jump

.rom_jump_table_tls_host_get_time_jump :
{
    *(*.tls_host_get_time_jump )
} > ROM_JUMPS_tls_host_get_time_jump AT > ROM_JUMPS_tls_host_get_time_jump

