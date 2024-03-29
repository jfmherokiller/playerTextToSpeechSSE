//
// Created by peter on 7/27/21.
//


std::vector<ISpObjectToken*> gVoices;
ISpVoice* gVoice = nullptr;

// Default settings - should match those in Papyrus

bool gModEnabled = true;
uint32_t gPlayerVoiceID = 0;
uint32_t gPlayerVoiceVolume = 50;
int32_t gPlayerVoiceRateAdjust = 0;

/*************************
 **	Player speech state **
**************************/

enum PlayerSpeechState {
    TOPIC_NOT_SELECTED = 0,
    TOPIC_SELECTED = 1,
    TOPIC_SPOKEN = 2
};

struct PlayerSpeech {
    PlayerSpeechState state;
    bool isNPCSpeechDelayed;
    uint32_t option;
};

PlayerSpeech* gPlayerSpeech = nullptr;

void initializePlayerSpeech() {
    if (gPlayerSpeech == nullptr) {
        gPlayerSpeech = new PlayerSpeech();
        gPlayerSpeech->state = TOPIC_NOT_SELECTED;
        gPlayerSpeech->isNPCSpeechDelayed = false;
    }
}

/***************************************************
 **	Event handling for when TTS finished speaking **
****************************************************/
void TopicSpokenEventDelegateFn() {
    //uint32_t setTopic = 0x006740E0;
    //uint32_t sayTopicResponse = 0x006741B0;

    if (gPlayerSpeech->state == TOPIC_SELECTED) {
        gPlayerSpeech->state = TOPIC_SPOKEN;
        gPlayerSpeech->isNPCSpeechDelayed = false;

        // Here's the fun part: once TTS stopped speaking, we have to set the topic again,
        // then speak it. It's already done on the first click event, but we're ignoring it
        // with our onDialogueSayHook to allow TTS to speak.
        //RE::MenuTopicManager* mtm = RE::MenuTopicManager::GetSingleton();
        //thisCall<void>(mtm, setTopic, gPlayerSpeech->option);
        //thisCall<void>(mtm, sayTopicResponse, 0, 0);
    }
}


void executeVoiceNotifyThread() {
    CSpEvent evt;
	HANDLE voiceNotifyHandle = gVoice->GetNotifyEventHandle();

    do {
        WaitForSingleObject(voiceNotifyHandle, INFINITE);

        while (gVoice != nullptr && evt.GetFrom(gVoice) == S_OK) {
            if (evt.eEventId == SPEI_END_INPUT_STREAM) {
                SKSE::GetTaskInterface()->AddTask(TopicSpokenEventDelegateFn);
            }
        }
    } while (gVoice != nullptr);
}

/********************************************
 **	Initializing voices and setting up TTS **
*********************************************/


std::vector<ISpObjectToken*> getVoices() {

    std::vector<ISpObjectToken*> voiceObjects;
    ULONG voiceCount = 0;
    ISpObjectToken* voiceObject;
    IEnumSpObjectTokens* enumVoiceObjects;

    SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &enumVoiceObjects);
    enumVoiceObjects->GetCount(&voiceCount);

    HRESULT hr = S_OK;
    ULONG i = 0;
    while (SUCCEEDED(hr) && i < voiceCount) {
        hr = enumVoiceObjects->Item(i, &voiceObject);
        voiceObjects.push_back(voiceObject);
        i++;
    }

    enumVoiceObjects->Release();
    return voiceObjects;
}

ULONG getVoicesCount() {
    ULONG voiceCount = 0;
    IEnumSpObjectTokens* enumVoiceObjects;
    SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &enumVoiceObjects);
    enumVoiceObjects->GetCount(&voiceCount);
    enumVoiceObjects->Release();
    return voiceCount;
}

void initializeVoices() {
    if (gVoice == nullptr) {
        gVoices = getVoices();

        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
            //_MESSAGE("Problem: CoInitializeEx failed");
            logger::error<>("Problem: CoCreateInstance failed");
        }
        else if (FAILED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void **)&gVoice))) {
            //_MESSAGE("Problem: CoCreateInstance failed");
            logger::error<>("Problem: CoCreateInstance failed");
        }

        CoUninitialize();

        ULONGLONG eventTypes = SPFEI(SPEI_END_INPUT_STREAM);

        if (FAILED(gVoice->SetInterest(eventTypes, eventTypes))) {
            //_MESSAGE("Problem: SetInterest failed");
            logger::error<>("Problem: SetInterest failed");
        }

        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)executeVoiceNotifyThread, nullptr, NULL, nullptr);
    }
}

