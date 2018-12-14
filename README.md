# SAPI TTS
Node module for supporting Windows SAPI voices in an Electron app. Outside of Electron your mileage may vary. Windows only.

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


## License
MIT License

## TODO
- Better Docs
- Specs