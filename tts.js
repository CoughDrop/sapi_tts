(function () {
    var req = require;
    if(typeof(requireNode) != 'undefined') { req = requireNode; }
    var sapi = null;
    try {
        if(process.platform == 'win32') {
            if (process.arch == 'ia32') {
                sapi = req('sapi_tts/sapi_tts.32');
            } else {
                sapi = req('sapi_tts/sapi_tts.64');
            }
        }
    } catch (e) { }

    var tts = {
        exec: function () {
        },
        enabled: false
    };
    console.log("SAPI", sapi);
    if (sapi) {
        tts.enabled = true;
        tts.sapi = sapi;
        var speaker = {
            listen: function (callback) {
                speaker.listeners = speaker.listeners || [];
                speaker.listeners.push(callback);
                if (!speaker.pinging) {
                    speaker.pinging = true;
                    setTimeout(speaker.ping, 100);
                }
            },
            ping: function () {
                // There is a callback on the native speak method, but I'm not smart
                // enough to figure out how to trigger a javascript callback from
                // a c callback, so I poll instead. Don't judge.
                var res = sapi.isSpeaking();
                if (res) {
                    setTimeout(speaker.ping, 50);
                } else {
                    speaker.pinging = false;
                    while (speaker.listeners && speaker.listeners.length > 0) {
                        var cb = speaker.listeners.shift();
                        cb();
                    }
                }
            }
        };
        tts.exec = function (method, opts) {
            opts = opts || {};
            if (method == 'speakText') {
                speaker.listen(opts.success);
                if (!tts.voice_id_map) {
                    tts.getAvailableVoices();
                }
                if (tts.voice_id_map && tts.voice_id_map[opts.voice_id]) {
                    opts.voice_key = tts.voice_id_map[opts.voice_id];
                    sapi.openVoice(opts.voice_key);
                }
                if (opts.volume) {
                    opts.volume = Math.min(Math.max(0, opts.volume * 100), 150);
                }
                if (opts.rate) {
                    opts.rate = Math.min(Math.max(30, opts.rate * 100), 300);
                }
                if (opts.pitch) {
                    opts.pitch = Math.min(Math.max(50, opts.pitch * 100), 150);
                }
                opts.success = function (res) { };
            }
            var res = sapi[method](opts);
            if (res && res.ready !== false) {
                if (opts.success) {
                    opts.success(res);
                }
            } else {
                if (opts.error) {
                    opts.error({ error: "negative response to " + method });
                }
            }
        }
        var keys = ['init', 'status', 'getAvailableVoices', 'speakText', 'stopSpeakingText'];
        for (var idx = 0; idx < keys.length; idx++) {
            (function (method) {
                tts[method] = function (opts) {
                    tts.exec(method, opts);
                }
            })(keys[idx]);
        }
        tts.reload = function (opts) {
          var teardown = tts.exec('teardown');
          var init = tts.exec('init');
          if(opts && opts.success) {
            opts.success({
              teardown: teardown,
              setup: setup
            });
          }
        };
        tts.getAvailableVoices = function (opts) {
            opts = opts || {};
            console.log("getting available voices");
            var raw_list = sapi.getAvailableVoices();
            var new_list = [];
            tts.voice_id_map = {};
            for (var idx = 0; idx < raw_list.length; idx++) {
                var voice = raw_list[idx];
                voice.raw_voice_id = voice.voice_id;
                voice.voice_id = 'tts:' + voice.raw_voice_id;
                tts.voice_id_map[voice.voice_id] = voice.raw_voice_id;
                new_list.push(voice);
            }
            if (opts.success) {
                opts.success(new_list);
            }
        };
    }
    
    module.exports = tts;
})();