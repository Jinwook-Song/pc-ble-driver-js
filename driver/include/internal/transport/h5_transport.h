/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#ifndef H5_TRANSPORT_H
#define H5_TRANSPORT_H

#include "transport.h"

#include <mutex>
#include <condition_variable>

#include <vector>

#include <stdint.h>
#include <map>
#include <thread>
#include "h5.h"

typedef enum
{
    STATE_START,
    STATE_RESET,
    STATE_UNINITIALIZED,
    STATE_INITIALIZED,
    STATE_ACTIVE,
    STATE_FAILED,
    STATE_UNKNOWN
} h5_state_t;

typedef std::function<h5_state_t()> state_action_t;

class ExitCriterias
{
public:
    ExitCriterias() : ioResourceError(false), close(false) {}
    virtual ~ExitCriterias() {}

    bool ioResourceError;
    bool close;

    virtual bool isFullfilled() const = 0;

    virtual void reset()
    {
        ioResourceError = false; close = false;
    }
};

class StartExitCriterias : public ExitCriterias
{
public:
    bool isOpened;

    StartExitCriterias()
        : ExitCriterias(),
        isOpened(false) {}

    bool isFullfilled() const override
    {
        return (isOpened || ioResourceError);
    }

    void reset() override
    {
        ExitCriterias::reset();
        isOpened = false;
    }
};

class UninitializedExitCriterias : public ExitCriterias
{
public:
    bool syncSent;
    bool syncRspReceived;

    UninitializedExitCriterias()
        : ExitCriterias(),
        syncSent(false),
        syncRspReceived(false) {}

    bool isFullfilled() const override
    {
        return (syncSent && syncRspReceived) || ioResourceError || close;
    }

    void reset() override
    {
        ExitCriterias::reset();
        syncSent = false;
        syncRspReceived = false;
    }
};

class InitializedExitCriterias : public ExitCriterias
{
public:
    bool syncConfigSent;
    bool syncConfigRspReceived;
    bool syncConfigReceived;
    bool syncConfigRspSent;

    InitializedExitCriterias()
        : ExitCriterias(),
        syncConfigSent(false),
        syncConfigRspReceived(false),
        syncConfigReceived(false),
        syncConfigRspSent(false) {}

    bool isFullfilled() const override
    {
        return ioResourceError || close || (syncConfigSent && syncConfigRspReceived && syncConfigReceived && syncConfigRspSent);
    }

    void reset() override
    {
        ExitCriterias::reset();
        syncConfigSent = false;
        syncConfigRspSent = false;
        syncConfigReceived = false;
        syncConfigRspReceived = false;
    };

};

class ActiveExitCriterias : public ExitCriterias
{
public:
    bool irrecoverableSyncError;
    bool syncReceived;

    ActiveExitCriterias()
        : ExitCriterias(), irrecoverableSyncError(false),
        syncReceived(false) {}

    bool isFullfilled() const override {
        return ioResourceError || syncReceived || close || irrecoverableSyncError;
    }

    void reset() override
    {
        ExitCriterias::reset();
        irrecoverableSyncError = false;
        syncReceived = false;
        close = false;
    }
};

class ResetExitCriterias : public ExitCriterias
{
public:
    bool resetSent;

    ResetExitCriterias()
        : ExitCriterias(), resetSent(false)
    {}

    bool isFullfilled() const override
    {
        return ioResourceError || close || resetSent;
    }

    void reset() override
    {
        ExitCriterias::reset();
        resetSent = false;
    }
};

class H5Transport : public Transport {
public:
    H5Transport(Transport *nextTransportLayer, uint32_t retransmission_timeout);
    ~H5Transport();
    uint32_t open(error_cb_t error_callback, data_cb_t data_callback, log_cb_t log_callback) override;
    uint32_t close() override;
    uint32_t send(std::vector<uint8_t> &data) override;

private:
    void dataHandler(uint8_t *data, uint32_t length);
    void errorHandler(sd_rpc_app_err_t code, const char * error);
    void processPacket(std::vector<uint8_t>& packet);

    void sendAck();
    void sendSync();
    void sendSyncResponse();
    void sendSyncConfig();
    void sendSyncConfigResponse();
    void sendReset();

    void incrementSeqNum();
    void incrementAckNum();

    Transport *nextTransportLayer;
    std::vector<uint8_t> lastPacket;
    uint8_t remainingRetransmissions;
    uint8_t seqNum;
    uint8_t ackNum;

    bool c0Found;
    std::vector<uint8_t> unprocessedData;

    std::mutex syncMutex; // TODO: evaluate a new name for syncMutex
    std::condition_variable syncWaitCondition; // TODO: evaluate a new name for syncWaitCondition

    uint32_t retransmissionTimeout;
    std::mutex ackMutex;
    std::condition_variable ackWaitCondition;

    // Debugging related
    uint32_t incomingPacketCount;
    uint32_t outgoingPacketCount;
    uint32_t errorPacketCount;

    void logPacket(bool outgoing, std::vector<uint8_t> &packet);
    void log(std::string &logLine) const;
    void log(char const *logLine) const;
    void logStateTransition(h5_state_t from, h5_state_t to) const;
    static std::string stateToString(h5_state_t state);
    std::string asHex(std::vector<uint8_t> &packet) const;
    std::string hciPacketLinkControlToString(std::vector<uint8_t> payload) const;
    std::string h5PktToString(bool out, std::vector<uint8_t> &h5Packet) const;
    static std::string pktTypeToString(h5_pkt_type_t pktType);

    // State machine related
    h5_state_t currentState;

    std::map<h5_state_t, state_action_t> stateActions;
    void setupStateMachine();
    void startStateMachine();
    void stopStateMachine();

    std::map<h5_state_t, ExitCriterias*> exitCriterias;

    std::thread *stateMachineThread;
    void stateMachineWorker();
    bool runStateMachine;

    std::mutex stateMutex; // Mutex that allows threads to wait for a given state in the state machine
    bool waitForState(h5_state_t state, uint32_t timeoutInMillis);
    std::condition_variable stateWaitCondition;
};

#endif //H5_TRANSPORT_H
