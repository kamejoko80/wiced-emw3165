/*
 * $ Copyright Broadcom Corporation $
 */

    .arm
    .section .text._sbrk_jump, "ax"
    .align 4
    .global _sbrk
    .global _sbrk_jump
_sbrk_jump:
    B _sbrk

    .arm
    .section .text._read_jump, "ax"
    .align 4
    .global _read
    .global _read_jump
_read_jump:
    B _read

    .arm
    .section .text._write_jump, "ax"
    .align 4
    .global _write
    .global _write_jump
_write_jump:
    B _write

    .arm
    .section .text._close_jump, "ax"
    .align 4
    .global _close
    .global _close_jump
_close_jump:
    B _close

    .arm
    .section .text._fstat_jump, "ax"
    .align 4
    .global _fstat
    .global _fstat_jump
_fstat_jump:
    B _fstat

    .arm
    .section .text._isatty_jump, "ax"
    .align 4
    .global _isatty
    .global _isatty_jump
_isatty_jump:
    B _isatty

    .arm
    .section .text._lseek_jump, "ax"
    .align 4
    .global _lseek
    .global _lseek_jump
_lseek_jump:
    B _lseek

    .arm
    .section .text.tx_application_define_jump, "ax"
    .align 4
    .global tx_application_define
    .global tx_application_define_jump
tx_application_define_jump:
    B tx_application_define

    .arm
    .section .text.platform_power_down_hook_jump, "ax"
    .align 4
    .global platform_power_down_hook
    .global platform_power_down_hook_jump
platform_power_down_hook_jump:
    B platform_power_down_hook

    .arm
    .section .text._tx_initialize_low_level_jump, "ax"
    .align 4
    .global _tx_initialize_low_level
    .global _tx_initialize_low_level_jump
_tx_initialize_low_level_jump:
    B _tx_initialize_low_level

    .arm
    .section .text.packet_release_notify_jump, "ax"
    .align 4
    .global packet_release_notify
    .global packet_release_notify_jump
packet_release_notify_jump:
    B packet_release_notify

    .arm
    .section .text.host_rtos_get_tickrate_jump, "ax"
    .align 4
    .global host_rtos_get_tickrate
    .global host_rtos_get_tickrate_jump
host_rtos_get_tickrate_jump:
    B host_rtos_get_tickrate

    .arm
    .section .text.wps_host_get_ap_list_size_jump, "ax"
    .align 4
    .global wps_host_get_ap_list_size
    .global wps_host_get_ap_list_size_jump
wps_host_get_ap_list_size_jump:
    B wps_host_get_ap_list_size

    .arm
    .section .text.wps_host_scan_jump, "ax"
    .align 4
    .global wps_host_scan
    .global wps_host_scan_jump
wps_host_scan_jump:
    B wps_host_scan

    .arm
    .section .text.wps_host_join_jump, "ax"
    .align 4
    .global wps_host_join
    .global wps_host_join_jump
wps_host_join_jump:
    B wps_host_join

    .arm
    .section .text.wps_host_leave_jump, "ax"
    .align 4
    .global wps_host_leave
    .global wps_host_leave_jump
wps_host_leave_jump:
    B wps_host_leave

    .arm
    .section .text.wps_host_retrieve_ap_jump, "ax"
    .align 4
    .global wps_host_retrieve_ap
    .global wps_host_retrieve_ap_jump
wps_host_retrieve_ap_jump:
    B wps_host_retrieve_ap

    .arm
    .section .text.wps_host_create_eapol_packet_jump, "ax"
    .align 4
    .global wps_host_create_eapol_packet
    .global wps_host_create_eapol_packet_jump
wps_host_create_eapol_packet_jump:
    B wps_host_create_eapol_packet

    .arm
    .section .text.wps_host_get_eapol_packet_size_jump, "ax"
    .align 4
    .global wps_host_get_eapol_packet_size
    .global wps_host_get_eapol_packet_size_jump
wps_host_get_eapol_packet_size_jump:
    B wps_host_get_eapol_packet_size

    .arm
    .section .text.wps_host_get_eapol_data_jump, "ax"
    .align 4
    .global wps_host_get_eapol_data
    .global wps_host_get_eapol_data_jump
wps_host_get_eapol_data_jump:
    B wps_host_get_eapol_data

    .arm
    .section .text.wps_host_free_eapol_packet_jump, "ax"
    .align 4
    .global wps_host_free_eapol_packet
    .global wps_host_free_eapol_packet_jump
wps_host_free_eapol_packet_jump:
    B wps_host_free_eapol_packet

    .arm
    .section .text.wps_host_start_timer_jump, "ax"
    .align 4
    .global wps_host_start_timer
    .global wps_host_start_timer_jump
