/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wiced_resource.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define OTA_SERVER_PORT (1024)
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
const char upgrade_page_stylesheet_data[] =
        "    h3.one {\r\n" \
        "      color: #2E64FE;" \
        "      font-family: arial, arial, arial;\r\n" \
        "      font-style: italic;\r\n" \
        "      top:10;\r\n" \
        "      left:10;\r\n" \
        "      position:relative;\r\n" \
        "    }\r\n" \
        "    h3.two {\r\n" \
        "      color: #2E64FE;" \
        "      text-align:center;\r\n" \
        "      font-family: arial, arial, arial;\r\n" \
        "      font-style: italic;\r\n" \
        "      margin-bottom: 20;\r\n" \
        "      margin-top:0;\r\n" \
        "      height:30;\r\n" \
        "      top:3;\r\n" \
        "      position:relative;\r\n" \
        "    }\r\n" \
        "    div.one {\r\n" \
        "      background-color:#F5A9A9;\r\n" \
        "      border-radius: 10px;\r\n" \
        "      border: 1px solid #DF013A" \
        "    }\r\n" \
        "    div.upgrade_panel {\r\n" \
        "      margin-top:5;\r\n" \
        "      border: 1px solid #DF013A;\r\n" \
        "      background-color:#D8D8D8;\r\n" \
        "      border-radius: 10px;\r\n" \
        "      height: 200;\r\n" \
        "      width:600\r\n" \
        "    }\r\n" \
        "    button.upgrade_button {\r\n" \
        "      top:10;\r\n" \
        "      left:10;\r\n" \
        "      position:relative;\r\n" \
        "    }\r\n" \
        "    input.file_selector {\r\n" \
        "      top:10;\r\n" \
        "      left:10;\r\n" \
        "      position:relative;\r\n" \
        "    }\r\n";

