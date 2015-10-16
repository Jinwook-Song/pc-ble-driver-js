'use strict';

var i = 1;

class Service {
    constructor(deviceInstanceId, uuid) {
        this._instanceId = deviceInstanceId + '.' + (i++).toString();
        this._deviceInstanceId = deviceInstanceId;
        this.uuid = uuid;

        this.characteristics = {};
    }

    // unique ID for the service (since uuid is not enough to separate between services)
    get instanceId() {
        return this._instanceId;
    }

    // device address of the remote peripheral that the GATT service belongs to. 'local' when local.
    get deviceInstanceId() {
        return this._deviceInstanceId;
    }

    get name() {
        if (this._name) {
            return this._name;
        }

        // TODO: return a name looked up in uuid_definitions
        return this.uuid;
    }

    set name(name) {
        this._name = name;
    }
}