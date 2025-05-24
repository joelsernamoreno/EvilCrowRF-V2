function AutoRefresh(t) {
  setTimeout("window.location.reload(1);", t);
  }

function AutoRedirect() {
  window.setTimeout(function () {
        window.location.href = "/";
    }, 5000)
  }

function updatemenu() {
  if (document.getElementById('responsive-menu').checked == true) {
    document.getElementById('menu').style.borderBottomRightRadius = '0';
    document.getElementById('menu').style.borderBottomLeftRadius = '0';
  }else{
    document.getElementById('menu').style.borderRadius = '25px';
  }
}

// Attack Management Functions
function startAttack() {
    const form = document.getElementById('config');
    const formData = new FormData(form);
    
    // Update UI
    document.getElementById('attackStatus').textContent = 'Running';
    document.getElementById('attackProgress').textContent = '0%';
    document.getElementById('attackResult').textContent = 'In Progress';
    
    // Send attack configuration
    fetch(form.action, {
        method: 'POST',
        body: formData
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            updateAttackStatus();
        } else {
            document.getElementById('attackStatus').textContent = 'Failed';
            document.getElementById('attackResult').textContent = data.message || 'Error starting attack';
        }
    })
    .catch(error => {
        document.getElementById('attackStatus').textContent = 'Error';
        document.getElementById('attackResult').textContent = error.message;
    });
}

function stopAttack() {
    fetch('/attack/control?action=stop')
    .then(response => response.json())
    .then(data => {
        document.getElementById('attackStatus').textContent = 'Stopped';
        document.getElementById('attackProgress').textContent = '0%';
        document.getElementById('attackResult').textContent = data.message || 'Attack stopped';
    })
    .catch(error => {
        document.getElementById('attackResult').textContent = error.message;
    });
}

function updateAttackStatus() {
    fetch('/attack/control?action=status')
    .then(response => response.json())
    .then(data => {
        document.getElementById('attackStatus').textContent = data.status;
        document.getElementById('attackProgress').textContent = data.progress + '%';
        document.getElementById('attackResult').textContent = data.result || '-';
        
        if (data.status === 'Running') {
            setTimeout(updateAttackStatus, 1000);
        }
    })
    .catch(error => {
        document.getElementById('attackStatus').textContent = 'Error';
        document.getElementById('attackResult').textContent = error.message;
    });
}

// Protocol-specific form updates
function updateProtocolFields() {
    const protocol = document.getElementById('protocol').value;
    const freqInput = document.querySelector('input[name="frequency"]');
    
    switch(protocol) {
        case 'zigbee':
            freqInput.value = '2400';
            break;
        case 'zwave':
            freqInput.value = '908.42';
            break;
        case 'wifi':
            freqInput.value = '2400';
            break;
        case 'bluetooth':
            freqInput.value = '2400';
            break;
        default:
            freqInput.value = '433.92';
    }
}

// Protocol and Form Update Functions

function updateVehicleForm() {
    const protocol = document.getElementById('protocol');
    if (!protocol) return;
    
    const frequency = document.querySelector('input[name="frequency"]');
    switch(protocol.value) {
        case 'tpms':
            frequency.value = '315.00';
            break;
        case 'obd':
        case 'canbus':
            frequency.value = '433.92';
            break;
    }
}

function updateRemoteForm() {
    const remoteType = document.getElementById('remoteType');
    if (!remoteType) return;
    
    const frequency = document.querySelector('input[name="frequency"]');
    const attackType = document.getElementById('attackType');
    
    switch(remoteType.value) {
        case 'garage':
            frequency.value = '433.92';
            if (attackType) {
                attackType.innerHTML = `
                    <option value="hijack">Signal Hijacking</option>
                    <option value="replay">Record and Replay</option>
                    <option value="bruteforce">Code Brute Force</option>
                    <option value="block">Signal Blocking</option>
                `;
            }
            break;
        case 'car':
            frequency.value = '433.92';
            if (attackType) {
                attackType.innerHTML = `
                    <option value="replay">Record and Replay</option>
                    <option value="rolljam">RollJam Attack</option>
                    <option value="block">Signal Blocking</option>
                `;
            }
            break;
        case 'generic':
            if (attackType) {
                attackType.innerHTML = `
                    <option value="replay">Record and Replay</option>
                    <option value="bruteforce">Code Brute Force</option>
                    <option value="block">Signal Blocking</option>
                `;
            }
            break;
    }
}

function updateDoorbellForm() {
    const brand = document.getElementById('brand');
    if (!brand) return;
    
    const frequency = document.querySelector('input[name="frequency"]');
    switch(brand.value) {
        case 'ring':
            frequency.value = '908.42';
            break;
        case 'honeywell':
            frequency.value = '433.92';
            break;
        case 'generic':
        default:
            frequency.value = '433.92';
            break;
    }
}

// Dynamic form display functions
function toggleFormElements(attackType) {
    const elements = {
        'spoofData': ['spoof'],
        'relayOptions': ['relay'],
        'overrideOptions': ['hijack', 'replay'],
        'triggerOptions': ['trigger', 'spoof'],
        'customCommand': ['custom']
    };
    
    Object.entries(elements).forEach(([elementId, types]) => {
        const element = document.getElementById(elementId);
        if (element) {
            element.style.display = types.includes(attackType) ? 'block' : 'none';
        }
    });
}

// Event Listeners
document.addEventListener('DOMContentLoaded', function() {
    const protocolSelect = document.getElementById('protocol');
    if (protocolSelect) {
        protocolSelect.addEventListener('change', updateProtocolFields);
    }
    
    const form = document.getElementById('config');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            if (e.submitter.formAction.includes('stop')) {
                stopAttack();
            } else {
                startAttack();
            }
        });
    }
    
    // Vehicle Diagnostics form handlers
    const vehicleProtocol = document.getElementById('protocol');
    if (vehicleProtocol) {
        vehicleProtocol.addEventListener('change', updateVehicleForm);
    }
    
    // Remote Override form handlers
    const remoteType = document.getElementById('remoteType');
    if (remoteType) {
        remoteType.addEventListener('change', updateRemoteForm);
    }
    
    // Doorbell form handlers
    const doorbellBrand = document.getElementById('brand');
    if (doorbellBrand) {
        doorbellBrand.addEventListener('change', updateDoorbellForm);
    }
    
    // Attack type handlers
    const attackType = document.getElementById('attackType');
    if (attackType) {
        attackType.addEventListener('change', function() {
            toggleFormElements(this.value);
        });
    }
});
