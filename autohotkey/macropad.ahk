#Requires AutoHotkey v2.0
#SingleInstance Force

; --- 1. ELEVATE TO ADMIN ---
if !A_IsAdmin {
    Run("*RunAs `"" . A_AhkPath . "`" /restart `"" . A_ScriptFullPath . "`"")
    ExitApp()
}

; --- 2. CONFIGURATION ---
PS_SCRIPT := "c:\controlmymonitor\SwitchMonitorInputs.ps1"

; --- 3. HELPER FUNCTIONS ---
RunMonitorSwitch(monitorName, inputType) {
    command := 'powershell.exe -ExecutionPolicy Bypass -WindowStyle Hidden -File "' . PS_SCRIPT . '" ' . monitorName . ' ' . inputType
    Run(command, , "Hide")
}

; Function to trigger system sleep
PutToSleep() {
    ; Direct Method: More reliable than sending keystrokes
    ; Parameters: (Hibernate, Force, DisableWakeEvents)
    DllCall("PowrProf\SetSuspendState", "Int", 0, "Int", 0, "Int", 0)

    /*
    ; OLD METHOD (Use this if DllCall doesn't work for you)
    Send("#x")
    Sleep(300)
    Send("us")
    */
}
; --- 4. PRODUCTION HOTKEYS (CTRL + ALT + NUMPAD) ---

; Sleep Command
^!Numpad0::
^!NumpadIns::  PutToSleep()

; Monitor 1
^!Numpad1::
^!NumpadEnd::  RunMonitorSwitch("monitor1", "DP")
^!Numpad2::
^!NumpadDown:: RunMonitorSwitch("monitor1", "HDMI1")
^!Numpad3::
^!NumpadPgDn:: RunMonitorSwitch("monitor1", "HDMI2")

; Monitor 2
^!Numpad4::
^!NumpadLeft:: RunMonitorSwitch("monitor2", "DP")
^!Numpad5::
^!NumpadClear::RunMonitorSwitch("monitor2", "HDMI1")
^!Numpad6::
^!NumpadRight::RunMonitorSwitch("monitor2", "HDMI2")

; Monitor 3
^!Numpad7::
^!NumpadHome:: RunMonitorSwitch("monitor3", "DP")
^!Numpad8::
^!NumpadUp::   RunMonitorSwitch("monitor3", "HDMI1")
^!Numpad9::
^!NumpadPgUp:: RunMonitorSwitch("monitor3", "HDMI2")
