param([string]$Executable = "$PSScriptRoot\..\..\x64\Release\SnippetTool.exe")
$ErrorActionPreference='Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class CompatibilityNative {
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
 [DllImport("user32.dll")] public static extern void keybd_event(byte key, byte scan, uint flags, UIntPtr extra);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window,uint message,IntPtr wparam,IntPtr lparam);
}
'@
function Press-Key([byte]$key) { [CompatibilityNative]::keybd_event($key,0,0,[UIntPtr]::Zero);[CompatibilityNative]::keybd_event($key,0,2,[UIntPtr]::Zero) }
function Trigger-Hotkey {
 [CompatibilityNative]::keybd_event(0x11,0,0,[UIntPtr]::Zero)
 [CompatibilityNative]::keybd_event(0x10,0,0,[UIntPtr]::Zero)
 Press-Key 0x87
 [CompatibilityNative]::keybd_event(0x10,0,2,[UIntPtr]::Zero)
 [CompatibilityNative]::keybd_event(0x11,0,2,[UIntPtr]::Zero)
}
function Read-Text($element) {
 $pattern=$null
 if($element.TryGetCurrentPattern([Windows.Automation.ValuePattern]::Pattern,[ref]$pattern)){return $pattern.Current.Value}
 if($element.TryGetCurrentPattern([Windows.Automation.TextPattern]::Pattern,[ref]$pattern)){return $pattern.DocumentRange.GetText(-1)}
 return ''
}
function Find-Editable([IntPtr]$window,[string]$preferredName='') {
 $root=[Windows.Automation.AutomationElement]::FromHandle($window)
 $all=$root.FindAll([Windows.Automation.TreeScope]::Descendants,[Windows.Automation.Condition]::TrueCondition)
 $matches=@($all|Where-Object {$_.Current.IsKeyboardFocusable -and $_.Current.ControlType.ProgrammaticName -match 'Edit|Document'})
 if($preferredName){$preferred=@($matches|Where-Object {$_.Current.Name -eq $preferredName});if($preferred.Count){return $preferred[0]}}
 if(!$matches.Count){throw 'No editable automation element was found.'}
 return $matches[0]
}
$root=Join-Path $env:TEMP "SnippetTool-Compatibility-$PID"
$oldRoot=$env:SNIPPETTOOL_TEST_ROOT
$app=$notepad=$ise=$null
$explorerWindow=[IntPtr]::Zero
try {
 New-Item -ItemType Directory $root|Out-Null
 $body='SnippetTool_'+(-join [char[]](0x65E5,0x672C,0x8A9E))+'_!'
 $escaped=[Security.SecurityElement]::Escape($body)
 $xml='<?xml version="1.0" encoding="utf-8"?><snippetTool schemaVersion="1" startupEnabled="false" firstRunCompleted="true"><snippets><snippet id="{01234567-89AB-CDEF-0123-456789ABCDEF}" name="compat" enabled="true"><hotkey modifiers="Ctrl|Shift" virtualKey="135"/><body>'+$escaped+'</body></snippet></snippets></snippetTool>'
 [IO.File]::WriteAllText((Join-Path $root 'settings.xml'),$xml,[Text.UTF8Encoding]::new($false))
 $env:SNIPPETTOOL_TEST_ROOT=$root
 $app=Start-Process (Resolve-Path $Executable) -ArgumentList '--background' -PassThru
 $null=$app.WaitForInputIdle(3000)

 $notepad=Start-Process notepad.exe -PassThru;Start-Sleep -Seconds 2;$notepad.Refresh()
 if($notepad.HasExited -or $notepad.MainWindowHandle -eq 0){$notepad=Get-Process Notepad|Where-Object MainWindowHandle -ne 0|Select-Object -Last 1}
 if(!$notepad){throw 'Notepad window was not found.'}
 $notepadEdit=Find-Editable $notepad.MainWindowHandle
 [CompatibilityNative]::SetForegroundWindow($notepad.MainWindowHandle)|Out-Null;$notepadEdit.SetFocus();Trigger-Hotkey;Start-Sleep -Milliseconds 700
 if((Read-Text $notepadEdit) -notlike "*$body*"){throw 'Notepad compatibility failed.'}

 $ise=Start-Process powershell_ise.exe -PassThru;Start-Sleep -Seconds 2;$ise.Refresh()
 $iseEdit=Find-Editable $ise.MainWindowHandle 'Console'
 [CompatibilityNative]::SetForegroundWindow($ise.MainWindowHandle)|Out-Null;$iseEdit.SetFocus();Trigger-Hotkey;Start-Sleep -Milliseconds 700
 if((Read-Text $iseEdit) -notlike "*$body*"){throw 'PowerShell ISE compatibility failed.'}

 $before=@(Get-Process explorer|Where-Object MainWindowHandle -ne 0|ForEach-Object MainWindowHandle)
 Start-Process explorer.exe -ArgumentList $env:TEMP;Start-Sleep -Seconds 2
 $candidate=Get-Process explorer|Where-Object {$_.MainWindowHandle -ne 0 -and $before -notcontains $_.MainWindowHandle}|Select-Object -First 1
 if(!$candidate){$candidate=Get-Process explorer|Where-Object MainWindowHandle -ne 0|Select-Object -Last 1}
 $explorerWindow=$candidate.MainWindowHandle
 [CompatibilityNative]::SetForegroundWindow($explorerWindow)|Out-Null
 [CompatibilityNative]::keybd_event(0x11,0,0,[UIntPtr]::Zero);Press-Key 0x4C;[CompatibilityNative]::keybd_event(0x11,0,2,[UIntPtr]::Zero);Start-Sleep -Milliseconds 200
 $address=[Windows.Automation.AutomationElement]::FocusedElement
 Trigger-Hotkey;Start-Sleep -Milliseconds 700
 if((Read-Text $address) -notlike "*$body*"){throw 'Explorer compatibility failed.'}
 [pscustomobject]@{Notepad=$true;PowerShellISE=$true;Explorer=$true;UnicodeGlobalHotkey=$true}
}
finally {
 if($notepad -and !$notepad.HasExited){Stop-Process $notepad.Id -Force}
 if($ise -and !$ise.HasExited){Stop-Process $ise.Id -Force}
 if($explorerWindow -ne [IntPtr]::Zero){[CompatibilityNative]::PostMessage($explorerWindow,0x0010,[IntPtr]::Zero,[IntPtr]::Zero)|Out-Null}
 if($app -and !$app.HasExited){Stop-Process $app.Id -Force}
 $env:SNIPPETTOOL_TEST_ROOT=$oldRoot
 Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