const char upgrade_web_page_data[] =
"<html>\r\n" \
"  <head>\r\n" \
"    <link rel=\"stylesheet\" href=\"mystyle.css\">\r\n" \
"    <script type=\"text/javascript\">\r\n" \
"      var progress_run_id = null;" \
"      var control = null;" \
"      var trigger_for_upgrade = null;\r\n" \
"      function progress( current_pos, full_length ) {\r\n" \
"        var node = document.getElementById('progress');\r\n" \
"        var prg_width = parseInt(document.getElementById('progress_plane').style.width);\r\n" \
"        var transferring_stage = current_pos * 100 / full_length;\r\n" \
"        transferring_stage = Math.round(transferring_stage * 10) / 10;\r\n" \
"        var new_progress_bar_position = current_pos * prg_width / full_length;\r\n" \
"        var w    = node.style.width.match(/\\d+/);\r\n" \
"        if (current_pos == full_length){\r\n" \
"          w = 0;\r\n" \
"          document.getElementById(\"transfer_complete\").innerHTML=\"Transfer completed\";\r\n" \
"          node.style.width = prg_width + 'px';\r\n" \
"          document.getElementById(\"reboot\").innerHTML=\"Wiced device is rebooting now\";\r\n" \
"        }\r\n"\
"        else\r\n" \
"        {\r\n"
"          node.style.width = new_progress_bar_position + 'px';\r\n" \
"          console.log(\"Updating progress\");\r\n" \
"          document.getElementById(\"transfer_complete\").innerHTML=(\"Transferring \"+ transferring_stage.toString() + \"%\" );\r\n" \
"        }\r\n"
"      }\r\n"\
"      function send_file_in_chunks(){\r\n" \
"        var blobs = [];\r\n" \
"        var blob;\r\n" \
"        var xhr;\r\n" \
"        var total = 0;\r\n" \
"        var chunk_count = 0;\r\n" \
"        var file = document.getElementById(\"firmware_image\").files[0];\r\n" \
"        var bytes_per_chunk = 1024;\r\n" \
"        var start = 0;\r\n" \
"        var end = bytes_per_chunk;\r\n" \
"        var size = file.size;\r\n" \
"        var upgrade_chunk_url_with_query;\r\n" \
"        var current_pos = 0;\r\n" \
"        if (window.XMLHttpRequest)\r\n"
"        {\r\n" \
"          xhr = new XMLHttpRequest();\r\n" \
"        }\r\n" \
"        else\r\n" \
"        {\r\n" \
"          xhr = new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n" \
"        }\r\n" \
"        xhr.timeout = 2000;\r\n" \
"        upgrade_chunk_url_with_query = \"upgrade_chunk.html\" + \"?\" + \"offset=\" + current_pos + \"&\" + \"filesize=\" + size;\r\n" \
"        xhr.open('POST', upgrade_chunk_url_with_query , true);\r\n" \
"        xhr.onreadystatechange= send_function;\r\n"
"        function send_function()\r\n" \
"        {\r\n" \
"          if ( (xhr.readyState==4 && xhr.status==200) )\r\n" \
"          {\r\n" \
"            current_pos = parseInt(xhr.responseText);\r\n" \
"            console.log(\"Response received\" + current_pos);\r\n" \
"            progress(current_pos, file.size );\r\n" \
"            if( current_pos != file.size)\r\n " \
"            {\r\n" \
"              if (window.XMLHttpRequest)\r\n" \
"              {\r\n" \
"                xhr = new XMLHttpRequest();\r\n" \
"              }\r\n" \
"              else\r\n" \
"              {\r\n" \
"                xhr = new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n" \
"              }\r\n" \
"              xhr.onreadystatechange = send_function;\r\n" \
"              upgrade_chunk_url_with_query = \"upgrade_chunk.html\" + \"?\" + \"offset=\" + current_pos + \"&\" + \"filesize=\" + file.size;\r\n" \
"              xhr.open('POST', upgrade_chunk_url_with_query, true);\r\n" \
"              xhr.timeout = 4000;\r\n" \
"              chunk_count++;\r\n" \
"              console.log(\"Sending\" + chunk_count);\r\n" \
"              xhr.send(file.slice(current_pos, ( current_pos + 1024 )));\r\n" \
"            }\r\n" \
"          }\r\n" \
"          else\r\n" \
"          {\r\n" \
"            if ( (xhr.readyState==4 && xhr.status== 0) )\r\n" \
"            {\r\n" \
"              if (window.XMLHttpRequest)\r\n"
"              {\r\n" \
"                xhr = new XMLHttpRequest();\r\n" \
"              }\r\n" \
"              else\r\n" \
"              {\r\n" \
"                xhr = new ActiveXObject(\"Microsoft.XMLHTTP\");\r\n" \
"              }\r\n" \
"              xhr.onreadystatechange = send_function;\r\n" \
"              upgrade_chunk_url_with_query = \"upgrade_chunk.html\" + \"?\" + \"offset=\" + current_pos + \"&\" + \"filesize=\" + file.size;\r\n" \
"              xhr.open('POST', upgrade_chunk_url_with_query, true);\r\n" \
"              xhr.timeout = 4000;\r\n" \
"              console.log(\"Resending after comms failure\" + chunk_count + total );\r\n" \
"              xhr.send(file.slice(current_pos, ( current_pos + 1024 )));\r\n" \
"            }\r\n" \
"            else\r\n" \
"            {\r\n" \
"              console.log(\"Status =\" + xhr.readyState + \"  \" + xhr.status );\r\n" \
"            }\r\n" \
"          }\r\n" \
"        }\r\n" \
"        xhr.send(file.slice(0, 1024));\r\n" \
"      }\r\n" \
"      trigger_for_upgrade = document.getElementById(\"upgrade_button\");\r\n" \
"      control = document.getElementById(\"firmware_image\");\r\n" \
"      console.log(\"Here 2\");\r\n" \
"    </script>\r\n" \
"  </head>\r\n" \
"  <body>\r\n" \
"    <div class=\"one\" id = \"title\" >\r\n" \
"      <h3 class=\"two\" >Wiced OTA server</h3>\r\n" \
"    </div>\r\n" \
"    <div class=\"upgrade_panel\">"
"      <input class=\"file_selector\" type=\"file\" name=\"Select image\" id=\"firmware_image\"></input>\r\n" \
"      <p></p>\r\n" \
"      <button class= \"upgrade_button\" type=\"button\" id=\"upgrade_button\" onclick=\"send_file_in_chunks()\" >Start upgrade</button>\r\n" \
"      <p></p>\r\n" \
"      <p></p>\r\n" \
"      <div id=\"progress_plane\" style=\" background-color:#F5A9A9; border-radius: 10px; border: 1px solid #DF013A; width:580px; height:20px; font-family: arial, arial, arial; top:10; left:10; position:relative;\">\r\n" \
"        <div id=\"progress\" style=\" border-radius: 10px; height:20px; width:0px; background-color:#FA5858; top:10; left:10;\"></div>\r\n" \
"      </div>\r\n" \
"      <h3 class=\"one\" id = \"transfer_complete\"></h3>"
"      <h3 class=\"one\" id = \"reboot\"></h3>"
"    </div>"
"  </body>\r\n" \
"</html>\r\n";
const resource_hnd_t resource_upgrade_web_page        = { RESOURCE_IN_MEMORY, sizeof(upgrade_web_page_data),        { .mem = { ( const char* ) upgrade_web_page_data        } } };
const resource_hnd_t resource_upgrade_page_stylesheet = { RESOURCE_IN_MEMORY, sizeof(upgrade_page_stylesheet_data), { .mem = { ( const char* ) upgrade_page_stylesheet_data } } };
