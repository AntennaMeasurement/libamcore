function(amcore_filter_transport_notices variable)
    if(AMCORE_TELEMETRY)
        string(REGEX REPLACE
            "\\[[^\r\n]*\\] [NW]: (lws_plat_vhost_tls_client_ctx_init: Imported [0-9]+ certs from plat store|lws_plat_set_socket_options_ip: not implemented on windows platform)\r?\n"
            "" filtered "${${variable}}")
        set(${variable} "${filtered}" PARENT_SCOPE)
    endif()
endfunction()