void speak(const char* message) {
	auto masterVolumeSetting = RE::TESForm::LookupByID<RE::BGSSoundCategory>(0x000EB803)->GetCategoryVolume();
	auto voiceVolumeSetting = RE::TESForm::LookupByID<RE::BGSSoundCategory>(0x000876BD)->GetCategoryVolume();

    initializeVoices();
    std::wstringstream messageStream;
    messageStream << message;

    // Volume is set every time because master / voice volume might have changed
    gVoice->SetVolume((u_short)round((float)gPlayerVoiceVolume * static_cast<float>(masterVolumeSetting) * (voiceVolumeSetting)));
    gVoice->Speak(messageStream.str().c_str(), SPF_ASYNC | SPF_IS_NOT_XML | SPF_PURGEBEFORESPEAK, nullptr);
}

void stopSpeaking() {
    gVoice->Speak(L"", SPF_ASYNC | SPF_IS_NOT_XML | SPF_PURGEBEFORESPEAK, nullptr);
}

/*********************
 **	ON TOPIC SETTER **
**********************/
//x86
//BYTE* gOnTopicSetter = (BYTE*)0x00674113;
//x64
//uintptr_t gOnTopicSetter = 0x014056F910;
uintptr_t gOnTopicSetter;
uintptr_t gOnTopicSetterResume;
uintptr_t* gOnTopicSetterResumePtr = &gOnTopicSetterResume;

struct ObjectWithMessage {
    const char* message;
};

struct ObjectWithObjectWithMessage {
    ObjectWithMessage* object;
};

void onTopicSetterHook(RE::MenuTopicManager* object, uint32_t option)
{
    if (!gModEnabled) {
        return;
    }

    initializePlayerSpeech();

    if (gPlayerSpeech->state == TOPIC_NOT_SELECTED || gPlayerSpeech->state == TOPIC_SPOKEN) {
        gPlayerSpeech->state = TOPIC_SELECTED;
        gPlayerSpeech->isNPCSpeechDelayed = false;
        gPlayerSpeech->option = option;
        speak(object->lastSelectedDialogue->topicText.c_str());
    }
}

//__declspec(naked) void onTopicSetterHooked() {
//    __asm {
//    push edx
//    mov edx, [esp+0xC] // The selected option is an argument to the function, and is still on the stack there
//    pushad
//    push edx
//    push esi           // `esi` register here contains a pointer to an object, containing
//    // a pointer to another object with a pointer to the string of a chosen topic
//    // (I didn't bother to figure out what this object is)
//    call onTopicSetterHook
//    popad
//    pop edx
//    jmp[gOnTopicSetterResume]
//    }
//}
struct onTopicSetterHookedPatch :
    Xbyak::CodeGenerator
{
    explicit onTopicSetterHookedPatch(uintptr_t SetterHook,uintptr_t ResumePtr)
    {
        mov(rbx, ptr [rsp+0x98]);
        //mov(edx,ptr [esp+0xC]);
        //pushad();
        //push(edx);
        //push(esi);
		push(rcx);
		mov(rcx, rsi);
		push(rax);
		mov(rax, SetterHook);
		call(rax);
        pop(rax);
		pop(rcx);
        //popad();
        //pop(edx);
		mov(r15, ResumePtr);
		mov(r15, ptr[r15]);
		jmp(r15);
    }
};

/***********************
 **	DIALOGUE SAY HOOK **
************************/

uintptr_t gOnDialogueSay;
uintptr_t gOnDialogueSayResume;
uintptr_t gOnDialogueSaySkip;
uintptr_t* gOnDialogueSayResumePtr = &gOnDialogueSayResume;


