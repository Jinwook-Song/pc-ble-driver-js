/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
*
* The information contained herein is confidential property of Nordic Semiconductor. The use,
* copying, transfer or disclosure of such information is prohibited except by express written
* agreement with Nordic Semiconductor.
*
*/

#ifndef ADAPTER_INTERNAL_H__
#define ADAPTER_INTERNAL_H__

#include "sd_rpc_types.h"
#include "serialization_transport.h"

#include "nrf_error.h"
#include "ble.h"

#include <string>

class Adapter {
    public:
        explicit Adapter(SerializationTransport *transport);
        ~Adapter();
        uint32_t open(const sd_rpc_error_handler_t error_callback, const sd_rpc_evt_handler_t event_callback, const sd_rpc_log_handler_t log_callback);
        uint32_t close() const;
        static bool isInternalError(const uint32_t error_code);

        void errorHandler(sd_rpc_app_err_t code, const char * error);
        void eventHandler(ble_evt_t *event);
        void logHandler(sd_rpc_log_severity_t severity, std::string log_message);

        SerializationTransport *transport;

    private:
        sd_rpc_evt_handler_t eventCallback;
        sd_rpc_error_handler_t errorCallback;
        sd_rpc_log_handler_t logCallback;
};

#endif // ADAPTER_INTERNAL_H__