wps_host_start_timer_jump:
    B wps_host_start_timer

    .arm
    .section .text.wps_host_stop_timer_jump, "ax"
    .align 4
    .global wps_host_stop_timer
    .global wps_host_stop_timer_jump
wps_host_stop_timer_jump:
    B wps_host_stop_timer

    .arm
    .section .text.wps_host_store_ap_jump, "ax"
    .align 4
    .global wps_host_store_ap
    .global wps_host_store_ap_jump
wps_host_store_ap_jump:
    B wps_host_store_ap

    .arm
    .section .text.wps_host_add_vendor_ie_jump, "ax"
    .align 4
    .global wps_host_add_vendor_ie
    .global wps_host_add_vendor_ie_jump
wps_host_add_vendor_ie_jump:
    B wps_host_add_vendor_ie

    .arm
    .section .text.wps_host_remove_vendor_ie_jump, "ax"
    .align 4
    .global wps_host_remove_vendor_ie
    .global wps_host_remove_vendor_ie_jump
wps_host_remove_vendor_ie_jump:
    B wps_host_remove_vendor_ie

    .arm
    .section .text.wps_host_get_authorized_macs_jump, "ax"
    .align 4
    .global wps_host_get_authorized_macs
    .global wps_host_get_authorized_macs_jump
wps_host_get_authorized_macs_jump:
    B wps_host_get_authorized_macs

    .arm
    .section .text.wps_host_retrieve_credential_jump, "ax"
    .align 4
    .global wps_host_retrieve_credential
    .global wps_host_retrieve_credential_jump
wps_host_retrieve_credential_jump:
    B wps_host_retrieve_credential

    .arm
    .section .text.wps_host_store_credential_jump, "ax"
    .align 4
    .global wps_host_store_credential
    .global wps_host_store_credential_jump
wps_host_store_credential_jump:
    B wps_host_store_credential

    .arm
    .section .text.wps_clear_pbc_overlap_array_jump, "ax"
    .align 4
    .global wps_clear_pbc_overlap_array
    .global wps_clear_pbc_overlap_array_jump
wps_clear_pbc_overlap_array_jump:
    B wps_clear_pbc_overlap_array

    .arm
    .section .text.wps_record_last_pbc_enrollee_jump, "ax"
    .align 4
    .global wps_record_last_pbc_enrollee
    .global wps_record_last_pbc_enrollee_jump
wps_record_last_pbc_enrollee_jump:
    B wps_record_last_pbc_enrollee

    .arm
    .section .text.wps_pbc_overlap_check_jump, "ax"
    .align 4
    .global wps_pbc_overlap_check
    .global wps_pbc_overlap_check_jump
wps_pbc_overlap_check_jump:
    B wps_pbc_overlap_check

    .arm
    .section .text.wps_host_send_packet_jump, "ax"
    .align 4
    .global wps_host_send_packet
    .global wps_host_send_packet_jump
wps_host_send_packet_jump:
    B wps_host_send_packet

    .arm
    .section .text.wps_process_event_jump, "ax"
    .align 4
    .global wps_process_event
    .global wps_process_event_jump
wps_process_event_jump:
    B wps_process_event

    .arm
    .section .text.besl_host_get_mac_address_jump, "ax"
    .align 4
    .global besl_host_get_mac_address
    .global besl_host_get_mac_address_jump
besl_host_get_mac_address_jump:
    B besl_host_get_mac_address

    .arm
    .section .text.besl_host_malloc_jump, "ax"
    .align 4
    .global besl_host_malloc
    .global besl_host_malloc_jump
besl_host_malloc_jump:
    B besl_host_malloc

    .arm
    .section .text.besl_host_free_jump, "ax"
    .align 4
    .global besl_host_free
    .global besl_host_free_jump
besl_host_free_jump:
    B besl_host_free

    .arm
    .section .text.besl_host_hton16_jump, "ax"
    .align 4
    .global besl_host_hton16
    .global besl_host_hton16_jump
besl_host_hton16_jump:
    B besl_host_hton16

    .arm
    .section .text.besl_host_hton32_jump, "ax"
    .align 4
    .global besl_host_hton32
    .global besl_host_hton32_jump
besl_host_hton32_jump:
    B besl_host_hton32

    .arm
    .section .text.besl_host_ltoh16_jump, "ax"
    .align 4
    .global besl_host_ltoh16
    .global besl_host_ltoh16_jump
besl_host_ltoh16_jump:
    B besl_host_ltoh16

    .arm
    .section .text.besl_host_hton32_ptr_jump, "ax"
    .align 4
    .global besl_host_hton32_ptr
    .global besl_host_hton32_ptr_jump
