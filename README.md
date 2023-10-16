# SAPI TTS
Node module for supporting Windows SAPI voices in an Electron app. Outside of Electron your mileage may vary. Windows only.
SAPI documentation used for implementation: https://learn.microsoft.com/en-us/previous-versions/office/developer/speech-technologies/jj127903(v=msdn.10)

## Requirements
Windows

## Usage

`npm install https://www.github.com/coughdrop/sapi_tts.git`

The easiest way to use the library is to require `tts.js` in the 
node module. If you require it in the app process then you can do things
like the following:

```
var tts = require('sapi_tts/tts.js');

tts.getAvailableVoices({success: function(list) {
  console.log(list);
}});

tts.speakText({
  voice_id: "<voice id from the list>",
  text: "hello my friends, I am speaking to you",
  success: function() {
    console.log("done speaking!");
  }
});

tts.stopSpeakingText();
```

NOTE: The precompiled binaries are for a specific version of 
Electron, and may need to be recompiled for your version. I'm not
smart enough to know how to address this problem, feel free to 
issue a pull request if you are. In the mean time, I'm going to
keep it tied to the version of Electron's node-gyp as used for
coughdrop/coughdrop-desktop.

## Compiling
Compiles to a specific version of node, so most likely the compiled
binaries won't work for you. You can recompile them using node-gyp

```
npm install -g node-gyp
node-gyp clean
node-gyp configure --target=vX.X.X --arch=<ia32 or x64>
node-gyp build
```

## License
MIT License

## TODO
- Better Docs
- Specs