bool shouldDelayNPCSpeech() {
    if (!gModEnabled) {
        return false;
    }

    initializePlayerSpeech();


    // This is for when the user wants to skip the convo by (usually vigorously) clicking
    if (gPlayerSpeech->state == TOPIC_SELECTED && gPlayerSpeech->isNPCSpeechDelayed) {
        gPlayerSpeech->state = TOPIC_NOT_SELECTED;
        gPlayerSpeech->isNPCSpeechDelayed = false;
        stopSpeaking();
    }

    else if (gPlayerSpeech->state == TOPIC_SELECTED) {
        gPlayerSpeech->isNPCSpeechDelayed = true;
        return true;
    }

    return false;
}
struct onDialogueSayHookedPatch :
    Xbyak::CodeGenerator
{
    explicit onDialogueSayHookedPatch(uintptr_t DelayTest,uintptr_t ResumePtr,uintptr_t SaySkip)
    {
        Xbyak::Label DELAY_NPC_SPEECH;
        //pushad();
        mov(ptr [rsp+0x20], ecx);
        mov(rcx, rbx);
		push(rcx);
		mov(rcx,DelayTest);
		call(rcx);
		pop(rcx);
        test(al,al);
        jnz(DELAY_NPC_SPEECH);
        //popad();
		mov(rax, ResumePtr);
		mov(rax, ptr[rax]);
		jmp(rax);

        L(DELAY_NPC_SPEECH);
		mov(r8, SaySkip);
		jmp(r8);
        //popad();
    }
};
//__declspec(naked) void onDialogueSayHooked() {
//    __asm {
//    pushad
//    call shouldDelayNPCSpeech
//    test al, al
//    jnz DELAY_NPC_SPEECH // If we should delay NPC speech, go to some code after
//    popad
//    jmp[gOnDialogueSayResume]
//
//    DELAY_NPC_SPEECH:
//    popad
//    jmp[gOnDialogueSaySkip]
//    }
//}

/**********************************************
 **	Registered functions and their delegates **
**********************************************/
void SetVoiceFn() {
    initializeVoices();
    gVoice->SetVoice(gVoices[gPlayerVoiceID]);
}

void SetRateAdjustFn() {
    initializeVoices();
    gVoice->SetRate(gPlayerVoiceRateAdjust);
}

std::string getAvailableTTSVoices(RE::StaticFunctionTag*) {
    std::vector<ISpObjectToken*> voices = getVoices(); // We can't just use `gVoices` because it's on another thread
    std::vector<std::string> vmVoiceList;
    WCHAR* szDesc{};
    const char* szDescString;

    size_t size = voices.size();
    vmVoiceList.resize(size);

    for (size_t i = 0; i < size; i++) {
        SpGetDescription(voices[i], &szDesc);
        _bstr_t szDescCommon(szDesc);
        szDescString = szDescCommon;
        vmVoiceList[i] = std::string(szDescString);
        voices[i]->Release();
    }

    voices.clear();
    auto joinedParts = boost::algorithm::join(vmVoiceList, "::");
    return joinedParts;
}

uint32_t setModEnabled(RE::StaticFunctionTag*, bool modEnabled) {
    gModEnabled = modEnabled;
    return 1; // Pretty sure it'll be successful
}

int32_t setTTSPlayerVoiceID(RE::StaticFunctionTag*, int32_t id) {
    int32_t isSuccessful = 1;

    if (static_cast<uint32_t>(id) >= getVoicesCount()) {
        id = 0;
        isSuccessful = 0;
    }

    gPlayerVoiceID = id;
    SKSE::GetTaskInterface()->AddTask(SetVoiceFn);
    return isSuccessful;
}

int32_t setTTSPlayerVoiceVolume(RE::StaticFunctionTag*, int32_t volume) {
    int32_t isSuccessful = 1;

    if (volume > 100 || volume < 0) {
        volume = 50;
        isSuccessful = 0;
    }

    gPlayerVoiceVolume = volume;
    return isSuccessful;
}

int32_t setTTSPlayerVoiceRateAdjust(RE::StaticFunctionTag*, int32_t rateAdjust) {
    int32_t isSuccessful = 1;

    if (rateAdjust < -10 || rateAdjust > 10) {
        rateAdjust = 0;
        isSuccessful = 0;
    }

    gPlayerVoiceRateAdjust = rateAdjust;
    SKSE::GetTaskInterface()->AddTask(SetRateAdjustFn);
    return isSuccessful;
}