besl_host_hton32_ptr_jump:
    B besl_host_hton32_ptr

    .arm
    .section .text.besl_host_hton16_ptr_jump, "ax"
    .align 4
    .global besl_host_hton16_ptr
    .global besl_host_hton16_ptr_jump
besl_host_hton16_ptr_jump:
    B besl_host_hton16_ptr

    .arm
    .section .text.besl_host_random_bytes_jump, "ax"
    .align 4
    .global besl_host_random_bytes
    .global besl_host_random_bytes_jump
besl_host_random_bytes_jump:
    B besl_host_random_bytes

    .arm
    .section .text.p2p_send_action_frame_jump, "ax"
    .align 4
    .global p2p_send_action_frame
    .global p2p_send_action_frame_jump
p2p_send_action_frame_jump:
    B p2p_send_action_frame

    .arm
    .section .text.p2p_host_negotiation_complete_jump, "ax"
    .align 4
    .global p2p_host_negotiation_complete
    .global p2p_host_negotiation_complete_jump
p2p_host_negotiation_complete_jump:
    B p2p_host_negotiation_complete

    .arm
    .section .text.p2p_host_find_device_jump, "ax"
    .align 4
    .global p2p_host_find_device
    .global p2p_host_find_device_jump
p2p_host_find_device_jump:
    B p2p_host_find_device

    .arm
    .section .text.p2p_host_add_vendor_ie_jump, "ax"
    .align 4
    .global p2p_host_add_vendor_ie
    .global p2p_host_add_vendor_ie_jump
p2p_host_add_vendor_ie_jump:
    B p2p_host_add_vendor_ie

    .arm
    .section .text.p2p_host_remove_vendor_ie_jump, "ax"
    .align 4
    .global p2p_host_remove_vendor_ie
    .global p2p_host_remove_vendor_ie_jump
p2p_host_remove_vendor_ie_jump:
    B p2p_host_remove_vendor_ie

    .arm
    .section .text.tls_host_get_packet_data_jump, "ax"
    .align 4
    .global tls_host_get_packet_data
    .global tls_host_get_packet_data_jump
tls_host_get_packet_data_jump:
    B tls_host_get_packet_data

    .arm
    .section .text.tls_host_set_packet_start_jump, "ax"
    .align 4
    .global tls_host_set_packet_start
    .global tls_host_set_packet_start_jump
tls_host_set_packet_start_jump:
    B tls_host_set_packet_start

    .arm
    .section .text.tls_host_free_packet_jump, "ax"
    .align 4
    .global tls_host_free_packet
    .global tls_host_free_packet_jump
tls_host_free_packet_jump:
    B tls_host_free_packet

    .arm
    .section .text.tls_host_receive_packet_jump, "ax"
    .align 4
    .global tls_host_receive_packet
    .global tls_host_receive_packet_jump
tls_host_receive_packet_jump:
    B tls_host_receive_packet

    .arm
    .section .text.ssl_flush_output_jump, "ax"
    .align 4
    .global ssl_flush_output
    .global ssl_flush_output_jump
ssl_flush_output_jump:
    B ssl_flush_output

    .arm
    .section .text.tls_host_get_defragmentation_buffer_jump, "ax"
    .align 4
    .global tls_host_get_defragmentation_buffer
    .global tls_host_get_defragmentation_buffer_jump
tls_host_get_defragmentation_buffer_jump:
    B tls_host_get_defragmentation_buffer

    .arm
    .section .text.tls_host_free_defragmentation_buffer_jump, "ax"
    .align 4
    .global tls_host_free_defragmentation_buffer
    .global tls_host_free_defragmentation_buffer_jump
tls_host_free_defragmentation_buffer_jump:
    B tls_host_free_defragmentation_buffer

    .arm
    .section .text.tls_host_create_buffer_jump, "ax"
    .align 4
    .global tls_host_create_buffer
    .global tls_host_create_buffer_jump
tls_host_create_buffer_jump:
    B tls_host_create_buffer

    .arm
    .section .text.tls_host_malloc_jump, "ax"
    .align 4
    .global tls_host_malloc
    .global tls_host_malloc_jump
tls_host_malloc_jump:
    B tls_host_malloc

    .arm
    .section .text.tls_host_free_jump, "ax"
    .align 4
    .global tls_host_free
    .global tls_host_free_jump
tls_host_free_jump:
    B tls_host_free

    .arm
    .section .text.tls_host_get_time_jump, "ax"
    .align 4
    .global tls_host_get_time
    .global tls_host_get_time_jump
tls_host_get_time_jump:
    B tls_host_get_time

