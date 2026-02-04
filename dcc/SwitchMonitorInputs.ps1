
param (
    [string]$MonitorArg,
    [string]$Action = "Cycle"  # Options: "Cycle" or "DP, HDMI1, HDMI2"
)

function Switch-MonitorDPHDMI1 {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [string]$MonitorInstanceName
    )

    $vcpCode = 0x60
    $inputValues = @{
        15 = 'DP'
        16 = 'HDMI1'
        17 = 'HDMI2'
    }

    Write-Host "Searching for monitor with instance name: $MonitorInstanceName"
    $monitor = Get-Monitor | Where-Object { $_.InstanceName -eq $MonitorInstanceName }

    if (-not $monitor) {
        Write-Host "Error: Monitor not found with instance name: $MonitorInstanceName" -ForegroundColor Red
        return
    }

    # compute modifier only after confirming $monitor is present
    $monitor_modifier = if ($monitor.InstanceName -like "*DEL*") { 3840 } else { 0 }

    $currentInput = Get-MonitorVCPResponse -Monitor $monitor -VCPCode $vcpCode
    $currentInputValue = [int]$currentInput.CurrentValue
    if ($monitor_modifier -ne 0) {
        $currentInputValue = $currentInputValue - $monitor_modifier
    }

    if ($currentInputValue -eq 15) {
        $nextInput = 17
        Write-Host "Switching from DP to HDMI1"
    } elseif ($currentInputValue -eq 16) {
        $nextInput = 15
        Write-Host "Switching from HDMI1 to DP"        
    } elseif ($currentInputValue -eq 17) {
        $nextInput = 15
        Write-Host "Switching from HDMI2 to DP"
    } else {
        Write-Host "Current input is not DP or HDMI1. No action taken. Input = $currentInputValue" -ForegroundColor Yellow
        return
    }

    Get-Monitor -DeviceName $monitor.LogicalDisplay | Set-MonitorVCPValue -VCPCode $vcpCode -Value $nextInput
    Write-Host "Monitor input switched successfully." -ForegroundColor Green
}

function Switch-MonitorInputCycle {
    [CmdletBinding()]
    param (
        [Parameter(Mandatory=$true)]
        [string]$MonitorInstanceName
    )

    $vcpCode = 0x60
    # Common MCCS VCP values: DP=15 (0x0F), HDMI1=17 (0x11), HDMI2=18 (0x12)
    # Note: Some monitors use 16 for HDMI1; check your specific hardware if 17 fails.

    $monitor = Get-Monitor | Where-Object { $_.InstanceName -eq $MonitorInstanceName }

    if (-not $monitor) {
        Write-Host "Error: Monitor not found: $MonitorInstanceName" -ForegroundColor Red
        return
    }

    # Dell monitors often report values with a 3840 (0x0F00) offset
    $monitor_modifier = if ($monitor.InstanceName -like "*DEL*") { 3840 } else { 0 }

    $currentInput = Get-MonitorVCPResponse -Monitor $monitor -VCPCode $vcpCode
    $currentValue = [int]$currentInput.CurrentValue - $monitor_modifier

    # Define the Cycle: DP (15) -> HDMI1 (17) -> HDMI2 (18) -> DP (15)
    if ($currentValue -eq 15) {
        $nextInput = 17
        $msg = "DP to HDMI1"
    } elseif ($currentValue -eq 17 ) {
        $nextInput = 18
        $msg = "HDMI to HDMI2"
    } elseif ($currentValue -eq 16) {
        $nextInput = 15
        $msg = "HDMI2 to DP" 
    } else {
        # Default or fallback from HDMI2 back to DP
        $nextInput = 15
        $msg = "Back to DP"
    }

    Write-Host "Current (Adjusted): $currentValue | Switching: $msg" -ForegroundColor Cyan
    
    # Apply the new value
    Get-Monitor -DeviceName $monitor.LogicalDisplay | Set-MonitorVCPValue -VCPCode $vcpCode -Value $nextInput
    Write-Host "Successfully switched to value $nextInput" -ForegroundColor Green
}


# PS C:\controlmymonitor> Get-Monitor
# LogicalDisplay FriendlyName        InstanceName
# -------------- ------------        ------------
# \\.\DISPLAY0  Generic PnP Monitor DISPLAY\DELD107\5&2c99ba1c&2&UID256
# \\.\DISPLAY1  Generic PnP Monitor DISPLAY\AOC3201\5&2c99ba1c&2&UID260
# \\.\DISPLAY6  Generic PnP Monitor DISPLAY\GBT3209\7&31c0062b&0&UID516
# \\.\DISPLAY7  Generic PnP Monitor DISPLAY\DELD107\7&31c0062b&0&UID512
# param block moved to top of script

# Map friendly names to instance names
$monitorMap = @{
    'monitor1' = 'DISPLAY\DELD107\7&1638f3cd&2&UID772'
    'monitor2' = 'DISPLAY\GBT3209\7&1638f3cd&2&UID768'
    'monitor3' = 'DISPLAY\DELD107\7&1638f3cd&2&UID776'
    'monitor4' = 'DISPLAY\AOC3201\7&1638f3cd&2&UID780'
}

if ($MonitorArg -and $monitorMap.ContainsKey($MonitorArg.ToLower())) {
    $resolvedInstanceName = $monitorMap[$MonitorArg.ToLower()]
} else {
    $resolvedInstanceName = $MonitorArg
}

if ($resolvedInstanceName) {
    $actionNormalized = if ($Action) { $Action.ToString().Trim().ToUpper() } else { 'CYCLE' }

    if ($actionNormalized -eq 'CYCLE') {
        Switch-MonitorInputCycle -MonitorInstanceName $resolvedInstanceName
    } else {
        switch ($actionNormalized) {
            'DP'    { $targetInput = 15 }
            'HDMI1' { $targetInput = 17 }
            'HDMI2' { $targetInput = 18 }
            Default {
                Write-Host "Unknown Action: '$Action'. Use 'Cycle', 'DP', 'HDMI1' or 'HDMI2'." -ForegroundColor Yellow
                return
            }
        }

        # Resolve monitor object and apply monitor-specific modifier if needed
        $monitor = Get-Monitor | Where-Object { $_.InstanceName -eq $resolvedInstanceName }
        if (-not $monitor) {
            Write-Host "Error: Monitor not found: $resolvedInstanceName" -ForegroundColor Red
            return
        }

        $monitor_modifier = if ($monitor.InstanceName -like "*DEL*") { 3840 } else { 0 }
        $valueToSet = $targetInput + $monitor_modifier

        Write-Host "Setting monitor input to $Action (value $valueToSet) on $resolvedInstanceName" -ForegroundColor Cyan
        Get-Monitor -DeviceName $monitor.LogicalDisplay | Set-MonitorVCPValue -VCPCode 0x60 -Value $valueToSet
        Write-Host "Switched monitor input to $Action" -ForegroundColor Green
    }
} else {
    Write-Host "No monitor argument provided or monitor not found in map." -ForegroundColor Red
}