bool registerFuncs(RE::BSScript::Internal::VirtualMachine* a_registry) {

    a_registry->RegisterFunction("GetAvailableTTSVoices", "TTS_Voiced_Player_Dialogue_MCM_Script", getAvailableTTSVoices);
    a_registry->RegisterFunction("SetTTSModEnabled", "TTS_Voiced_Player_Dialogue_MCM_Script", setModEnabled);
    a_registry->RegisterFunction("SetTTSPlayerVoiceID", "TTS_Voiced_Player_Dialogue_MCM_Script", setTTSPlayerVoiceID);
    a_registry->RegisterFunction("SetTTSPlayerVoiceVolume", "TTS_Voiced_Player_Dialogue_MCM_Script", setTTSPlayerVoiceVolume);
    a_registry->RegisterFunction("SetTTSPlayerVoiceRateAdjust", "TTS_Voiced_Player_Dialogue_MCM_Script", setTTSPlayerVoiceRateAdjust);
    return true;
}

/********************
**	Initialization **
*********************/

bool InnerPluginLoad()
{
    //x86
    //BYTE* gOnDialogueSaySkip = (BYTE*)0x006D39C4;
    //x64
    //uintptr_t gOnDialogueSaySkip = 0x01405E83D8;
	//file offset 0x005E77D8
	//RVA seems to be 0x5E83D8
	gOnDialogueSaySkip = REL::Offset(0x5E83D1).address();
    //x86
    //BYTE* gOnDialogueSay = (BYTE*)0x006D397E;
    //comparison https://i.imgur.com/i2exbOy.png
    //X64
    //uintptr_t gOnDialogueSay = 0x01405E837F;
    gOnDialogueSay = REL::Offset(0x5E83C5).address();
    gOnDialogueSayResume = REL::Offset(0x5E83C9).address();
    //x86
    //BYTE* gOnTopicSetter = (BYTE*)0x00674113;
    //x64
    //uintptr_t gOnTopicSetter = 0x014056F910;
    //comparison https://i.imgur.com/4ZAN0aU.png
	//file offset 0x0056ED10
	//RVA seems to be 0x56F910
    gOnTopicSetter = REL::Offset(0x56F910).address();
    gOnTopicSetterResume = REL::Offset(0x56F918).address();
    // These set up injection points to the game:
    auto& trampoline = SKSE::GetTrampoline();
	onTopicSetterHookedPatch Tsh{ reinterpret_cast<uintptr_t>(onTopicSetterHook), reinterpret_cast<uintptr_t>(gOnTopicSetterResumePtr) };
	onDialogueSayHookedPatch Dsh{ reinterpret_cast<uintptr_t>(shouldDelayNPCSpeech), reinterpret_cast<uintptr_t>(gOnDialogueSayResumePtr), gOnDialogueSaySkip };
    const auto onTopicSetterHooked = trampoline.allocate(Tsh);
    const auto onDialogueSayHooked = trampoline.allocate(Dsh);

        // 1. When the topic is clicked, we'd like to remember the selected
    //    option (so that we can trigger same option choice later) and actually speak the TTS message
    //gOnTopicSetterResume = detourWithTrampoline(gOnTopicSetter, (BYTE*)onTopicSetterHooked, 5);
	REL::safe_fill(gOnTopicSetter, 0x90, 8);
    trampoline.write_branch<5>(gOnTopicSetter,onTopicSetterHooked);

    // 2. When the NPC is about to speak, we'd like prevent them initially, but still allow other dialogue events.
    //    We also check there, well, if user clicks during a convo to try to skip it, we'll also stop the TTS speaking.
    //gOnDialogueSayResume = detourWithTrampoline(gOnDialogueSay, (BYTE*)onDialogueSayHooked, 6);
	//REL::safe_fill(gOnDialogueSay, 0x90, 7);
    //trampoline.write_branch<5>(gOnDialogueSay,onDialogueSayHooked);

    //if (!g_papyrusInterface) {
   //     _MESSAGE("Problem: g_papyrusInterface is false");
  //      return false;
   // }
    auto papyrus = SKSE::GetPapyrusInterface();
    if (!papyrus->Register(registerFuncs))
    {
        return false;
    }

    //g_taskInterface = static_cast<SKSETaskInterface*>(skse->QueryInterface(kInterface_Task));

    //if (!g_taskInterface) {
   //     _MESSAGE("Problem: task registration failed");
   //     return false;
   // }

    logger::info<>("TTS Voiced Player Dialogue loaded");
    return true;
}
