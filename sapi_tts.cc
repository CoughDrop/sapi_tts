#include <node.h>
#include <math.h>
#include <Windows.h>
#include <sapi.h>
#pragma warning(disable:4996)
#include <sphelper.h>
#pragma warning(default: 4996) 
#include <iostream>
#include <string>

namespace sapi_tts {
	using v8::Local;
	using v8::Persistent;
	using v8::Handle;
	using v8::Isolate;
	using v8::FunctionCallbackInfo;
	using v8::Object;
	using v8::HandleScope;
	using v8::String;
	using v8::Boolean;
	using v8::Array;
	using v8::Number;
	using v8::Value;
	using v8::Null;
	using v8::Function;
	using node::AtExit;

	static bool already_setup = false;
	static double downloadPercent = 0;
	static char * last_voice = 0;
	static bool dll_ready = false;
	static bool initialized = false;

	std::wstring str_to_ws(const std::string& s)
	{
		int len;
		int slength = (int)s.length() + 1;
		len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
		wchar_t* buf = new wchar_t[len];
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
		std::wstring r(buf);
		delete[] buf;
		return r;
	}

	ISpVoice * pVoice = NULL;

	bool setup() {
		printf("SAPI Setting Up...\n");
		if (already_setup) {
			printf("SAPI Already Set Up\n");
			return true;
		}

		bool success = SUCCEEDED(::CoInitialize(NULL));
		if(success) {
  			already_setup = true;
  		}

		return success;
	}

	void jsStatus(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		bool ready = setup();
		Local<Object> obj = Object::New(isolate);
		obj->Set(String::NewFromUtf8(isolate, "ready"), Boolean::New(isolate, ready));
		args.GetReturnValue().Set(obj);
	}

	bool closeVoice() {
		printf("SAPI_Close %s\n", last_voice);
		if (pVoice) {
			pVoice->Release();
			pVoice = NULL;
		}
		return true;
	}

	bool teardown() {
		if (!already_setup) { return true; }

		printf("SAPI Tearing down \n");
		closeVoice();
		bool success = true;
		::CoUninitialize();
		last_voice = 0;
		already_setup = false;
		return success;
	}

	Handle<Array> listVoices(Isolate* isolate) {
		teardown();
		setup();

		IEnumSpObjectTokens* cpEnum;
		printf("SAPI Retrieving Voices...\n");
		HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);
		ULONG i = 0, ulCount = 0;
		hr = cpEnum->GetCount(&ulCount);
		Handle<Array> result = Array::New(isolate, ulCount);

