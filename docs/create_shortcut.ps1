$DesktopPath = [Environment]::GetFolderPath('Desktop')
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut("$DesktopPath\IPC_Cockpit.lnk")
$Shortcut.TargetPath = "h:\IPC\start_cockpit.bat"
$Shortcut.WorkingDirectory = "h:\IPC"
$Shortcut.IconLocation = "shell32.dll,23"
$Shortcut.Description = "IPC Intelligent Planning Cockpit"
$Shortcut.Save()
write-host "Shortcut successfully created on Desktop at $DesktopPath\IPC_Cockpit.lnk!"
