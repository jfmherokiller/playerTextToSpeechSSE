Scriptname TTS_Voiced_Player_Dialogue_MCM_Script extends SKI_ConfigBase
Import StringUtil

string Function GetAvailableTTSVoices() Global Native
int Function SetTTSModEnabled(bool modEnabled) Global Native
int Function SetTTSPlayerVoiceID(int voiceId) Global Native
int Function SetTTSPlayerVoiceVolume(int volume) Global Native
int Function SetTTSPlayerVoiceRateAdjust(int rateAdjust) Global Native

bool modEnabled = true
string[] availableVoices
int playerVoiceID = 0
int playerVoiceVolume = 50
int playerVoiceRateAdjust = 0
int modEnabledOID
int voiceOID
int volumeOID
int rateAdjustOID

Function VoiceInit()
	If (SetTTSModEnabled(modEnabled) == 0)
		modEnabled = true
	EndIf

	If (SetTTSPlayerVoiceID(playerVoiceID) == 0)
		playerVoiceID = 0
	EndIf

	If (SetTTSPlayerVoiceVolume(playerVoiceVolume) == 0)
		playerVoiceVolume = 50
	EndIf

	If (SetTTSPlayerVoiceRateAdjust(playerVoiceRateAdjust) == 0)
		playerVoiceRateAdjust = 0
	EndIf
EndFunction

string Function ShortenVoiceOption(string voiceString)
	If (GetLength(voiceString) > 25)
		Return Substring(voiceString, 0, 23) + "..."
	EndIf

	Return voiceString
EndFunction

;Event OnConfigInit()
;	VoiceInit()
;EndEvent

Event OnGameReload()
	parent.OnGameReload()
	
	Utility.WaitMenuMode(1.0)
	VoiceInit()
EndEvent

Event  OnPageReset(string page)
	If (page == "Player Voice")
		availableVoices = StringUtil.Split(GetAvailableTTSVoices(),"::")
		SetCursorFillMode(TOP_TO_BOTTOM)
		modEnabledOID = AddToggleOption("Player TTS Enabled", modEnabled)
		voiceOID = AddMenuOption("Voice", ShortenVoiceOption(availableVoices[playerVoiceID]))
		volumeOID = AddSliderOption("Volume", playerVoiceVolume as float, "{0} out of 100")
		rateAdjustOID = AddSliderOption("Rate Adjust", playerVoiceRateAdjust as float, "{0}")
	EndIf
EndEvent

Event OnOptionSelect(int option)
	If (option == modEnabledOID)
		modEnabled = !modEnabled

		If (SetTTSModEnabled(modEnabled) == 0)
			modEnabled = true
		EndIf

		SetToggleOptionValue(modEnabledOID, modEnabled)
	EndIf
EndEvent

Event OnOptionSliderOpen(int option)
	If (option == volumeOID)
		SetSliderDialogStartValue(playerVoiceVolume as float)
		SetSliderDialogDefaultValue(50.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1.0)

	ElseIf (option == rateAdjustOID)
		SetSliderDialogStartValue(playerVoiceRateAdjust as float)
		SetSliderDialogDefaultValue(0.0)
		SetSliderDialogRange(-10.0, 10.0)
		SetSliderDialogInterval(1.0)
	EndIf
EndEvent

Event OnOptionSliderAccept(int option, float value)
	If (option == volumeOID)
		If (SetTTSPlayerVoiceVolume(value as int) == 0)
			value = 50.0
		EndIf

		playerVoiceVolume = value as int
		SetSliderOptionValue(volumeOID, value, "{0} out of 100")

	ElseIf (option == rateAdjustOID)
		If (SetTTSPlayerVoiceRateAdjust(value as int) == 0)
			value = 0.0
		EndIf

		playerVoiceRateAdjust = value as int
		SetSliderOptionValue(rateAdjustOID, value, "{0}")
	EndIf
EndEvent

Event OnOptionMenuOpen(int option)
	If (option == voiceOID)
		SetMenuDialogOptions(availableVoices)
		SetMenuDialogStartIndex(playerVoiceID)
		SetMenuDialogDefaultIndex(0)
	EndIf
EndEvent

Event OnOptionMenuAccept(int option, int index)
	If (option == voiceOID)
		If (SetTTSPlayerVoiceID(index) == 0)
			index = 0
		EndIf

		playerVoiceID = index		
		SetMenuOptionValue(voiceOID, ShortenVoiceOption(availableVoices[index]))
	EndIf
EndEvent