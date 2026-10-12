param([string]$Target = 'all')
$ErrorActionPreference = 'Stop'
$allowed = @('all','legacy','grub','installer','doom','nes','freedoom','test','test-ui','test-doom','test-nes','test-install','test-storage','test-audio','test-desktop','test-network','test-internet','test-upgrade','test-resolutions','desktop-data','clean','run-legacy','run-grub')
if ($Target -notin $allowed) { throw 'Unknown make target' }
$linuxPath = (wsl -d Ubuntu -- wslpath -a $PSScriptRoot.Replace('\','/')).Trim()
wsl -d Ubuntu -- make -C $linuxPath $Target
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)" }