		ISpObjectToken* tok;
		CSpDynamicString szDesc;// = L"12345678901234567890123456789012345678901234567890";
		LANGID lang(0);
		char iso[5] = { 0 };
		char ctry[5] = { 0 };
		while (SUCCEEDED(hr) && i<ulCount)
		{
			hr = cpEnum->Next(1, &tok, NULL);
			SpGetLanguageFromToken(tok, &lang);
			GetLocaleInfo(lang, LOCALE_SISO639LANGNAME, iso, sizeof(iso) / sizeof(WCHAR));
			GetLocaleInfo(lang, LOCALE_SISO3166CTRYNAME, ctry, sizeof(ctry) / sizeof(WCHAR));
			CSpDynamicString dstrDefaultName;
			SpGetDescription(tok, &dstrDefaultName);
			SpGetDescription(tok, &szDesc);

			LPWSTR lang_id;
			tok->GetId(&lang_id);
			std::string lang_id_str = CW2A(lang_id);
			const char* id = lang_id_str.c_str();
			CoTaskMemFree(lang_id);

			char locale[20] = "en-US";
			snprintf(locale, 20, "%s-%s", iso, ctry);

			char* desc = CW2A(szDesc);

			Local<Object> obj = Object::New(isolate);
			obj->Set(String::NewFromUtf8(isolate, "voice_id"), String::NewFromUtf8(isolate, id));
			obj->Set(String::NewFromUtf8(isolate, "name"), String::NewFromUtf8(isolate, desc));
			obj->Set(String::NewFromUtf8(isolate, "locale"), String::NewFromUtf8(isolate, locale));
			obj->Set(String::NewFromUtf8(isolate, "language"), String::NewFromUtf8(isolate, locale));
			obj->Set(String::NewFromUtf8(isolate, "active"), Boolean::New(isolate, true));

			result->Set(i, obj); // String::Utf8Value("asdf"));
			printf("SAPI Voice: %s Speaker: %s Language: %s Version: %s\n", id, desc, locale, locale);

			i++;
		}
		printf("SAPI Number of voice: %d\n", i);
		cpEnum->Release();
		return result;
	}

	bool openVoice(const char * voice_string) {
		if (!already_setup) { setup(); }

		if (last_voice != 0 && strcmp(last_voice, voice_string) == 0) {
			return true;
		} else if(last_voice != 0) {
	   	  printf("SAPI Closing last voice before opening new one %s %s\n", last_voice, voice_string);
		  closeVoice();
		}
		HRESULT hresult = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);

		if(FAILED(hresult)) { return false; }

		IEnumSpObjectTokens* cpEnum;
		HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);
		ULONG i = 0, ulCount = 0;
		hr = cpEnum->GetCount(&ulCount);

		ISpObjectToken* tok;
		while (SUCCEEDED(hr) && i<ulCount)
		{
			hr = cpEnum->Next(1, &tok, NULL);
			LPWSTR lang_id;
			tok->GetId(&lang_id);
			std::string lang_id_str = CW2A(lang_id);
			const char *id = lang_id_str.c_str();
			CoTaskMemFree(lang_id);

			if (strcmp(voice_string, id) == 0) {
				pVoice->SetVoice(tok);
				last_voice = (char *)malloc(strlen(voice_string) + 1);
				strcpy(last_voice, voice_string);
			}
			i++;
		}
		cpEnum->Release();

		printf("SAP_Create %s, %s\n", last_voice, voice_string);
		last_voice = (char *) malloc(strlen(voice_string) + 1);
		strcpy(last_voice, voice_string);
		return true;
	}

	static bool isSpeaking;

	bool speakText(Isolate * isolate, Local<Object> opts) {
		isSpeaking = true;
		String::Utf8Value string8(Local<String>::Cast(opts->Get(String::NewFromUtf8(isolate, "text"))));
		const char * text = *string8;
		double speed = opts->Get(String::NewFromUtf8(isolate, "rate"))->NumberValue();
		double volume = opts->Get(String::NewFromUtf8(isolate, "volume"))->NumberValue();
		double pitch = opts->Get(String::NewFromUtf8(isolate, "pitch"))->NumberValue();
		printf("SAPI values: %G %G %G\n", speed, volume, pitch);
		if (!speed || speed == 0 || isnan(speed)) {
			speed = 100;
		}
		if (!volume || volume == 0 || isnan(volume)) {
			volume = 100;
		}
		if (!pitch || pitch == 0 || isnan(pitch)) {
			pitch = 100;
		}
		if (pVoice) {
			pVoice->SetRate((((long) speed) - 100) / 100);
			pVoice->SetVolume((short) volume);
			// TODO: include pitch in XML markup, <prosody pitch="+5%">text</prosody>
			std::string str(text);
			std::wstring speak = str_to_ws(str);
			HRESULT hresult = pVoice->Speak(speak.c_str(), SVSFlagsAsync, NULL);
		} else {
			printf("SAPI Cannot speak text, voice is not set");
		}

		Local<Function> func = Local<Function>::Cast(opts->Get(String::NewFromUtf8(isolate, "success")));
		return true;
	}

	bool stopSpeakingText() {
		if (pVoice) {
			ULONG ref;
			pVoice->Skip(L"SENTENCE", 100, &ref);
		}
		return true;
	}

	void jsSetup(const FunctionCallbackInfo<Value>& args) {
		bool result = setup();
		args.GetReturnValue().Set(result);
	}

	void jsTeardown(const FunctionCallbackInfo<Value>& args) {
		bool result = teardown();
		args.GetReturnValue().Set(result);
	}

	void jsSpeak(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Local<Object> opts = Local<Object>::Cast(args[0]);
		bool result = speakText(isolate, opts);
		args.GetReturnValue().Set(result);
	}

	void jsStopSpeaking(const FunctionCallbackInfo<Value>& args) {
		bool result = stopSpeakingText();
		args.GetReturnValue().Set(result);
	}

	void jsSpeakCheck(const FunctionCallbackInfo<Value>& args) {
		if (isSpeaking) {
			if (pVoice) {
				HANDLE handle = pVoice->SpeakCompleteEvent();
				DWORD res = WaitForSingleObject(handle, 1);
				if (res == WAIT_ABANDONED || res == WAIT_TIMEOUT) {
					isSpeaking = true;
				}
				else if(res != WAIT_FAILED) {
					isSpeaking = false;
				}
			}
			else {
				isSpeaking = false;
			}
		}
		args.GetReturnValue().Set(isSpeaking);
	}

	void jsListVoices(const FunctionCallbackInfo<Value>& args) {
		Isolate* isolate = args.GetIsolate();
		Handle<Array> result = listVoices(isolate);
		args.GetReturnValue().Set(result);
	}

	void jsOpenVoice(const FunctionCallbackInfo<Value>& args) {
		String::Utf8Value string(args[0]);
		const char * voice_string = *string;
		bool result = openVoice(voice_string);
		args.GetReturnValue().Set(result);
	}

	void jsCloseVoice(const FunctionCallbackInfo<Value>& args) {
		bool result = closeVoice();
		args.GetReturnValue().Set(result);
	}

	void init(Local<Object> exports) {
		NODE_SET_METHOD(exports, "status", jsStatus);
		NODE_SET_METHOD(exports, "init", jsSetup);
		NODE_SET_METHOD(exports, "teardown", jsTeardown);
		NODE_SET_METHOD(exports, "speakText", jsSpeak);
		NODE_SET_METHOD(exports, "stopSpeakingText", jsStopSpeaking);
		NODE_SET_METHOD(exports, "isSpeaking", jsSpeakCheck);
		NODE_SET_METHOD(exports, "getAvailableVoices", jsListVoices);
		NODE_SET_METHOD(exports, "openVoice", jsOpenVoice);
		NODE_SET_METHOD(exports, "closeVoice", jsCloseVoice);
	}

	NODE_MODULE(sapi_tts, init)
}