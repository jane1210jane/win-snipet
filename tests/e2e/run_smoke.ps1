param([string]$Executable = "$PSScriptRoot\..\..\x64\Release\SnippetTool.exe")
$ErrorActionPreference = 'Stop'
$root = Join-Path $env:TEMP "SnippetTool-E2E-$PID"
$oldRoot = $env:SNIPPETTOOL_TEST_ROOT
$primary = $null
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Native {
 [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr window, int id);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wparam, string lparam);
 [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wparam, IntPtr lparam);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string className, string title);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
 [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern uint RegisterWindowMessage(string name);
}
'@
function Start-App([string]$exe, [switch]$Background) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    if ($Background) { $process = Start-Process -FilePath (Resolve-Path $exe) -ArgumentList '--background' -PassThru }
    else { $process = Start-Process -FilePath (Resolve-Path $exe) -PassThru }
    $null = $process.WaitForInputIdle(3000)
    $process.Refresh()
    [pscustomobject]@{ Process = $process; StartupMilliseconds = $watch.ElapsedMilliseconds }
}
try {
    $createdName = -join [char[]](0x6328,0x62F6)
    $editedName = -join [char[]](0x7DE8,0x96C6,0x6E08,0x307F)
    $bodyText = (-join [char[]](0x3053,0x3093,0x306B,0x3061,0x306F,0x3002)) + [Environment]::NewLine + (-join [char[]](0x8A18,0x53F7)) + ' & < >'
    $confirmationTitle = -join [char[]](0x78BA,0x8A8D)
    New-Item -ItemType Directory -Path $root | Out-Null
    $settings = Join-Path $root 'settings.xml'
    [IO.File]::WriteAllText($settings, '<broken', [Text.UTF8Encoding]::new($false))
    $before = (Get-FileHash $settings -Algorithm SHA256).Hash
    $env:SNIPPETTOOL_TEST_ROOT = $root
    $started = Start-App $Executable -Background
    $primary = $started.Process
    if ($primary.HasExited) { throw 'Primary process exited unexpectedly.' }
    $startupMs = $started.StartupMilliseconds
    $secondary = Start-Process -FilePath (Resolve-Path $Executable) -ArgumentList '--background' -PassThru
    if (-not $secondary.WaitForExit(3000)) { $secondary.Kill(); throw 'Secondary process did not exit.' }
    if ($secondary.ExitCode -ne 0) { throw "Secondary exit code was $($secondary.ExitCode)." }
    $primary.Refresh()
    if ($primary.HasExited) { throw 'Primary process was not retained.' }
    if ((Get-FileHash $settings -Algorithm SHA256).Hash -ne $before) { throw 'Corrupt settings were overwritten.' }
    Stop-Process -Id $primary.Id -Force
    $primary.WaitForExit()

    Remove-Item $settings -Force
    $started = Start-App $Executable
    $primary = $started.Process
    $startupMs = $started.StartupMilliseconds
    $window = $primary.MainWindowHandle
    if ($window -eq 0) { throw 'Main window was not created.' }
    [Native]::SendMessage([Native]::GetDlgItem($window,101),0x000C,[IntPtr]::Zero,$createdName) | Out-Null
    [Native]::SendMessage([Native]::GetDlgItem($window,102),0x000C,[IntPtr]::Zero,$bodyText) | Out-Null
    $hotkeyWord = 0x87 -bor (0x03 -shl 8)
    [Native]::SendMessage([Native]::GetDlgItem($window,103),0x0401,[IntPtr]$hotkeyWord,[IntPtr]::Zero) | Out-Null
    [Native]::SendMessage([Native]::GetDlgItem($window,104),0x00F1,[IntPtr]1,[IntPtr]::Zero) | Out-Null
    [Native]::SendMessage([Native]::GetDlgItem($window,105),0x00F5,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    [xml]$created = [IO.File]::ReadAllText($settings,[Text.Encoding]::UTF8)
    if ($created.snippetTool.snippets.snippet.name -ne $createdName) { throw 'GUI create failed.' }
    [Native]::SendMessage([Native]::GetDlgItem($window,101),0x000C,[IntPtr]::Zero,$editedName) | Out-Null
    [Native]::SendMessage([Native]::GetDlgItem($window,105),0x00F5,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    [xml]$edited = [IO.File]::ReadAllText($settings,[Text.Encoding]::UTF8)
    if ($edited.snippetTool.snippets.snippet.name -ne $editedName) { throw 'GUI update failed.' }
    foreach ($i in 2..10) {
        [Native]::SendMessage([Native]::GetDlgItem($window,106),0x00F5,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
        [Native]::SendMessage([Native]::GetDlgItem($window,101),0x000C,[IntPtr]::Zero,"snippet-$i") | Out-Null
        [Native]::SendMessage([Native]::GetDlgItem($window,102),0x000C,[IntPtr]::Zero,"body-$i") | Out-Null
        $key = (0x70 + $i - 1) -bor (0x03 -shl 8)
        [Native]::SendMessage([Native]::GetDlgItem($window,103),0x0401,[IntPtr]$key,[IntPtr]::Zero) | Out-Null
        [Native]::SendMessage([Native]::GetDlgItem($window,105),0x00F5,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    }
    [xml]$ten = [IO.File]::ReadAllText($settings,[Text.Encoding]::UTF8)
    if (@($ten.snippetTool.snippets.snippet).Count -ne 10) { throw 'GUI did not persist ten snippets.' }
    Stop-Process -Id $primary.Id -Force
    $primary.WaitForExit()
    $measured = Start-App $Executable -Background
    $primary = $measured.Process
    $startupMs = $measured.StartupMilliseconds
    Start-Sleep -Seconds 2
    $primary.Refresh()
    $measuredWorkingSet = $primary.WorkingSet64
    $measuredThreads = $primary.Threads.Count
    $cpuBefore = $primary.TotalProcessorTime.TotalMilliseconds
    Start-Sleep -Seconds 2
    $primary.Refresh()
    $measuredCpu = $primary.TotalProcessorTime.TotalMilliseconds - $cpuBefore
    Stop-Process -Id $primary.Id -Force
    $primary.WaitForExit()
    $started = Start-App $Executable
    $primary = $started.Process
    $window = $primary.MainWindowHandle
    $itemCount = [Native]::SendMessage([Native]::GetDlgItem($window,100),0x1004,[IntPtr]::Zero,[IntPtr]::Zero).ToInt32()
    if ($itemCount -ne 10) { throw 'Restart persistence failed.' }
    [Native]::PostMessage($window,0x0010,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
    if ([Native]::IsWindowVisible($window)) { throw 'Close did not hide the settings window.' }
    $showMessage = [Native]::RegisterWindowMessage('SnippetTool.Show')
    [Native]::PostMessage($window,$showMessage,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
    if (-not [Native]::IsWindowVisible($window)) { throw 'Existing-instance notification did not restore the window.' }
    [Native]::SendMessage([Native]::GetDlgItem($window,100),0x0100,[IntPtr]0x28,[IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 100
    [Native]::PostMessage([Native]::GetDlgItem($window,107),0x00F5,[IntPtr]::Zero,[IntPtr]::Zero) | Out-Null
    $deadline = [DateTime]::UtcNow.AddSeconds(2)
    do { Start-Sleep -Milliseconds 25; $confirm=[Native]::FindWindow('#32770',$confirmationTitle) } while ($confirm -eq 0 -and [DateTime]::UtcNow -lt $deadline)
    if ($confirm -eq 0) { throw 'Delete confirmation was not shown.' }
    [Native]::PostMessage($confirm,0x0111,[IntPtr]6,[IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds 200
    [xml]$deleted = [IO.File]::ReadAllText($settings,[Text.Encoding]::UTF8)
    if (@($deleted.snippetTool.snippets.snippet).Count -ne 9) { throw 'GUI delete failed.' }
    [pscustomobject]@{
        StartupMilliseconds = $startupMs
        WorkingSetMiB = [math]::Round($measuredWorkingSet / 1MB, 2)
        ThreadCount = $measuredThreads
        CpuMillisecondsOver2Seconds = [math]::Round($measuredCpu, 2)
        CorruptSettingsPreserved = $true
        SecondaryExitCode = $secondary.ExitCode
        GuiCrudAndRestart = $true
        SnippetsCreated = 10
        TrayCloseAndRestore = $true
    }
}
finally {
    if ($primary -and -not $primary.HasExited) { Stop-Process -Id $primary.Id -Force; $primary.WaitForExit() }
    $env:SNIPPETTOOL_TEST_ROOT = $oldRoot
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
