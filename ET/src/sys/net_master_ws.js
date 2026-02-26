mergeInto(LibraryManager.library, {
    $WS_Master: {
        ws: null,
        peerId: null,
        serverQueue: [],
        pendingMessages: [],
        connected: false,
        reconnectDelay: 1000,
        reconnectTimer: null,
        url: null,
        peerToAddr: {},   // peerId -> {ip:[4], port:number}
        addrToPeer: {},   // "ip0.ip1.ip2.ip3:port" -> peerId
        incomingPackets: [], // {from:peerId, data:Uint8Array}

        addrKey: function(ip0, ip1, ip2, ip3, port) {
            return ip0 + '.' + ip1 + '.' + ip2 + '.' + ip3 + ':' + port;
        },

        hashPeerId: function(id) {
            var hash = 0;
            for (var i = 0; i < id.length; i++) {
                hash = (hash * 31 + id.charCodeAt(i)) | 0;
            }
            return {
                ip: [(hash >> 24) & 0xFF, (hash >> 16) & 0xFF, (hash >> 8) & 0xFF, hash & 0xFF],
                port: 27960
            };
        },

        connect: function(url) {
            if (WS_Master.ws) {
                return;
            }
            WS_Master.url = url;
            try {
                WS_Master.ws = new WebSocket(url);
            } catch (e) {
                console.error('[WS_Master] Failed to create WebSocket:', e);
                WS_Master.scheduleReconnect();
                return;
            }
            WS_Master.ws.onopen = function() {
                console.log('[WS_Master] Connected to', url);
                WS_Master.connected = true;
                WS_Master.reconnectDelay = 1000;
                // flush any messages queued while connecting
                var pending = WS_Master.pendingMessages;
                WS_Master.pendingMessages = [];
                for (var i = 0; i < pending.length; i++) {
                    WS_Master.send(pending[i]);
                }
            };
            WS_Master.ws.onclose = function() {
                console.log('[WS_Master] Disconnected');
                WS_Master.connected = false;
                WS_Master.ws = null;
                WS_Master.peerId = null;
                WS_Master.scheduleReconnect();
            };
            WS_Master.ws.onerror = function(e) {
                console.error('[WS_Master] WebSocket error:', e);
            };
            WS_Master.ws.onmessage = function(event) {
                try {
                    var data = JSON.parse(event.data);
                    if (data.type === 'id') {
                        WS_Master.peerId = data.id;
                        console.log('[WS_Master] Assigned peer id:', data.id);
                    } else if (data.type === 'servers' && Array.isArray(data.list)) {
                        WS_Master.serverQueue = data.list.slice();
                        console.log('[WS_Master] Received', data.list.length, 'servers');
                    } else if (data.type === 'relay' && data.from && data.data) {
                        var raw = typeof atob === 'function' ? atob(data.data) : Buffer.from(data.data, 'base64').toString('binary');
                        var bytes = new Uint8Array(raw.length);
                        for (var i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
                        WS_Master.incomingPackets.push({from: data.from, data: bytes});
                    }
                } catch (e) {
                    console.error('[WS_Master] Failed to parse message:', e);
                }
            };
        },

        disconnect: function() {
            if (WS_Master.reconnectTimer) {
                clearTimeout(WS_Master.reconnectTimer);
                WS_Master.reconnectTimer = null;
            }
            if (WS_Master.ws) {
                WS_Master.ws.onclose = null;
                WS_Master.ws.close();
                WS_Master.ws = null;
            }
            WS_Master.connected = false;
            WS_Master.peerId = null;
            WS_Master.pendingMessages = [];
            WS_Master.peerToAddr = {};
            WS_Master.addrToPeer = {};
            WS_Master.incomingPackets = [];
            WS_Master.url = null;
        },

        scheduleReconnect: function() {
            if (!WS_Master.url || WS_Master.reconnectTimer) {
                return;
            }
            var delay = WS_Master.reconnectDelay;
            WS_Master.reconnectDelay = Math.min(delay * 2, 30000);
            WS_Master.reconnectTimer = setTimeout(function() {
                WS_Master.reconnectTimer = null;
                if (WS_Master.url && !WS_Master.ws) {
                    WS_Master.connect(WS_Master.url);
                }
            }, delay);
        },

        send: function(obj) {
            if (WS_Master.ws && WS_Master.connected) {
                WS_Master.ws.send(JSON.stringify(obj));
            } else if (WS_Master.ws) {
                // socket exists but not yet open — queue for delivery
                WS_Master.pendingMessages.push(obj);
            }
        }
    },

    WS_MasterConnect__deps: ['$WS_Master'],
    WS_MasterConnect: function(urlPtr) {
        var url = UTF8ToString(urlPtr);
        // Allow Module.masterServerUrl to override
        if (Module.masterServerUrl) {
            url = Module.masterServerUrl;
        }
        WS_Master.connect(url);
    },

    WS_MasterDisconnect__deps: ['$WS_Master'],
    WS_MasterDisconnect: function() {
        WS_Master.disconnect();
    },

    WS_MasterHeartbeat__deps: ['$WS_Master'],
    WS_MasterHeartbeat: function(infoJsonPtr) {
        var infoStr = UTF8ToString(infoJsonPtr);
        try {
            var info = JSON.parse(infoStr);
            WS_Master.send({ type: 'heartbeat', info: info });
        } catch (e) {
            console.error('[WS_Master] Failed to parse heartbeat info:', e);
        }
    },

    WS_MasterRequestServers__deps: ['$WS_Master'],
    WS_MasterRequestServers: function() {
        WS_Master.send({ type: 'getservers' });
    },

    WS_MasterGetNextServer__deps: ['$WS_Master'],
    WS_MasterGetNextServer: function(outBuf, bufSize) {
        if (WS_Master.serverQueue.length === 0) {
            return 0;
        }
        // Master already returns Quake info strings — pass through
        var info = WS_Master.serverQueue.shift();
        var len = lengthBytesUTF8(info) + 1;
        if (len > bufSize) {
            return 0;
        }
        stringToUTF8(info, outBuf, bufSize);
        return 1;
    },

    WS_MasterIsConnected__deps: ['$WS_Master'],
    WS_MasterIsConnected: function() {
        return WS_Master.connected ? 1 : 0;
    },

    WS_RegisterPeerAddress__deps: ['$WS_Master'],
    WS_RegisterPeerAddress: function(ip0, ip1, ip2, ip3, port, peerIdPtr) {
        var peerId = UTF8ToString(peerIdPtr);
        var key = WS_Master.addrKey(ip0, ip1, ip2, ip3, port);
        WS_Master.peerToAddr[peerId] = {ip: [ip0, ip1, ip2, ip3], port: port};
        WS_Master.addrToPeer[key] = peerId;
    },

    WS_SendGamePacket__deps: ['$WS_Master'],
    WS_SendGamePacket: function(ip0, ip1, ip2, ip3, port, dataPtr, dataLen) {
        var key = WS_Master.addrKey(ip0, ip1, ip2, ip3, port);
        var peerId = WS_Master.addrToPeer[key];
        if (!peerId) return 0;
        var bytes = HEAPU8.subarray(dataPtr, dataPtr + dataLen);
        var binary = '';
        for (var i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
        var b64 = typeof btoa === 'function' ? btoa(binary) : Buffer.from(binary, 'binary').toString('base64');
        WS_Master.send({type: 'relay', target: peerId, data: b64});
        return 1;
    },

    WS_RecvGamePacket__deps: ['$WS_Master'],
    WS_RecvGamePacket: function(fromIpPtr, fromPortPtr, dataPtr, maxLen) {
        if (WS_Master.incomingPackets.length === 0) return 0;
        var pkt = WS_Master.incomingPackets.shift();
        var addr = WS_Master.peerToAddr[pkt.from];
        if (!addr) {
            // Auto-register unknown peer using hash
            addr = WS_Master.hashPeerId(pkt.from);
            var key = WS_Master.addrKey(addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3], addr.port);
            WS_Master.peerToAddr[pkt.from] = addr;
            WS_Master.addrToPeer[key] = pkt.from;
        }
        HEAPU8[fromIpPtr] = addr.ip[0];
        HEAPU8[fromIpPtr + 1] = addr.ip[1];
        HEAPU8[fromIpPtr + 2] = addr.ip[2];
        HEAPU8[fromIpPtr + 3] = addr.ip[3];
        HEAPU16[fromPortPtr >> 1] = addr.port;
        var len = Math.min(pkt.data.length, maxLen);
        HEAPU8.set(pkt.data.subarray(0, len), dataPtr);
        return len;
    }
});
